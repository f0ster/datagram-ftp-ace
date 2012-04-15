#ifndef _TEST_PACKET_H
#define _TEST_PACKET_H


#include <stdio.h>
#include <cstdlib>
#include <cstring>

#include "ftp.h"

int main(){
	
	int winSize = 300;
	int payloadLen = 30000;
	crcInit();
	packet data;
	data.seqNum = 261;
	data.type = ACK_TYPE;
	data.len  = 6;
	data.payload = (u_int8*)calloc(data.len,sizeof(char));
	
	data.payload[0] = (winSize >> 8) & 0xff;
	data.payload[1] = (winSize) & 0xff;
	data.payload[2] = (payloadLen >> 24) & 0xff;
	data.payload[3] = (payloadLen >> 16) & 0xff;
	data.payload[4] = (payloadLen >> 8) & 0xff;
	data.payload[5] = payloadLen  & 0xff;
	
	packet newP = unpack(pack(data));
	
	cout << data <<endl;
	cout << newP <<endl;

		
	winSize = (newP.payload[0] << 8) | newP.payload[1] ;


	payloadLen = newP.payload[2];
	payloadLen = ( payloadLen << 8) | newP.payload[3];
	payloadLen = ( payloadLen << 8) | newP.payload[4];
	payloadLen = ( payloadLen << 8) | newP.payload[5];
	
	cout << winSize << endl;
	cout << payloadLen << endl;
	cout << crcCheck(newP) <<endl;
	
	return 0;
}
#endif
