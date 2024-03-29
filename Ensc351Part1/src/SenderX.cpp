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
// Helpers: None.
//
// Also, list any resources beyond the course textbooks and the course pages on Piazza
// that you used in making your submission.
//
// Resources:  None.
//
//%% Instructions:
//% * Put your name(s), student number(s), userid(s) in the above section.
//% * Also enter the above information in other files to submit.
//% * Edit the "Helpers" line and, if necessary, the "Resources" line.
//% * Your group name should be "P1_<userid1>_<userid2>" (eg. P1_stu1_stu2)
//% * Form groups as described at:  https://courses.cs.sfu.ca/docs/students
//% * Submit files to courses.cs.sfu.ca
//
// File Name   : SenderX.cpp
// Version     : September 3rd, 2017
// Description : Starting point for ENSC 351 Project
// Original portions Copyright (c) 2017 Craig Scratchley  (wcs AT sfu DOT ca)
//
// Revision 1 (2017-09-14):
// Part 1 of Multipart project - make sendFile(), genBlk(), and
// crc16ns() work. Used bit operations instead of htons() to avoid function
// overheads. 
//============================================================================

#include <iostream>
#include <stdint.h> // for uint8_t
#include <string.h> // for memset()
#include <errno.h>
#include <fcntl.h>	// for O_RDWR

#include "myIO.h"
#include "SenderX.h"

using namespace std;

SenderX::SenderX(const char *fname, int d)
:PeerX(d, fname), bytesRd(-1), blkNum(255)
{
}

//-----------------------------------------------------------------------------

/* tries to generate a block.  Updates the
variable bytesRd with the number of bytes that were read
from the input file in order to create the block. Sets
bytesRd to 0 and does not actually generate a block if the end
of the input file had been reached when the previously generated block was
prepared or if the input file is empty (i.e. has 0 length).
*/
void SenderX::genBlk(blkT blkBuf)
{
	// ********* The next line needs to be changed ***********
	if (-1 == (bytesRd = myRead(transferringFileD, &blkBuf[3], CHUNK_SZ )))
		ErrorPrinter("myRead(transferringFileD, &blkBuf[3], CHUNK_SZ )", __FILE__, __LINE__, errno);
	// ********* and additional code must be written ***********
	// If we reach the end, and we read less than 128 bytes, write CTRL_Zs
	// everywhere else.
	if (bytesRd != -1 && bytesRd != CHUNK_SZ)
    {
    	for(int ii=bytesRd; ii < CHUNK_SZ; ii++)
    	{
    		blkBuf[3+ii] = CTRL_Z; // As per xmodem-edited.txt
    	}

    }
	blkBuf[0] = SOH;
    blkBuf[1] = blkNum;
    blkBuf[2] = 0xFF - blkNum;

    uint8_t checksum = 0x00;

    if(Crcflg)
    {
    	crc16ns((uint16_t*)&blkBuf[3+CHUNK_SZ], &blkBuf[3]);

    }
    else
    {
    	for(int ii = 0; ii < CHUNK_SZ; ii++)
    	{
    		checksum += blkBuf[3+ii];
    	}

    	blkBuf[3+CHUNK_SZ] = checksum;
    }

}

void SenderX::sendFile()
{
	transferringFileD = myOpen(fileName, O_RDWR, 0);
	if(transferringFileD == -1) {
		cout /* cerr */ << "Error opening input file named: " << fileName << endl;
		result = "OpenError";
	}
	else {
		cout << "Sender will send " << fileName << endl;

		blkNum = 1; // but first block sent will be block #1, not #0

		// do the protocol, and simulate a receiver that positively acknowledges every
		//	block that it receives.

		// assume 'C' or NAK received from receiver to enable sending with CRC or checksum, respectively
		genBlk(blkBuf); // prepare 1st block
		while (bytesRd)
		{
			blkNum ++; // 1st block about to be sent or previous block was ACK'd

			// ********* fill in some code here to send a block ***********

			if(Crcflg)
			{
				for (int ii = 0; ii < BLK_SZ_CRC; ii++)
				{
					sendByte(blkBuf[ii]);
				}
			}
			else
			{
				for (int ii = 0; ii < BLK_SZ; ii++)
				{
					sendByte(blkBuf[ii]);
				}
			}


			// assume sent block will be ACK'd
			genBlk(blkBuf); // prepare next block
			// assume sent block was ACK'd
		};
		// finish up the protocol, assuming the receiver behaves normally
		// ********* fill in some code here ***********

        // rclui - write <eot> twice here.
		sendByte(EOT);
		sendByte(EOT);

		//(myClose(transferringFileD));
		if (-1 == myClose(transferringFileD))
			ErrorPrinter("myClose(transferringFileD)", __FILE__, __LINE__, errno);
		result = "Done";
	}
}
