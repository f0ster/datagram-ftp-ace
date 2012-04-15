#ifndef _SERVER_H
#define _SERVER_H

#include "/usr/local/ACE/ace/OS_main.h"
#include "/usr/local/ACE/ace/Reactor.h"
#include "/usr/local/ACE/ace/SOCK_Dgram.h"
#include "/usr/local/ACE/ace/INET_Addr.h"
#include "/usr/local/ACE/ace/ACE.h"

#include <string>
#include <map>
#include <cmath>

#include "ftp.h"
#include "window.h"
bool flag = true;
class Server : public ACE_Event_Handler
{
public:
  Server (const ACE_INET_Addr &addr,int);

  virtual ~Server (void);

  // = Override <ACE_Event_Handler> methods.
  virtual ACE_HANDLE get_handle (void) const;
  virtual int handle_input (ACE_HANDLE);
  virtual int handle_close (ACE_HANDLE handle, ACE_Reactor_Mask close_mask);
  virtual int handle_signal(ACE_HANDLE handle, ACE_Reactor_Mask close_mask);
  //window win;
protected:
  unsigned int payloadSize;
  map<string, window> *clientWindows;

private:
  ACE_SOCK_Dgram endpoint_;
  // Receives datagrams.

  ACE_UNIMPLEMENTED_FUNC (Server (void))
  ACE_UNIMPLEMENTED_FUNC (Server (const Server &))
  ACE_UNIMPLEMENTED_FUNC (Server &operator= (const Server &))
  
  int processPacket(window&, packet, ACE_INET_Addr);
  int sendPacket(packet, ACE_INET_Addr);
  int sendACK(u_int16 seqNum ,ACE_INET_Addr to_addr );	
};

Server::Server (const ACE_INET_Addr &addr, int pSize) :  endpoint_ (addr)
{
	  this->payloadSize = pSize;
	  this->clientWindows = new map<string, window>();
      if (ACE_Reactor::instance ()->register_handler (this, ACE_Event_Handler::READ_MASK) == -1)
        ACE_ERROR ((LM_ERROR, "ACE_Reactor::register_handler: Server\n"));
	
}

Server::~Server (void)
{
}

ACE_HANDLE
Server::get_handle (void) const
{
  return endpoint_.get_handle ();
}

int
Server::handle_input (ACE_HANDLE)
{
  unsigned char buf[payloadSize+9];
  ACE_INET_Addr from_addr;

  ssize_t n = endpoint_.recv (buf, sizeof buf, from_addr);
  
 // printf("this is the packet received %x: \n",buf);
  
  packet receivedPacket = unpack(buf);

  printf("pass crc %i\n", crcCheck(receivedPacket));
  //cout << "packet after crc  " << receivedPacket << endl;
  
  ACE_TCHAR addrStr[50];
  from_addr.addr_to_string( addrStr, sizeof(addrStr), 1);
   
  map<string,window>::iterator it;
  pair<map<string,window>::iterator,bool> ret;

  it=clientWindows->find(string(addrStr));
 
  
  
  if( it == clientWindows->end() ) { // not found
	  window newWindow(SEND);
	clientWindows->insert (pair<string,window>(  string(addrStr), newWindow  )); 
	//ACE_DEBUG ((LM_DEBUG, "New connection from %s\n", addrStr));
	it=clientWindows->find( string(addrStr) );
	processPacket( it->second, receivedPacket, from_addr );
	
  }
  else{
	processPacket(it->second, receivedPacket, from_addr);
  }
	//processPacket(newWindow, receivedPacket, from_addr); // added this
	return 0;
}
int Server::handle_signal(ACE_HANDLE handle, ACE_Reactor_Mask close_mask){
	return 0;
}
int Server::handle_close (ACE_HANDLE,  ACE_Reactor_Mask)
{
  endpoint_.close ();

  return 0;
}

int Server::processPacket(window &w, packet p, ACE_INET_Addr from_addr) {
	 char addrStr[50];
	 from_addr.addr_to_string( addrStr, sizeof(addrStr), 1);
	 //u_int8 tmp = p.type;
	 //cout << p << endl;
	// printf("receieved %c from %s\n", tmp, addrStr);
	 switch(p.type) {
		case HAND_TYPE:

			w.winSize = (p.payload[0] << 8) | p.payload[1] ;
			w.payloadLen = p.payload[2];
			w.payloadLen = ( w.payloadLen << 8) | p.payload[3];
			w.payloadLen = ( w.payloadLen << 8) | p.payload[4];
			w.payloadLen = ( w.payloadLen << 8) | p.payload[5];
			w.type = p.payload[6];
			w.MaxSegNum = (p.payload[7] << 8) | p.payload[8] ;
			ACE_DEBUG ((LM_DEBUG, "Setting client window size %i and payload size %i and type to %c and max seg size to %i\n",w.winSize,w.payloadLen, w.type, w.MaxSegNum));
			sendPacket(p,from_addr);
		break;
		case ACK_TYPE:
			cout << "Packet : " << p.seqNum << " acked!" <<endl;
			w.init(); // setup
			cout << "LFS( " << w.lastFrameSent << " )  - LAR( " << w.lastAcknowledgmentReceived << " ) <= winSize ( " << w.winSize << " ) " << " witch is " << (w.lastFrameSent - w.lastAcknowledgmentReceived  < w.winSize) << endl;
			if(w.lastFrameSent - w.lastAcknowledgmentReceived  <= w.winSize){ 
				cout << "w.frames[p.seqNum].status : " << w.frames[p.seqNum].status << " == " << NOTACKED <<endl;
				if(w.frames[p.seqNum].status == NOTACKED){ // sent not yet acked received
					 
					w.frames[p.seqNum].status = ACKED;
					int j = w.getSlideDist();
					cout << "sliding window this far : " << j << endl;
					w.slide( j );
					w.lastAcknowledgmentReceived = p.seqNum;
				}
			}else{
				cout << "ack not in window range" << endl;
				// ack not in the window range
			}
			for(int i=w.lastAcknowledgmentReceived + 1; (i != w.lastFrameSent) || ( i==0 && w.lastFrameSent==0 ); i++) {
				i = i % w.MaxSegNum;
				if ((w.frames[i].status == READY ) &&  (w.lastFrameSent - w.lastAcknowledgmentReceived  <= w.winSize)){
					//cout << "LFS( " << w.lastFrameSent << " )  - LAR( " << w.lastAcknowledgmentReceived << " ) <= winSize ( " << w.winSize << " ) " << " witch is " << (w.lastFrameSent - w.lastAcknowledgmentReceived  < w.winSize) <<endl;
					sendPacket(w.frames[i].data,from_addr);
					//w.lastFrameSent = w.frames[i].data.seqNum;
					w.frames[i].status = NOTACKED;
					cout<< w << endl;
				}
			}
			if (flag){
				w.lastAcknowledgmentReceived = 0;
				flag = false;
			}
			cout<< w << endl;
				
		break;
		default:
			ACE_DEBUG ((LM_DEBUG, "packet type not recognized!\n"));
			return 0;
		break;
	 }
	 return 0;

}
int Server::sendPacket(packet p, ACE_INET_Addr to_addr){
	u_int8* buf = pack(p);
	cout << "Packet sent: " << p.seqNum << endl;
	if (this->endpoint_.send (buf,(p.len + 9) * sizeof(char), to_addr) == -1){
		ACE_ERROR ((LM_ERROR,"%p\n", "send"));
		return -1;
	}
	return 0;
}

int Server::sendACK(u_int16 seqNum ,ACE_INET_Addr to_addr ) {
	packet p;
	p.seqNum = seqNum;
	p.type = ACK_TYPE;
	p.len = 1;
	cout << "sending hand ack " << p <<endl;
	u_int8* buf = pack(p);
	if ( endpoint_.send( buf, (p.len + 9) * sizeof(char), to_addr ) == -1 ) {
		ACE_ERROR_RETURN ((LM_ERROR,"%p\n", "sendACK(%d)",seqNum), -1);
		return -1;
	}
	return 0;
}
#endif
