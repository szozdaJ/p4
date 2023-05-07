#include "mfs.h"
#include "udp.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>

int serverport;
struct sockaddr_in server;
int sd;

message_t message;
message_t reply;

int transmit(message_t message){
	int rc = UDP_Write(sd, &server, (char*) &message, sizeof(message_t));
	return rc;
}

int
MFS_Init(char *hostname, int port)
{

 struct sockaddr_in replyaddr;
 serverport = port;

 sd = UDP_Open(5000);
 int rc;
 if ((rc = UDP_FillSockAddr(&server, hostname, port)) == -1){
	printf("port fill failure |n");
	exit(1);
 }
 server = server;
 

 sprintf(message.cmd,"Init");
 message.mtype = 0;

 rc = transmit(message);

 fd_set m;
 FD_ZERO(&m);
 FD_SET (sd, &m);

 struct timeval t;
 t.tv_sec = 5;
 t.tv_usec = 0;
 int rc2 = select(sd+1, &m, NULL, NULL, &t);
 printf("Running %s on %d and select returned =%d\n",hostname,serverport,rc2);
 if(rc2>0) {
	rc = UDP_Read(sd, &replyaddr, (char*)&reply, sizeof(message_t));
	return 0;
 } else {
	return -1;
 }


 return 0;
}


int MFS_Lookup(int pinum, char* name)
{


 struct sockaddr_in replyaddr;
 sprintf(message.cmd, "Lookup");
 message.mtype = 1;
 message.pinum = pinum;
 sprintf(message.name, name);

 int rc;

 rc = transmit(message);

 fd_set m;
 FD_ZERO (&m);
 FD_SET (sd, &m);

 struct timeval t;
 t.tv_sec = 5;
 t.tv_usec = 0;
 int rc2 = select(sd+1, &m, NULL, NULL, &t);

 if (rc2>0) {
	 rc = UDP_Read(sd, &replyaddr, (char*)&reply, sizeof(message_t));
	// printf("returned %d  %s %d\n", reply.rc, reply.name, reply.mtype);
 } else {
	return -1;
 }

 return reply.rc;
}

int MFS_Stat(int inum, MFS_Stat_t *msg)
{


struct sockaddr_in replyaddr;
 sprintf(message.cmd, "Stat");
 message.mtype = 2;
 message.inum = inum;
 int rc;

 rc = transmit(message);

 fd_set m;
 FD_ZERO (&m);
 FD_SET (sd, &m);

 struct timeval t;
 t.tv_sec = 5;
 t.tv_usec = 0;
 int rc2 = select(sd+1, &m, NULL, NULL, &t);

 if (rc2>0) {
	rc = UDP_Read(sd, &replyaddr, (char*)&reply, sizeof(message_t));
 } else {
	 return -1;
 }

 return reply.rc;
}

int MFS_Write(int inum, char*buffer, int offset, int nbytes)
{

 return 0;
}

int MFS_Read(int inum, char *buffer, int offset, int nbytes) {
//check for valid inum, valid offset, valid nbytes
 
 return 0;
}
//creates a file REG_FILE or MFS_DIRECTORY in parent directory name, with inode pinum
int MFS_Creat(int pinum, int type, char *name)
{

struct sockaddr_in replyaddr;

 sprintf(message.cmd,"Creat");
 message.mtype = 5;
 message.pinum = pinum;
 message.type = type;
 sprintf(message.name, name);
 printf("%s\n", message.name);

 int rc;

 rc = transmit(message);
  //printf("Sent message\n");
  fd_set m;
  FD_ZERO(&m);
  FD_SET(sd,&m);

  struct timeval t;
  t.tv_sec=5;
  t.tv_usec=0;
  int rc2 = select(sd+1,&m, NULL, NULL, &t);
  if (rc2 > 0) {
	rc = UDP_Read(sd, &replyaddr, (char*)&reply, sizeof(message_t));
  	//printf("returned %d, %d\n", rc, reply.rc);
	return reply.rc;
  } else {
	printf("Creat rc2: %d is not greater than 0\n", rc2);
	return -1;
  }


 return 0;
}
//removes file or directory name for directory by pinum
int MFS_Unlink(int pinum, char *name)
{
 
 struct sockaddr_in replyaddr;
 sprintf(message.cmd,"Unlink");
 message.mtype = 6;
 message.pinum = pinum;
 sprintf(message.name, name);

 int rc;

 rc = transmit(message);

  fd_set m;
  FD_ZERO(&m);
  FD_SET(sd,&m);

  struct timeval t;
  t.tv_sec=5;
  t.tv_usec=0;
  int rc2 = select(sd+1,&m, NULL, NULL, &t);
  if (rc2 > 0) {
	rc = UDP_Read(sd, &replyaddr, (char*)&reply, sizeof(message_t));
  } else {
	  return -1;
  }

 return reply.rc;
}

int MFS_Shutdown()
{
  
struct sockaddr_in replyaddr;
 sprintf(message.cmd,"Shutdown");
  message.mtype=7;
  int rc;
  
  rc = transmit(message);
   
  fd_set m;
  FD_ZERO(&m);
  FD_SET(sd,&m);

  struct timeval t;
  t.tv_sec=5;
  t.tv_usec=0;
  rc = select(sd+1,&m, NULL, NULL, &t);
  if (rc > 0) {
	rc = UDP_Read(sd, &replyaddr, (char*)&reply, sizeof(message_t));
  } else {
	  return -1;
  }

  rc = UDP_Close(sd);
  return 0;	
}
