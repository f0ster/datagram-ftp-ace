#include "client.h"


int main (int argc, char **argv) { 
	if(argc!=4)
		{ printf("\nclient <local port> <serv_ip> <port>\n"); exit(1); }
		
	crcInit(); //setup crc Table
	
	u_short PORT = atoi(argv[3]);
	u_short localp = atoi(argv[1]);

	//setup client conditions
	ACE_INET_Addr local (localp);
	ACE_INET_Addr remote (PORT, argv[2]);
	int packetSize = 32*sizeof(char);
	char winType = STOP;  // GBN,STOP,REPEAT
	int winSize = 8;
	int maxSeg = 16;
	
	Client client (local,remote,packetSize,winType,winSize, maxSeg);
	ACE_Reactor::instance()->run_event_loop();
	
	
	return 0;
}
