#include <stdio.h>
#include "udp.h"
#include <signal.h>
#include "mfs.h"

#define BUFFER_SIZE (1000)

#define PORT 10000

void intHandler(int dummy) {
   UDP_Close(PORT);
   exit(130);
}

// server code
int main(int argc, char *argv[]) {
    int sd = UDP_Open(PORT);
    assert(sd > -1);
    signal(SIGINT, intHandler);
    
    while (1) {
	struct sockaddr_in addr;

	message_t m;
	message_t r;	
	printf("server:: waiting...\n");
	int rc = UDP_Read(sd, &addr, (char*) &m, BUFFER_SIZE);
	printf("server:: read message [size:%d contents:(%d)]\n", rc, m.mtype);
	if (rc > 0) {
            r.mtype = m.mtype;
	    r.rc = 3;
            rc = UDP_Write(sd, &addr, (char *) &r, BUFFER_SIZE);
	    printf("server:: reply\n");
	} 
    }
    return 0; 
}
   
