#ifndef _CLIENT_H
#define _CLIENT_H

#include "window.h"
#include "/usr/local/ACE/ace/OS_main.h"
#include "/usr/local/ACE/ace/Reactor.h"
#include "/usr/local/ACE/ace/SOCK_Dgram.h"
#include "/usr/local/ACE/ace/INET_Addr.h"
#include "/usr/local/ACE/ace/ACE.h"

class Client : public ACE_Event_Handler
{
public:

  virtual ~Client (void);
  Client(const ACE_INET_Addr &local, const ACE_INET_Addr &remote_addr, int pSize, char winType, int winSize, int maxSeg);
  // = Override <ACE_Event_Handler> methods.
  virtual ACE_HANDLE get_handle (void) const;
  virtual int handle_input (ACE_HANDLE);
  virtual int handle_close (ACE_HANDLE handle, ACE_Reactor_Mask close_mask);
  
  int sendPacket(packet);
  int handshake();
  int processPacket(packet);
  int sendACK(u_int16);
  
protected:

	ACE_Reactor* _reactor;
	int payloadSize;
	// packetSize = payloadSize+9;
	window* window_;
	char* writeBuffer;
	
private:
  ACE_SOCK_Dgram endpoint_;
  // Receives datagrams.
  ACE_INET_Addr remote;
  
  u_int8* currentPacket;
  
  
  ACE_UNIMPLEMENTED_FUNC (Client (void))
  ACE_UNIMPLEMENTED_FUNC (Client (const Client &))
  ACE_UNIMPLEMENTED_FUNC (Client &operator= (const Client &))
};

Client::Client (const ACE_INET_Addr &local, const ACE_INET_Addr &remote_addr, int pSize, char winType, int winSize, int maxSeg): endpoint_(local)
{

	  this->remote.set(remote_addr);
	  this->payloadSize = pSize;
	  this->window_ = new window(maxSeg,winSize,winType,RECV);
	  this->window_->init();
	  this->window_->payloadLen  = pSize; // this was never set

	 if (ACE_Reactor::instance()->register_handler(this,ACE_Event_Handler::READ_MASK) == -1) {
		ACE_ERROR ((LM_ERROR,"Client :: ACE_Reactor::register_handler() failed\n")); 
		exit(-1);
	 }
	 
	 
	 	  
	/* if (endpoint_.open( local ) == -1) {
		ACE_ERROR ((LM_ERROR,"Client::endpoint_open(local) error\n")); 
	 } */
	 this->handshake();
	 

}

Client::~Client(void){
   free(currentPacket);
   delete window_;
}

ACE_HANDLE
Client::get_handle (void) const
{
  return endpoint_.get_handle ();
}

int
Client::handle_input (ACE_HANDLE)
{
   ACE_DEBUG ((LM_DEBUG,"%p\n"," client::handle_input() call\n"));
   u_int8* buf = (u_int8 *)malloc((payloadSize+9)*sizeof(char));
   endpoint_.recv(buf, (payloadSize+9), remote);
   
   //chop buff and CRC, if fails, return 0;
   packet received = unpack(buf);
   if( crcCheck(received) != 1 ) {
     ACE_DEBUG ((LM_DEBUG,"%p\n","crcCheck failed!\n"));
	 return 0;
   }
   
   free(buf);
   
   switch (received.type) {
		case HAND_TYPE:  // handshake
			sendACK(received.seqNum);
			break;
		case DATA_TYPE:
		processPacket( received );
			break;
			default: break;
   }
   
   return 0;
}

int
Client::handle_close (ACE_HANDLE, ACE_Reactor_Mask)
{
  endpoint_.close ();
  return 0;
}

int Client::processPacket( packet p ){
	int seqNum = p.seqNum;
	/*cout << "processing packet with seqnum " << p << endl;
	cout << "window_->lastFrameReceived = " << window_->lastFrameReceived << endl;
	cout << "window_->largestFrameAcceptable = " << window_->largestFrameAcceptable  << endl;*/
	if( window_->lastFrameReceived <= seqNum && seqNum <= window_->largestFrameAcceptable ){
		//cout << " processing packet " << p << " and the window " << endl << *window_ << endl;
		if( window_-> frames[seqNum].status == EXPECTED ) { //not yet recv 
			window_->frames[seqNum].data = p;
			sendACK(seqNum);
			window_->frames[seqNum].status = OUTOFORDER; //buffered, to be acked.
			window_->packetNum ++;
		cout << "sliding window this far : " << window_->getSlideDist() << endl;
			window_->slide( window_->getSlideDist() );
		}
		else {
		if (window_->lastFrameReceived == seqNum ) {
			window_->frames[seqNum].data = p;
			sendACK(seqNum);
			window_->frames[seqNum].status = OUTOFORDER;
		cout << "sliding window this far : " << window_->getSlideDist() << endl;
			window_->slide( window_->getSlideDist() );
			
		 }	
		}
	}
	return 0;
}
int Client::handshake(){
	
	ACE_DEBUG ((LM_DEBUG,"%p\n","sending handshake!\n"));
	packet handshakePacket;
	handshakePacket.seqNum = 1;
	handshakePacket.type = HAND_TYPE;
	handshakePacket.len = 9;
	free(handshakePacket.payload);
	handshakePacket.payload = (u_int8*)calloc(handshakePacket.len,sizeof(u_int8));
	
	handshakePacket.payload[0] = ( this->window_->winSize >> 8) & 0xff;
	handshakePacket.payload[1] = ( this->window_->winSize) & 0xff;
	handshakePacket.payload[2] = ( this->window_->payloadLen >> 24) & 0xff;
	handshakePacket.payload[3] = ( this->window_->payloadLen >> 16) & 0xff;
	handshakePacket.payload[4] = ( this->window_->payloadLen >> 8) & 0xff;
	handshakePacket.payload[5] =  this->window_->payloadLen  & 0xff;
	handshakePacket.payload[6] = this->window_->type;
	handshakePacket.payload[7] = (this->window_->MaxSegNum >> 8) & 0xff;
	handshakePacket.payload[8] = (this->window_->MaxSegNum) & 0xff;

	int ret = sendPacket(handshakePacket);

	return ret;
	
}


int Client::sendPacket(packet p){
	 u_int8* buf = pack(p);
	// printf("this is the packet before packing\n");
	// cout << p << endl;
	 //printf("this is the packed packet being sent %x: \n",buf);
	if (endpoint_.send (buf,(p.len + 9) * sizeof(char), this->remote) == -1){
		ACE_ERROR ((LM_ERROR,"%p\n", "send"));
		return -1;
	}
	return 0;
}
int Client::sendACK(u_int16 seqNum) {
	packet p;
	p.seqNum = seqNum;
	p.type = ACK_TYPE;
	p.len = 1;
	sendPacket(p);
	return 0;
}
#endif

