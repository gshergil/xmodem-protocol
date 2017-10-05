//============================================================================
//
//% Student Name 1: student1
//% Student 1 #: 123456781
//% Student 1 userid (email): stu1 (stu1@sfu.ca)
//
//% Student Name 2: student2
//% Student 2 #: 123456782
//% Student 2 userid (email): stu2 (stu2@sfu.ca)
//
//% Below, edit to list any people who helped you with the code in this file,
//%      or put 'None' if nobody helped (the two of) you.
//
// Helpers: _everybody helped us/me with the assignment (list names or put 'None')__
//
// Also, list any resources beyond the course textbooks and the course pages on Piazza
// that you used in making your submission.
//
// Resources:  ___________
//
//%% Instructions:
//% * Put your name(s), student number(s), userid(s) in the above section.
//% * Also enter the above information in other files to submit.
//% * Edit the "Helpers" line and, if necessary, the "Resources" line.
//% * Your group name should be "P2_<userid1>_<userid2>" (eg. P1_stu1_stu2)
//% * Form groups as described at:  https://courses.cs.sfu.ca/docs/students
//% * Submit files to courses.cs.sfu.ca
//
// File Name   : ReceiverX.cpp
// Version     : September 3rd, 2017
// Description : Starting point for ENSC 351 Project Part 2
// Original portions Copyright (c) 2017 Craig Scratchley  (wcs AT sfu DOT ca)
//============================================================================

#include <string.h> // for memset()
#include <fcntl.h>
#include <stdint.h>
#include <iostream>
#include "myIO.h"
#include "ReceiverX.h"
#include "VNPE.h"

//using namespace std;

ReceiverX::
ReceiverX(int d, const char *fname, bool useCrc)
:PeerX(d, fname, useCrc), goodBlk(false), goodBlk1st(false), numLastGoodBlk(0)
{
	NCGbyte = useCrc ? 'C' : NAK;
}

void ReceiverX::receiveFile()
{
	mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
	// should we test for error and set result to "OpenError" if necessary?
	transferringFileD = PE2(creat(fileName, mode), fileName);

	// ***** improve this member function *****
	enum  { START, CRC, RECEIVE_FIRST, BLKNUM255, BLKNUM, VERIFY, EOT1, CAN1, DONE}; //fix
	int nextState = 0;

	bool done = false;
	//uint8_t crcCounter = 0;

	while(!done){
		switch(nextState) {
			case START: {
				//crcCounter = 0;
				numLastGoodBlk = 0;
				if (Crcflg)
				{
					sendByte('C');
				}
				else
				{
					sendByte(NAK);
				}
				nextState = RECEIVE_FIRST;
			}break;	//end case START
			case RECEIVE_FIRST: {
				//PE_NOT(myRead(mediumD, rcvBlk, 1), 1);	//Read first byte
				PE_NOT(myReadcond(mediumD, rcvBlk, 1, 1, 0,0),1);
				if (rcvBlk[0] == EOT)
				{
					sendByte(NAK);
					nextState = EOT1;
				}
				else if (rcvBlk[0] == CAN || errCnt > 11)
				{
					sendByte(CAN);
					nextState = CAN1;
				}
				else //rcvBlk[0] == SOH
				{
					getRestBlk();
					nextState = BLKNUM255;
				}
			}break; //end case RECEIVE_FIRST
			case BLKNUM255: {
				if (rcvBlk[1]+rcvBlk[2] != 255)
				{
					sendByte(NAK);
					errCnt++;
					nextState = RECEIVE_FIRST;
				}
				else
				{
					nextState = BLKNUM;
				}
			} break; //end case BLKNUM255
			case BLKNUM: {
				if ( rcvBlk[1] == (uint8_t)(numLastGoodBlk+1))
				{
					// if blkNum is previous blkNum or current blkNum
					nextState = VERIFY;
				}
				else if ((rcvBlk[1] == numLastGoodBlk) )
				{
					//ACK got corrupted to NAK.  Just ACK this again, and move on.
					sendByte(ACK);
					nextState = RECEIVE_FIRST;
				}
				else
				{
					can8();
					nextState = DONE;
					result = "Error: Wrong blkNum.";
				}
			}break; //end case BLKNUM
			case VERIFY: {
				uint16_t crc = 0;
				uint8_t checksum = 0;
				if (Crcflg)
				{
					crc16ns(&crc, &rcvBlk[3]);
					crc = crc << 8 | crc >> 8;
				}
				else
				{
					for (int ii=0; ii<CHUNK_SZ; ii++)
					{
						checksum += rcvBlk[DATA_POS+ii];
					}
				}

				// BRANCHING
				if ((checksum != rcvBlk[PAST_CHUNK] && !Crcflg) || (crc != (uint16_t)(rcvBlk[PAST_CHUNK]<< 8|rcvBlk[PAST_CHUNK+1]) && Crcflg))
				{
					sendByte(NAK);
					errCnt++;
				}
				else
				{
					sendByte(ACK);
					errCnt = 0;
					numLastGoodBlk++;
					writeChunk();
				}
				nextState = RECEIVE_FIRST;	//Both branches back to receive_first
			}break; //end case VERIFY
			case EOT1: {
				PE_NOT(myReadcond(mediumD, rcvBlk, 1, 1, 0,0),1);
				sendByte(ACK);
				result = "Complete";
				nextState = DONE;
			} break; //end case EOT1
			case CAN1: {
				PE_NOT(myRead(mediumD, rcvBlk, 1), 1);
				//PE_NOT(myReadcond(mediumD, rcvBlk, 1, 1, 0,0),1);
				sendByte(CAN);
				result = "Cancelled";
				nextState = DONE;
			}break; //end case CAN1
			case DONE: {
				done = true;
			}break; //end case DONE
		}	//end switch
	} //end while

	PE(close(transferringFileD));
}

/* Only called after an SOH character has been received.
The function tries
to receive the remaining characters to form a complete
block.  The member variable goodBlk will be made false if
the received block formed has something
wrong with it, like the checksum being incorrect.  The member
variable goodBlk1st will be made true if this is the first
time that the block was received in "good" condition.
*/
void ReceiverX::getRestBlk()
{
	// ********* this function must be improved ***********
	goodBlk1st = goodBlk = true;

	uint16_t crc = 0;
	uint8_t checksum = 0;

	//Read the correct length.
	if (Crcflg)
	{
		//PE_NOT(myRead(mediumD, &rcvBlk[1], REST_BLK_SZ_CRC), REST_BLK_SZ_CRC);
		PE_NOT(myReadcond(mediumD, &rcvBlk[1], REST_BLK_SZ_CRC, REST_BLK_SZ_CRC, 0, 0), REST_BLK_SZ_CRC);
	}
	else
	{
		//PE_NOT(myRead(mediumD, &rcvBlk[1], REST_BLK_SZ_CS), REST_BLK_SZ_CS);
		PE_NOT(myReadcond(mediumD, &rcvBlk[1], REST_BLK_SZ_CS, REST_BLK_SZ_CS, 0, 0), REST_BLK_SZ_CS);
	}

	//Check for correct complement.
	if (rcvBlk[1]+rcvBlk[2] != 255)
	{
		goodBlk = false;
		goodBlk1st = false;
		return;
	}

	//Check for correct blkNum
	if (rcvBlk[1] == numLastGoodBlk-1)
	{
		//Received previous blk again.
		goodBlk1st = false;
	}
	else if(!(rcvBlk[1] == numLastGoodBlk) || (rcvBlk[1] == numLastGoodBlk+1))
	{
		//Not the expected blkNum
		goodBlk = false;
		goodBlk1st = false;
		return;
	}

	//Check CRC or checksum
	if (Crcflg)
	{
		crc16ns(&crc, &rcvBlk[3]);
		crc = crc << 8 | crc >> 8;
		if (crc != (uint16_t)(rcvBlk[PAST_CHUNK]<< 8|rcvBlk[PAST_CHUNK+1]))
		{
			goodBlk = false;
			goodBlk1st = false;
		}
	}
	else
	{
		for (int ii=0; ii<CHUNK_SZ; ii++)
		{
			checksum += rcvBlk[DATA_POS+ii];
		}
		if (checksum != rcvBlk[PAST_CHUNK])
		{
			goodBlk = false;
			goodBlk1st = false;
		}
	}

}

//Write chunk (data) in a received block to disk
void ReceiverX::writeChunk()
{
	PE_NOT(write(transferringFileD, &rcvBlk[DATA_POS], CHUNK_SZ), CHUNK_SZ);
}

//Send 8 CAN characters in a row to the XMODEM sender, to inform it of
//	the cancelling of a file transfer
void ReceiverX::can8()
{
	// no need to space CAN chars coming from receiver
	const int canLen=8; // move to defines.h
    char buffer[canLen];
    memset( buffer, CAN, canLen);
    PE_NOT(myWrite(mediumD, buffer, canLen), canLen);
}

