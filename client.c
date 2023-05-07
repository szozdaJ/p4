#include <stdio.h>
#include "udp.h"
#include "mfs.h"

#define BUFFER_SIZE (1000)

// client code
int main(int argc, char *argv[]) {
   // struct sockaddr_in addrSnd, addrRcv;
   
    if (argc != 2) {
	printf("Usage: client [server-port]\n");
	return -1;
    }

   //int sd = UDP_Open(20000);
   //int rc = UDP_FillSockAddr(&addrSnd, "localhost", 10010);

    int rc = MFS_Init("localhost", atoi(argv[1]));

    if (rc != 0) {
	printf("client:: failed to connect");
	exit(1);
    } 
    rc = MFS_Lookup(0, "testdir");

    rc = MFS_Unlink(0, "testdir");
    rc = MFS_Lookup(0, "testdir");
       printf("Testdir rc: %d\n", rc);

    rc = MFS_Shutdown();   

    /*char message[BUFFER_SIZE];
    //sprintf(message, "hello world");

    //printf("client:: send message [%s]\n", message);
   // rc = UDP_Write(sd, &addrSnd, message, BUFFER_SIZE);
    if (rc < 0) {
	printf("client:: failed to send\n");
	exit(1);
    }

    printf("client:: wait for reply...\n");
    rc = UDP_Read(sd, &addrRcv, message, BUFFER_SIZE);
    printf("client:: got reply [size:%d contents:(%s)\n", rc, message);
    */
    return 0;
    
}
