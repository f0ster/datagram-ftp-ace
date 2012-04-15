#ifndef WINDOW_H
#define WINDOW_H

#include "ftp.h"
#include "ace/ACE.h"
#include "ace/FILE_IO.h"
#include "ace/FILE_Addr.h"
#include "ace/FILE_Connector.h"

// frame
//sender
const char ACKED = 'a';  // already Acked
const char NOTACKED = 'n'; // sent, not yet acked
const char READY = 'r';  //  useable, not sent

const char NOTUSEABLE = 'u'; // not useable (frame outside of window)
//revc
const char OUTOFORDER = 'o'; // out of order(buffered) but already acked
const char ACCEPTABLE = 'c'; // within window
const char EXPECTED = 'e'; //  not yet received

//window
const char GNB = 'g';
const char STOP = 's';
const char REPEAT = 'p';

const char SEND = 'd';
const char RECV = 'v';

#define infile "input"
#define outfile "outfile"



class frame{
	protected:
		
		  // must keep track of on sender side , absolute sequence #
	public:
		frame() {
			//cout << "default frame constructor" << endl;
			this->status = NOTUSEABLE;
		};
		~frame() {
			//free(data.payload);
		};
		
		frame(const frame &f);
		frame &operator=(const frame &f);
		
		char status;
		packet data;
		friend ostream& operator << (ostream& os, const frame& p);
};

ostream& operator << (ostream& os, const frame& p){
	return os <<"Frame: Status: "<< p.status << "data: " << p.data ;
}

frame::frame(const frame &f){
	this->status = f.status;
	this->data = f.data;
}

frame &frame::operator=(const frame &f){
	//cout << "frame operator= " << endl << f << endl;
	this->status = f.status;
	//cout << "incoming packet in frame operator= " << f.data << endl;
	this->data = f.data;
	return (*this);
}

class window{
	protected:
		
		
	private:
	
	public:
		frame *frames;
		//recv
		int largestFrameAcceptable;  // LFA
		int lastFrameReceived;  // LFR
		//send
		int lastAcknowledgmentReceived;
		int lastFrameSent;
		
		int MaxSegNum;
		int packetNum;
		int winSize;
		char side;
		char type;
		unsigned int payloadLen;
		int notInit;
		
		window(){
			this->notInit =1;
			printf("in default const : win\n");
			(*this).MaxSegNum = 1;
			(*this).winSize = 1;
			(*this).type = 0;
			(*this).side = SEND;
			(*this).lastAcknowledgmentReceived = -1;
			(*this).lastFrameSent = 0;
			(*this).largestFrameAcceptable = 1;
			(*this).lastFrameReceived = 0;
			this->packetNum = 0;
			ACE_FILE_Addr inFile(infile);
		}
		window (char si){
			printf("in default const(si) : win\n");
			this->notInit = 1;
			(*this).MaxSegNum = 1;
			(*this).winSize = 1;
			(*this).type = 0;
			(*this).side = si;
			(*this).lastAcknowledgmentReceived = -1;
			(*this).lastFrameSent = 0;
			(*this).largestFrameAcceptable = 1;
			(*this).lastFrameReceived = 0;
			this->packetNum = 0;

		};
		
		window(int MSN, int s, char t, char si){
			this->notInit = 1;
			(*this).MaxSegNum = MSN;
			(*this).winSize = s;
			(*this).frames = (frame *) malloc( this->MaxSegNum * sizeof(frame) );
			(*this).type = t;
			(*this).side = si;
			switch(this->side){
				case RECV:
					this->largestFrameAcceptable = this->winSize;
					this->lastFrameReceived = 0;
				break;
				case SEND:
					this->lastAcknowledgmentReceived = -1;
					this->lastFrameSent = this->winSize;
				break;
				default:break;
			} 
			this->packetNum = 0;
			 //firstACK = true;
		};
		
		
		~window() {
				for(int i=0; i< MaxSegNum && !notInit; ++i){
					free(frames[i].data.payload);
				}
				//free(frames);
			};
		
		window(const window &w);
		window &operator=(const window &w);
		
		void setPayloadLen(int len){
			this->payloadLen = len;
		};
		
		int slide(int dist);
		int getSlideDist();
		friend ostream& operator << (ostream& os, const frame& p);
		void init();
		packet nextPacket();
};

ostream& operator << (ostream& os, const window& p){
	if(!p.notInit){
		os << "[ ";
		switch(p.side){		
			case RECV:
			for(int i=p.lastFrameReceived; i != p.largestFrameAcceptable; i++) {
				 i = i % p.MaxSegNum;
				/*if(p.frames[i].status ==  OUTOFORDER)
					os << p.frames[i].data.seqNum << " ,";
				else
					os << p.frames[i].status << " ,";*/
				os << p.frames[i].data.seqNum << " : " << p.frames[i].status << " ,";
			}
			break;
			case SEND:
			for(int i=p.lastAcknowledgmentReceived + 1; i != p.lastFrameSent; i++) {
				i = i % p.MaxSegNum;
				os << p.frames[i].data.seqNum << " : " << p.frames[i].status << " ,";
			}
			break;
			default:break;
		}
		os << "]";
	}
	return os;
}

window::window(const window &w){
	this->largestFrameAcceptable = w.largestFrameAcceptable;
	this->lastFrameReceived = w.lastFrameReceived;
	this->lastAcknowledgmentReceived = w.lastAcknowledgmentReceived;
	this->lastFrameSent = w.lastFrameSent;
	this->MaxSegNum = w.MaxSegNum;
	this->packetNum = w.packetNum;
	this->winSize = w.winSize;
	this->payloadLen = w.payloadLen;
	this->notInit = w.notInit;
	this->type = w.type;
	this->side = w.side;
	if (!this->notInit){
		this->frames = new frame[this->MaxSegNum];
		for(int i = 0; i < MaxSegNum; ++i){
			this->frames[i] = w.frames[i];
		}
	}
	cout << "copy constructor "<< endl<< *this << endl;
}

window &window::operator=(const window &w){
	this->largestFrameAcceptable = w.largestFrameAcceptable;
	this->lastFrameReceived = w.lastFrameReceived;
	this->lastAcknowledgmentReceived = w.lastAcknowledgmentReceived;
	this->lastFrameSent = w.lastFrameSent;
	this->MaxSegNum = w.MaxSegNum;
	this->packetNum = w.packetNum;
	this->winSize = w.winSize;
	this->payloadLen = w.payloadLen;
	this->notInit = w.notInit;
	this->type = w.type;
	this->side = w.side;
	if (!this->notInit){
		this->frames = new frame[this->MaxSegNum];
		for(int i = 0; i < MaxSegNum; ++i){
			this->frames[i] = w.frames[i];
		}
	}
	cout << "copy constructor= "<< endl<< *this << endl;
	return (*this);
}

void window::init(){
	if(this->notInit){
		cout << "----------INIT----------" << endl;
		this->frames = new frame[this->MaxSegNum];
		switch(this->side){
			case RECV:
				this->largestFrameAcceptable = this->winSize;
				this->lastFrameReceived = 0;
				
				for(int i = 0; i < this->MaxSegNum; ++i){
					packet newP;
					this->frames[i].data = newP;
					this->frames[i].status = EXPECTED;
				}
			break;
			case SEND:
				this->lastAcknowledgmentReceived = 0;
				this->lastFrameSent =this->winSize;
				for(int i = 0; i < this->MaxSegNum; ++i){
					this->frames[i].data = nextPacket();
					this->frames[i].status = READY;
				}
				
			break;
			default:break;
		}
		cout << "----------INIT----------" << endl;
	}
	this->notInit = 0;
}

int window::slide(int dist) { 

	
	ACE_FILE_IO file;
	ACE_FILE_Connector conn;
	ACE_FILE_Addr oFile(outfile);
	int frameIndex =0;
	packet newPacket;
					
	switch(this->side){
	
	
		case RECV:
			//cout << "sliding window" << endl;
			frameIndex =(*this).lastFrameReceived;
			
			// start sliding from LFR to dist
			for(int i = 0; i < dist; i++){
				
				// write the packet out then slide
				conn.connect(file , oFile,0, ACE_Addr::sap_any, 0, O_RDWR|O_CREAT|O_APPEND , ACE_DEFAULT_FILE_PERMS);
				file.send(this->frames[frameIndex + i].data.payload,this->frames[frameIndex + i].data.len);
				file.dump();
				frame newFrame;
				newFrame.status = EXPECTED;
				newPacket.seqNum = i + frameIndex;
				newPacket.type = ACK_TYPE;
				newFrame.data = newPacket; // this is the new packet
				// now slide window info over one ->
				(*this).frames[(frameIndex + i + (*this).winSize) % (*this).MaxSegNum] = newFrame;
				(*this).largestFrameAcceptable += 1;
				(*this).lastFrameReceived +=1; 
			}
		break;
		case  SEND:
			/*this needs to be looked at*/
				
			/* this io is not righ yet fyi */

			frameIndex = this->lastAcknowledgmentReceived + 1;
			for(int i = 0; i < dist; i++){

				free(this->frames[i].data.payload);
				this->frames[i].data = nextPacket();
				this->frames[i].status = READY;
				this->lastFrameSent = (this->lastFrameSent + 1) % this->MaxSegNum;
				//this->lastAcknowledgmentReceived ++;
			}
			file.dump();
			break;
			default:break;
	}
	return this->payloadLen;  // not sure if this where this should be done  ( if this->payloadLen == 0 then eof)
}
int window::getSlideDist(){
	int count = 0;
	switch(this->side){
		case RECV:
		for(int i=lastFrameReceived; i != largestFrameAcceptable; i++){
			i = i % this->MaxSegNum;
			if( this->frames[i].status==OUTOFORDER )
				count++;
		}
		break;
		case SEND:
		for(int i=lastAcknowledgmentReceived + 1; i != lastFrameSent; i++) {
			//cout << "i=lastAcknowledgmentReceived + 1 = " << i << " lastFrameSent " << lastFrameSent<< endl;
			i = i %this->MaxSegNum;
		//cout << "frames[i].status slide? : " << this->frames[i].status << " looking for " << ACKED <<endl;
			if( this->frames[i].status==ACKED )
				count++;
		}
		break;
		default:break;
	}
	return count;
}

packet window::nextPacket(){ // data packet only
	
	ACE_FILE_IO file;
	ACE_FILE_Addr inFile(infile);
	ACE_FILE_Connector conn;
	
	if (conn.connect(file , inFile, 0, ACE_Addr::sap_any, 0, O_RDWR , ACE_DEFAULT_FILE_PERMS) == -1)  // read the file 
	{
		ACE_DEBUG ((LM_DEBUG, "no input file.\n"));
		exit(1);
	}
	packet p;
	p.seqNum = this->packetNum % this->MaxSegNum;
	p.type = DATA_TYPE;
	file.seek((this->packetNum* this->payloadLen), SEEK_SET);
	p.payload = (u_int8*)realloc(p.payload,this->payloadLen);
	p.len = file.recv(p.payload,this->payloadLen);

	this->packetNum++;
	
	file.dump();
	//cout << "made packet :: " <<  p << endl;
	return p;
}

#endif















