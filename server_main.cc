#include "server.h"



int main (int argc, char** argv)
{
	if(argc!=2){
		printf("server_main <local port>\n"); exit(1);
	}
	crcInit();


	ACE_INET_Addr addr (atoi(argv[1]));
	// server(listening addr, max packet size);
	Server server (addr,1024);
    ACE_Reactor::run_event_loop ();

  return 0;
}
