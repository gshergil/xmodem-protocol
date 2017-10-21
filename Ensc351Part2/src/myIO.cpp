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
// Helpers: None
//
// Also, list any resources beyond the course textbooks and the course pages on Piazza
// that you used in making your submission.
//
// Resources: None
//
// Contains commented out static array version.
//

/* Wrapper functions for ENSC-351, Simon Fraser University, By
 *  - Craig Scratchley
 * 
 * These functions may be re-implemented later in the course.
 */

#include <unistd.h>			// for read/write/close
#include <fcntl.h>			// for open/creat
#include <sys/socket.h> 		// for socketpair
#include "SocketReadcond.h"
#include <mutex>
#include <condition_variable>
#include <vector>

using namespace std;

struct myCondition
{
    mutex m;
    condition_variable cV;
    int buffered = 0;
    int pairNum = 0; // Your other socket pair's descriptor.
    bool isSocket = true;
};

vector<myCondition*> myVector;
mutex vectorMutex;

myCondition oneDBuffer[10]; // Array of one directional "buffer" objects.

int myReadcond(int des, void * buf, int n, int min, int time, int timeout);

int mySocketpair( int domain, int type, int protocol, int des[2] )
{
	int returnVal = socketpair(domain, type, protocol, des);
    int largeDes = 0;
    
    // Lock the vector.
    lock_guard<mutex> lk(vectorMutex);
    
    // Get the largest descriptor.
    if (des[0] > des[1])
        largeDes = des[0];
    else
        largeDes = des[1];
    
    // Resize the vector if it's not large enough.
    if (myVector.size() < (unsigned)(largeDes + 1))
        myVector.resize(largeDes + 1);
    
    // Create the objects, and make it so each direction knows its other direction pair.
    myVector[des[0]] = new myCondition;
    myVector[des[1]] = new myCondition;
    
    //oneDBuffer[des[0]] = myCondition;
    //oneDBuffer[des[0]].pairNum = des[1]; 
    myVector[des[0]]->pairNum = des[1];
    
    //oneDBuffer[des[1]] = myCondition;
    //oneDBuffer[des[1]].pairNum = des[0];
    myVector[des[1]]->pairNum = des[0];
    
	return returnVal;
}

int myOpen(const char *pathname, int flags, mode_t mode)
{
    int des = open(pathname, flags, mode);
    
    //1. Obtain vector mutex
    lock_guard<mutex> lgLock(vectorMutex);
    
    // Resize the vector if needed.
    if (myVector.size() < (unsigned)(des + 1))
        myVector.resize(des + 1);
    
    // Create object, but specify that it is not a socket.
    myVector[des] = new myCondition;
    myVector[des]->isSocket = false;
    
    return des;
}

int myCreat(const char *pathname, mode_t mode)
{
	int des = creat(pathname, mode);
    
    //1. Obtain vector mutex
    lock_guard<mutex> lgLock(vectorMutex);
    
    // Resize the vector if needed.
    if (myVector.size() < (unsigned)(des + 1))
        myVector.resize(des + 1);
    
    // Create object, but specify that it is not a socket.
    myVector[des] = new myCondition;
    myVector[des]->isSocket = false;
    
    return des;
}

ssize_t myRead( int fildes, void* buf, size_t nbyte )
{
	//return myReadcond(fildes, buf, nbyte, 1, 0, 0);
    //return read(fildes, buf, nbyte );

	// Get the other descriptor
	//int otherDescriptor = oneDBuffer[fildes].pairNum;
    int otherDescriptor = myVector[fildes]->pairNum;
    
	int bytesRead = 0;

	// Step 1. Do the read from the originating socket first, and store bytes read.
	bytesRead = read(fildes, buf, nbyte);

	// Step 2. Lock the mutex.  Make sure you lock the right one.
	//lock_guard<mutex> lgLock(oneDBuffer[otherDescriptor].m);
    lock_guard<mutex> lgLock(myVector[otherDescriptor]->m);

	// Step 3. Decrement the buffer counter.
	//oneDBuffer[otherDescriptor].buffered -= bytesRead;
    myVector[otherDescriptor]->buffered -= bytesRead;

	// Step 4. If it's 0, notify your other thread.
	// if (oneDBuffer[otherDescriptor].buffered == 0)
	// {
	//	oneDBuffer[otherDescriptor].cV.notify_one();
	// }
    if (myVector[otherDescriptor]->buffered == 0)
	{
		myVector[otherDescriptor]->cV.notify_one();
	}

	// Step 5. Return and unlock (on out of scope)
	return bytesRead;
}

ssize_t myWrite( int fildes, const void* buf, size_t nbyte )
{
    int bytesWritten = 0;
    
    // Step 1. Acquire the lock.
    //lock_guard<mutex> lgLock(oneDBuffer[fildes].m);
    lock_guard<mutex> lgLock(myVector[fildes]->m);

    // Step 2. Do the write.
	bytesWritten = write(fildes, buf, nbyte );
    
    // Step 3. Increment the buffer counter.
    //oneDBuffer[fildes].buffered += bytesWritten;
	myVector[fildes]->buffered += bytesWritten;
    
    // Step 4/5. Return and unlock (on out of scope).
    return bytesWritten;
}

int myClose( int fd )
{
    int des = close(fd);
    int otherDescriptor = 0;
    
    unique_lock<mutex> ulVector(vectorMutex);
    
    if (!(myVector[fd]->isSocket))
    {
        delete myVector[fd];
        return des;
    }
    else
    {
        // Get two mutexes.
        //unique_lock<mutex> ulCurrentFD(myVector[des]->m, defer_lock);
        //otherDescriptor = myVector[fd]->pairNum;
        //unique_lock<mutex> ulOtherFD(myVector[otherDescriptor]->m, defer_lock);
        
        // Lock both at the same time.
        //lock(ulCurrentFD, ulOtherFD);
        //ulCurrentFD.lock();
        
        // Notify all other threads.
        //myVector[des]->cV.notify_all();
        //myVector[otherDescriptor]->cV.notify_all();
        
        // Unlock so other threads can wake up successfully.
        //ulCurrentFD.unlock();
        //ulOtherFD.unlock();
        
        // Relock and delete.
        //lock(ulCurrentFD, ulOtherFD);
        //ulCurrentFD.lock();
        
        delete myVector[des];
        myVector[des] = NULL;
        delete myVector[otherDescriptor];
        myVector[otherDescriptor] = NULL;
        
        return des;
    }
}

int myTcdrain(int des)
{ //is also included for purposes of the course.
    // Step 1. Acquire the lock. Make sure to use unique 
    // because we have condition variable.
    //unique_lock<mutex> uLock(oneDBuffer[des].m);
    unique_lock<mutex> uLock(myVector[des]->m);
    
    // Step 2. Wait for the condition variable.
    //oneDBuffer[des].cV.wait(uLock, [des] { return oneDBuffer[des].buffered == 0; });
    myVector[des]->cV.wait(uLock, [des] { return myVector[des]->buffered == 0; });
    
    // Step 3. Unlock and continue your way.
   	return 0;
}

int myReadcond(int des, void * buf, int n, int min, int time, int timeout)
{

    // Get the other descriptor
    //int fildes = oneDBuffer[des].pairNum;
    int fildes = myVector[des]->pairNum;
    int bytesRead = 0;

    // Step 1. Do the read first from the originating socket, because wcsReadcond can block, and store bytes read.
    bytesRead = wcsReadcond(des, buf, n, min, time, timeout );
    
    // Step 2. Lock the mutex.  Make sure you lock the right one.
    //lock_guard<mutex> lgLock(oneDBuffer[fildes].m);
    lock_guard<mutex> lgLock(myVector[fildes]->m);
    
    // Step 3. Decrement the buffer counter.
    //oneDBuffer[fildes].buffered -= bytesRead;
    myVector[fildes]->buffered -= bytesRead;
    
    // Step 4. If it's 0, notify your other thread.
    //if (oneDBuffer[fildes].buffered == 0)
    //{
    //    oneDBuffer[fildes].cV.notify_one();
    //}
    if (myVector[fildes]->buffered == 0)
    {
        myVector[fildes]->cV.notify_one();
    }
    
    // Step 5. Return and unlock (on out of scope)
    return bytesRead;
}

