#include "ace/ACE.h"
#include "ace/FILE_IO.h"
#include "ace/FILE_Addr.h"
#include "ace/FILE_Connector.h"
//#include "ace/ACE_Messaege_Block.h"


int main(){
	ACE_FILE_IO ifile;
	ACE_FILE_IO ofile;
	ACE_FILE_Addr inFile("test.txt");
	ACE_FILE_Addr other("t.txt");
	ACE_FILE_Connector conn;
	
	
	conn.connect(ifile , inFile, 0, ACE_Addr::sap_any, 0, O_RDWR|O_CREAT|O_APPEND , ACE_DEFAULT_FILE_PERMS);
	
	char  *buf = "yo come on now";
	ifile.send(buf, strlen(buf));
	ifile.dump();
	
	conn.dump();
	conn.connect(ofile , other, 0, ACE_Addr::sap_any, 0, O_RDWR , ACE_DEFAULT_FILE_PERMS);
	
	char *in = (char*) malloc(sizeof(char)* 2);
	int i =ofile.recv(in,10);
	while(i){
		ACE_DEBUG((LM_DEBUG, "%d: \t%s\n" , i, in));
		i =ofile.recv(in,10);
		/*if( i != 10){
			for(int o = 0; o <= (10 - i) ; ++o){
				in[i+o] = '\0';
			}*/
		}
	}
	ofile.dump();
		
	return 0;
};
