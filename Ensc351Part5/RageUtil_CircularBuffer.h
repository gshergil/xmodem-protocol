//============================================================================
//
//% Student Name 1: Ryan Lui
//% Student 1 #: 301251951
//% Student 1 userid (email): rclui (rclui@sfu.ca)
//
//% Student Name 2: Winsey Chui
//% Student 2 #: 301246253
//% Student 2 userid (email): winseyc (winseyc@sfu.ca)
//
//% Below, edit to list any people who helped you with the code in this file,
//%      or put 'None' if nobody helped (the two of) you.
//
// Helpers: Ivana Jovasevic
//
// Also, list any resources beyond the course textbooks and the course pages on Piazza
// that you used in making your submission.
//
// Resources: None
//
//%% Instructions:
//% * Put your name(s), student number(s), userid(s) in the above section.
//% * Also enter the above information in other files to submit.
//% * Edit the "Helpers" line and, if necessary, the "Resources" line.
//% * Your group name should be "P5_<userid1>_<userid2>" (eg. P1_stu1_stu2)
//% * Form groups as described at:  https://courses.cs.sfu.ca/docs/students
//% * Submit files to courses.cs.sfu.ca
//
// Notes:
// Check out the following functions for atomic comments:
// - advance_read_pointer
// - advance_write_pointer
// - get_read_pointers
// - get_write_pointers

/* CircBuf - A fast, thread-safe, lockless circular buffer. */
/* read()/write() interface adjusted by Craig Scratchley to be similar
 * to the posix read() and write() functions in order to increase efficiency.
 * Craig Scratchley -- March 29 2011 */
 
#ifndef RAGE_UTIL_CIRCULAR_BUFFER
#define RAGE_UTIL_CIRCULAR_BUFFER

#include <cstring>
#include <atomic>

/* Lock-free circular buffer.  This should be threadsafe if one thread is reading
 * and another is writing. */
template<class T>
class CircBuf
{
	T *buf;
	/* read_pos is the position data is read from; write_pos is the position
	 * data is written to.  If read_pos == write_pos, the buffer is empty.
	 *
	 * There will always be at least one position empty, as a completely full
	 * buffer (read_pos == write_pos) is indistinguishable from an empty buffer.
	 *
	 * Invariants: read_pos < size, write_pos < size. */
	unsigned size;
	unsigned m_iBlockSize;

	// Craig says:  making the variables volatile won't make this code thread safe.
	/* These are volatile to prevent reads and writes to them from being optimized. */
	/*volatile*/
	std::atomic<unsigned> read_pos, write_pos; // Use atomics, and below use the memory ordering from the book.

public:
	CircBuf()
	{
		buf = nullptr;
		clear();
	}

	~CircBuf()
	{
		delete[] buf;
	}

    
    // Change this function later - rclui
	void swap( CircBuf &rhs )
	{
		std::swap( size, rhs.size );
		std::swap( m_iBlockSize, rhs.m_iBlockSize );
		std::swap( read_pos, rhs.read_pos );
		std::swap( write_pos, rhs.write_pos );
		std::swap( buf, rhs.buf );
	}

	CircBuf &operator=( const CircBuf &rhs )
	{
		CircBuf c( rhs );
		this->swap( c );
		return *this;
	}

	CircBuf( const CircBuf &cpy )
	{
		size = cpy.size;
		read_pos.store(cpy.read_pos.load());
		write_pos.store(cpy.write_pos.load());
		m_iBlockSize = cpy.m_iBlockSize;
		if( size )
		{
			buf = new T[size];
			std::memcpy( buf, cpy.buf, size*sizeof(T) );
		}
		else
		{
			buf = nullptr;
		}
	}

	/* Return the number of elements available to read. */
    // Modified to use atomic variable methods. - rclui
    // Should we consider returning inside the if/else blocks? - rclui
	unsigned num_readable() const
	{
		const int rpos = read_pos.load();
		const int wpos = write_pos.load();
		if( rpos < wpos )
			/* The buffer looks like "eeeeDDDDeeee" (e = empty, D = data). */
			return wpos - rpos;
		else if( rpos > wpos )
			/* The buffer looks like "DDeeeeeeeeDD" (e = empty, D = data). */
			return size - (rpos - wpos);
		else // if( rpos == wpos )
			/* The buffer looks like "eeeeeeeeeeee" (e = empty, D = data). */
			return 0;
	}

	/* Return the number of writable elements. */
    // Modified to use atomic variable methods. - rclui
    // Should we consider returning inside the if/else blocks? - rclui
	unsigned num_writable() const
	{
		const int rpos = read_pos.load();
		const int wpos = write_pos.load();
 
 		int ret;
 		if( rpos < wpos )
 			/* The buffer looks like "eeeeDDDDeeee" (e = empty, D = data). */
 			ret = size - (wpos - rpos);
 		else if( rpos > wpos )
 			/* The buffer looks like "DDeeeeeeeeDD" (e = empty, D = data). */
 			ret = rpos - wpos;
 		else // if( rpos == wpos )
 			/* The buffer looks like "eeeeeeeeeeee" (e = empty, D = data). */
 			ret = size;
 
 		/* Subtract the blocksize, to account for the element that we never fill
		 * while keeping the entries aligned to m_iBlockSize. */
        return ret - m_iBlockSize;
	}

	unsigned capacity() const { return size; }

	void reserve( unsigned n, int iBlockSize = 1 )
	{
		m_iBlockSize = iBlockSize;

		clear();
		delete[] buf;
		buf = nullptr;

		/* Reserve an extra byte.  We'll never fill more than n bytes; the extra
		 * byte is to guarantee that read_pos != write_pos when the buffer is full,
		 * since that would be ambiguous with an empty buffer. */
		if( n != 0 )
		{
			size = n+1;
			size = ((size + iBlockSize - 1) / iBlockSize) * iBlockSize; // round up

			buf = new T[size];
		}
		else
			size = 0;
	}

    // Modified to use atomic variable methods. - rclui
	void clear()
	{
		//read_pos = write_pos = 0;
        read_pos.store(0);
        write_pos.store(0, std::memory_order_release);
	}

	/* Indicate that n elements have been written. */
    // Modified to use atomic variable methods. - rclui
	void advance_write_pointer( int n )
	{
		//write_pos = (write_pos + n) % size;
        // The below is release, because we want to synchronize it with the thread that is reading.
        write_pos.store((write_pos.load(std::memory_order_relaxed) + n) % size, std::memory_order_release);
	}

	/* Indicate that n elements have been read. */
    // Modified to use atomic variable methods. - rclui
	void advance_read_pointer( int n )
	{
		//read_pos = (read_pos + n) % size;
        
        // The below is release, because we want to synchronize it with the thread that is writing.
        read_pos.store((read_pos.load(std::memory_order_relaxed) + n) % size, std::memory_order_release);
	}

    // Modified to use atomic variable methods. - rclui
	void get_write_pointers( T *pPointers[2], unsigned pSizes[2] )
	{
        // Is doing the following enough? - rclui
        // We want to write as fast as possible, hence the following is relaxed.
        const int wpos = write_pos.load(std::memory_order_relaxed);
        
        // Need to make sure data is written before it is read, so this is acquire.
        // This synchronizes with the release in advance_read_pointer(), which is called in the reading thread.
		const int rpos = read_pos.load(std::memory_order_acquire); 

		if( rpos <= wpos )
		{
			// The buffer looks like "eeeeDDDDeeee" or "eeeeeeeeeeee" (e = empty, D = data).
			pPointers[0] = buf+wpos;
			pPointers[1] = buf;

			pSizes[0] = size - wpos;
			pSizes[1] = rpos;
		}
		else if( rpos > wpos )
		{
			// The buffer looks like "DDeeeeeeeeDD" (e = empty, D = data).
			pPointers[0] = buf+wpos;
			pPointers[1] = nullptr;

			pSizes[0] = rpos - wpos;
			pSizes[1] = 0;
		}
        

		/* Subtract the blocksize, to account for the element that we never fill
		 * while keeping the entries aligned to m_iBlockSize. */
		if( pSizes[1] )
			pSizes[1] -= m_iBlockSize;
		else
			pSizes[0] -= m_iBlockSize;
	}

	/* Like get_write_pointers, but only return the first range available. */
	T *get_write_pointer( unsigned *pSizes )
	{
		T *pBothPointers[2];
		unsigned iBothSizes[2];
		get_write_pointers( pBothPointers, iBothSizes );
		*pSizes = iBothSizes[0];
		return pBothPointers[0];
	}

    // Modified to use atomic variable methods. - rclui
	void get_read_pointers( T *pPointers[2], unsigned pSizes[2] )
	{
        // Is the following enough? - rclui
        // Read as fast as possible, hence this is relaxed.
		const int rpos = read_pos.load(std::memory_order_relaxed); 
        
        // Make sure data is read before it is written, hence acquire here.
		// This synchronizes with the release in advance_write_pointer(), which is called in the writing thread.
		const int wpos = write_pos.load(std::memory_order_acquire); 

		if( rpos < wpos )
		{
			/* The buffer looks like "eeeeDDDDeeee" (e = empty, D = data). */
			pPointers[0] = buf+rpos;
			pPointers[1] = nullptr;

			pSizes[0] = wpos - rpos;
			pSizes[1] = 0;
		}
		else if( rpos > wpos )
		{
			/* The buffer looks like "DDeeeeeeeeDD" (e = empty, D = data). */
			pPointers[0] = buf+rpos;
			pPointers[1] = buf;

			pSizes[0] = size - rpos;
			pSizes[1] = wpos;
		}
		else
		{
			/* The buffer looks like "eeeeeeeeeeee" (e = empty, D = data). */
			pPointers[0] = nullptr;
			pPointers[1] = nullptr;

			pSizes[0] = 0;
			pSizes[1] = 0;
		}
	}

	/* Write buffer_size elements from buffer into the circular buffer object,
	 * and advance the write pointer.  Return the number of elements that were
	 * able to be written.  If
	 * the data will not fit entirely, as much data as possible will be fit
	 * in. */
	unsigned write( const T *buffer, unsigned buffer_size )
	{
		using std::min;
		using std::max;
		T *p[2];
		unsigned sizes[2];
		get_write_pointers( p, sizes );

		unsigned max_write_size = sizes[0] + sizes[1];
		if( buffer_size > max_write_size )
			buffer_size = max_write_size;

		const int from_first = min( buffer_size, sizes[0] );
		std::memcpy( p[0], buffer, from_first*sizeof(T) );
		if( buffer_size > sizes[0] )
			std::memcpy( p[1], buffer+from_first, max(buffer_size-sizes[0], 0u)*sizeof(T) );

		advance_write_pointer( buffer_size );

		return buffer_size;
	}

	/* Read buffer_size elements into buffer from the circular buffer object,
	 * and advance the read pointer.  Return the number of elements that were
	 * read.  If buffer_size elements cannot be read, as many elements as
	 * possible will be read */
	unsigned read( T *buffer, unsigned buffer_size )
	{
		using std::max;
		using std::min;
		T *p[2];
		unsigned sizes[2];
		get_read_pointers( p, sizes );

		unsigned max_read_size = sizes[0] + sizes[1];
		if( buffer_size > max_read_size )
			buffer_size = max_read_size;

		const int from_first = min( buffer_size, sizes[0] );
		std::memcpy( buffer, p[0], from_first*sizeof(T) );
		if( buffer_size > sizes[0] )
			std::memcpy( buffer+from_first, p[1], max(buffer_size-sizes[0], 0u)*sizeof(T) );

		/* Set the data that we just read to 0xFF.  This way, if we're passing pointers
		 * through, we can tell if we accidentally get a stale pointer. */
		std::memset( p[0], 0xFF, from_first*sizeof(T) );
		if( buffer_size > sizes[0] )
			std::memset( p[1], 0xFF, max(buffer_size-sizes[0], 0u)*sizeof(T) );

		advance_read_pointer( buffer_size );
		return buffer_size;
	}
};

#endif

/*
 * Copyright (c) 2004 Glenn Maynard
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, and/or sell copies of the Software, and to permit persons to
 * whom the Software is furnished to do so, provided that the above
 * copyright notice(s) and this permission notice appear in all copies of
 * the Software and that both the above copyright notice(s) and this
 * permission notice appear in supporting documentation.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF
 * THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS
 * INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

