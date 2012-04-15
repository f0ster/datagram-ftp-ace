#ifndef _FTP_H
#define _FTP_H
// ftp mega_header
#include <iostream>
#include <stdio.h>
#include <cstdlib>
#include <cstring>


using namespace std;

extern int	traceflag;	/* -t command line option, or "trace" cmd */
#define	ERRORLOG	"/tmp/rfp.log"
#define	DEBUG1(fmt, arg1)	if (traceflag) { \
					fprintf(stderr, fmt, arg1); \
					fputc('\n', stderr); \
					fflush(stderr); \
				} else ;
#define	DEBUG2(fmt, arg1, arg2)	if (traceflag) { \
					fprintf(stderr, fmt, arg1, arg2); \
					fputc('\n', stderr); \
					fflush(stderr); \
				} else ;
				

typedef unsigned short u_int16;
typedef  unsigned char u_int8;
typedef unsigned long int u_int32;

#define ACK_TYPE 'a'
#define DATA_TYPE 'd'
#define HAND_TYPE 'h'


class packet{
	public:
		packet() {
			//cout << "default packet constructor " << endl;
			this->payload = (u_int8 *)malloc(1);
			this->seqNum = 0;
			this->type= ACK_TYPE;
			this->len = 1;
			this->payload[0] = 0x00;
			this->CRC =0; // never need to set
		};
		~packet() {
			//cout << "deconstucting  packet" << endl;
			free(this->payload);
		};
		packet(const packet &p);
		packet &operator=(const packet &p);
	
		u_int16 seqNum;
		u_int8 type; // a, d, h
		u_int32 len;
		u_int8* payload; 
		u_int16 CRC; 
		friend ostream& operator << (ostream& os, const packet& p);
};

ostream& operator << (ostream& os, const packet& p){
	return os <<"Packet: seqNumber: "<< p.seqNum << " Type: " << p.type << " Len: " << p.len << " CRC: " << p.CRC << " &payload: ";
	
}


packet::packet(const packet &p){
	//cout << "packet copy con " << endl;
	this->seqNum = p.seqNum;
	this->type = p.type;
	this->len = p.len;
	this->CRC = p.CRC;
	this->payload = (u_int8 *)malloc(p.len);
	for(unsigned int i = 0; i < this->len; ++i){
		this->payload[i] = p.payload[i];
	}
}

packet &packet::operator=(const packet &p){
	//cout << "packet operator= " << endl << p << endl;
	this->seqNum = p.seqNum;
	this->type = p.type;
	this->len = p.len;
	this->CRC = p.CRC;
	this->payload = (u_int8 *)malloc(this->len);
	for(unsigned int i = 0; i < this->len; ++i){
		this->payload[i] =p.payload[i];; // this seq faults
	}
	return (*this);
		///return (*new packet(p));
}

#define WIDTH  (8 * sizeof(u_int8))
#define TOPBIT (1 << (WIDTH - 1))
#define POLYNOMIAL 0xD8  /* 11011 followed by 0's */
u_int8  crcTable[256];

void crcInit()
{
    u_int16  remainder;
     //Compute the remainder of each possible dividend.
    for (int dividend = 0; dividend < 256; ++dividend)
    {
         //Start with the dividend followed by zeros.
        remainder = dividend << (WIDTH - 8);
         //Perform modulo-2 division, a bit at a time.
        for (u_int8 bit = 8; bit > 0; --bit)
        {
            //Try to divide the current data bit.
            if (remainder & TOPBIT) 
                remainder = (remainder << 1) ^ POLYNOMIAL;
            else
                remainder = (remainder << 1);
        }
        //Store the result into the table
        crcTable[dividend] = remainder;
    }
}   /* crcInit() */

u_int16 crcFast(u_int8 const *message, int nBytes)
{
    u_int8 data;
    u_int16 remainder = 0;
	
     //Divide the message by the polynomial, a byte at a time.
    for (int byte = 0; byte < nBytes; ++byte)
    {
        data = message[byte] ^ (remainder >> (WIDTH - 8));
        remainder = crcTable[data] ^ (remainder << 8);
    }
     //The final remainder is the CRC.

    return (remainder);
}

int crcCheck(packet p){
	u_int8* data = (u_int8*) malloc( (p.len + 7) * sizeof(char)); 
	data[0] = (p.seqNum >> 8)& 0xff;	
	data[1] = (p.seqNum)& 0xff;;
	data[2] = p.type;
	data[3] = (p.len >> 24) & 0xff;
	data[4] = (p.len >> 16) & 0xff;
	data[5] = (p.len >> 8) & 0xff;
	data[6] = p.len & 0xff;
	unsigned int i;
	for (i = 0; i < p.len; ++i){
		data[7+i] = p.payload[i];
	}
	u_int16 result = crcFast(data,p.len + 7);
	free(data);
	return p.CRC == result;
}



u_int8 * pack(u_int16 seqNum, u_int8 type, u_int32 len, u_int8* payload)
{
	u_int8* data = (u_int8*) malloc( (len + 9) * sizeof(u_int8)); 
	data[0] = (seqNum >> 8)& 0xff;	
	data[1] = (seqNum)& 0xff;;
	data[2] = type;
	data[3] = (len >> 24) & 0xff;
	data[4] = (len >> 16) & 0xff;
	data[5] = (len >> 8) & 0xff;
	data[6] = len & 0xff;
	unsigned int i;
	for (i = 0; i < len; ++i){
		data[7+i] = payload[i];
	}
	u_int8* upToCRC = (u_int8*)malloc( (7+len)*sizeof(u_int8) );
	memcpy(upToCRC,data,7+len);
	u_int16 CRC = crcFast( upToCRC, 7+len  );
	data[7+len] = (CRC >> 8) &0xff ;
	data[8+len] = (CRC) &0xff ;
	return data;
}

u_int8 * pack(packet data){ return pack(data.seqNum, data.type, data.len, data.payload); }


packet unpack(u_int8* in)
{
	packet data;
	data.seqNum = ((u_int16)in[0] << 8) | (u_int16)in[1];
	data.type = in[2];
	data.len = (((u_int32)in[3]& 0xff) << 24) | (((u_int32)in[4]& 0xff) << 16) | (((u_int32)in[5]& 0xff) << 8) | ((u_int32)in[6]& 0xff);	
		free(data.payload);
	data.payload = (u_int8*) malloc( data.len * sizeof(char)); 
	for (unsigned int i = 0; i < data.len; i++){
		data.payload[i] =in [i+7];
	}
	data.CRC = (in[7+data.len] << 8) | in[8+data.len];
	
	return data;
}
#endif
	
	
	
	
	
	

