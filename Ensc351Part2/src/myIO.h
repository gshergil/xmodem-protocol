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
// Note: Nothing changed in this file.


#ifndef MYSOCKET_H_
#define MYSOCKET_H_

#include <unistd.h>
#include <sys/stat.h>

/*
int myCreat(const char *pathname, mode_t mode);
ssize_t myRead( int fildes, void* buf, size_t nbyte );
ssize_t myWrite( int fildes, const void* buf, size_t nbyte );
int myClose(int fd);
*/

/* int myOpen( const char * path,
          int oflag,
          ... )
; */
int myOpen(const char *pathname, int flags, mode_t mode);

int myCreat(const char *pathname, mode_t mode);
int mySocketpair( int domain, int type, int protocol, int des[2] );
ssize_t myRead( int des, void* buf, size_t nbyte );
ssize_t myWrite( int des, const void* buf, size_t nbyte );
int myClose(int des);
// The last two are not ordinarily used with sockets
int myTcdrain(int des); //is also included for purposes of the course.
int myReadcond(int des, void * buf, int n, int min, int time, int timeout);

#endif /*MYSOCKET_H_*/
