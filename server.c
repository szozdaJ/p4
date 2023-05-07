#include <stdio.h>
#include "udp.h"
#include <signal.h>
#include "mfs.h"
#include "ufs.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <assert.h>
#include <sys/mman.h>
#include <sys/types.h>

#define BUFFER_SIZE (1000)

int lookup(int pinum, char* filename);
int create(int pinum, int type, char *name);
int unlnk(int pinum, char *name);
void* stats(int inum);

super_t *s;
inode_t *root_inode;
int *inode_bitmap;
int *data_bitmap;
void *image;
MFS_Stat_t *m;

int sd;
int fd;
void intHandler(int dummy) {
   UDP_Close(sd);
   exit(130);
}

int readIbit(int bit) {
   return !!(inode_bitmap[bit/32] & (1 << (31 - bit % 32)));
}

int readDbit(int bit) {
   return !!(data_bitmap[bit/32] & (1 << (31 - bit % 32)));
}

void writeIbit(int bit) {
    *inode_bitmap = (*inode_bitmap | 1 << (31 - bit % 32));
}

void writeDbit(int bit) {
    *data_bitmap = (*data_bitmap | 1 << (31 - bit % 32));
}

//helper function to find an available inode
int allocInode() {

    for (int i = 0; i < s->num_inodes; i++) {
	if (readIbit(i) == 0) {//unused bit
	   writeIbit(i);
	   return i;
	}
    }

    return -1;//failed
}

int allocData() {
    
   for (int i = 0; i < s->num_data; i++ ) {
	if (readDbit(i) == 0) {
	  writeDbit(i);
	  return i;
	}
   }

	
   return -1;
}
//in house testing of code consistency
void test() {
    unlnk(0, "testdir");
	
    exit(0);
}

// server code
int main(int argc, char *argv[]) {
    
  
    if (argc < 3) {
	printf("Usage: server [port-num] [file-system-image] &\n");
	exit(1);
    }
    
    
    signal(SIGINT, intHandler);
    sd = UDP_Open(atoi(argv[1]));
    assert(sd > -1);
    //server port
    int port_num = atoi(argv[1]);
    assert(port_num > -1); 
    
    //open file image
    fd = open(argv[2], O_RDWR);
    if (fd>=0) {
	printf("File %s opened\n", argv[2]);
    } else {
	printf("File does not exist, exiting... \n");
	exit(1);
    }

    //reading in data from image file
    struct stat sbuf;
    int rc = fstat(fd, &sbuf);
    assert(rc > -1);

    int image_size = (int) sbuf.st_size;

    image = mmap(NULL, image_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    assert(image != MAP_FAILED);
    
    s = (super_t *) image;
    inode_t *inode_table = image + (s->inode_region_addr * UFS_BLOCK_SIZE);
    root_inode = inode_table;//root inode to iterate from
    //need to parse out data and fit into memory using structs
    inode_bitmap = image + (s->inode_bitmap_addr * UFS_BLOCK_SIZE);
    data_bitmap = image + (s->data_bitmap_addr * UFS_BLOCK_SIZE);

    printf("              Server ready to accept commands\n");
    

    message_t message;


    //test();

    while(1) {
       
        message_t reply;

        sprintf(reply.name, "reply");
        reply.rc = 0;    
   	struct sockaddr_in client;
        //waits for a message
       int rc = UDP_Read(sd, &client, (char*)&message, 1000);
       //printf("server:: read message [size:%d contents: (%d)]\n", rc, message.mtype);
       if (rc > 0) {
	       if (message.mtype == 0) {
		   //printf("Server INIT: Client %s\n", message.name);
		   reply.rc = 0;
	       } else if (message.mtype == 1) {
                   // lookup command
		   message_t replytest;
		  // printf("Server recieved Lookup cmd\n");
		   printf("%d, %s\n", message.pinum, message.name);
		   reply.rc = lookup(message.pinum, message.name);
		   //reply.rc = 1;
                   //message.rc = 1;
		 
		} else if (message.mtype == 2) {
		   printf("Recieved Stat cmd\n");
		   if (stats(message.inum) == -1) { //reply wants the metadata back
		     reply.rc = -1;
		   } else 
	             reply.rc = 0;
		} else if (message.mtype == 5) {
		   printf("Server recieved Creat cmd\n");
		   reply.rc = create(message.pinum, message.type, message.name);
                   if (reply.rc != -1) {
		     msync(s, sizeof(super_t), MS_SYNC);
		   }
		   //reply.rc = 1;
		} else if (message.mtype == 6) {
		   printf("Server recieved Unlink cmd\n");
		   //reply.rc = unlnk(message.pinum, message.name);
		   reply.rc = -1;
		   if (reply.rc != -1) {
		     msync(s, sizeof(super_t), MS_SYNC);
		   }

	        } else if (message.mtype == 7) {
                   //printf("Closing...\n");  
		   rc = UDP_Write(sd, &client, (char*) &reply, sizeof(message_t));
		   msync(s, sizeof(super_t), MS_SYNC);
    		   close(fd);
                   UDP_Close(sd);
		   exit(0);
		} else {
		   printf("unknown command\n");
		   rc = -1;
		}

		   rc = UDP_Write(sd, &client, (char*) &reply, sizeof(message_t));
	    }
       //code to exit out of server loop since shutdown was called
    }
    
    /*  while (1) {
	struct sockaddr_in addr;
	char message[BUFFER_SIZE];
	printf("server:: waiting...\n");
	int rc = UDP_Read(sd, &addr, message, BUFFER_SIZE);
	printf("server:: read message [size:%d contents:(%s)]\n", rc, message);
	if (rc > 0) {
            char reply[BUFFER_SIZE];
            sprintf(reply, "goodbye world");
            rc = UDP_Write(sd, &addr, reply, BUFFER_SIZE);
	    printf("server:: reply\n");
	} 
    }*/



    return 0; 
}


int lookup(int pinum, char* filename) {

    if (pinum < 0 || pinum >= s->num_inodes) {
	printf("Server: invalid pinum\n");
	return -1;
    }

    inode_t *parent = root_inode + pinum;
    if (parent->type != MFS_DIRECTORY) {
	printf("Server: Inum does not point to a directory\n");
	return -1;
    }
    dir_ent_t *seekEntry;
    //linear search for the direct pointer
    for (int i = 0; i < DIRECT_PTRS; i++) { 
	seekEntry = (dir_ent_t*)(image + parent->direct[i] * UFS_BLOCK_SIZE);
	if (seekEntry != -1) {
	   for( int j = 0; j < UFS_BLOCK_SIZE/sizeof(dir_ent_t); j++) {	
	   //valid entry
	    if (seekEntry[j].inum != -1) {
	     if (strcmp(filename, seekEntry[j].name) == 0) {
		//printf("entry found\n");
		return seekEntry[j].inum;
	     }
 	    }
	   }
	}

    }
    //no valid entry found
    printf("Server: entry not found\n");
    return -1;

}
//creates a new directory based on the parent inum, of 
//MFS_DIRECTORY or MFS_FILE type with name
//remember that data starts at block 4
int create(int pinum, int type, char *name) {
   

    if (pinum < 0 || pinum >= s->num_inodes) {
	printf("Invalid pinum \n");
	return -1;
    }

    if (strlen(name) >= 28) {
	printf("Name too long to fit\n");
	return -1;
    }

    inode_t *parent = root_inode + pinum;
    //printf("parent stats: [%d, %d, %d] \n", parent->type, parent->size, parent->direct[0]);
    if (parent->type != MFS_DIRECTORY) {
	printf("Creat failed: parent is not a directory\n");
	return -1;
    }
    dir_ent_t *seekEntry;
    //checking if entry exists already
    for (int i = 0; i < DIRECT_PTRS; i++) {
	if (parent->direct[i] != -1) {
	seekEntry = (dir_ent_t*)(image + parent->direct[i] * UFS_BLOCK_SIZE);
	  for (int j = 0; j < UFS_BLOCK_SIZE/sizeof(dir_ent_t); j++) {
	      //printf("DEBUG: seekEntry %s, %d\n", seekEntry[j].name, seekEntry[j].inum);
 	   if (seekEntry[j].inum != -1) {
	      if(strcmp(seekEntry[j].name, name) == 0) {
		printf("Name already exists\n");
		return 0;
	      }
	   }
          }
	}
    }
    printf("creating..\n");
    
    //printf("entered create\n");
    //return 0;
    //name does not exist, creating
    if (type == MFS_DIRECTORY) {// == 0
	//get new data block
	int d = allocData();
	printf("%d\n", d);
	if (d != -1) {
	   for (int i = 0; i < DIRECT_PTRS; i++) {
              seekEntry = (dir_ent_t *)(image + parent->direct[i] * UFS_BLOCK_SIZE);
	      for (int j = 0; j < UFS_BLOCK_SIZE/sizeof(dir_ent_t); j++) {
	      if(seekEntry[j].inum == -1) {//found an unused directory entry
	         //find an available inode
	         int inode = allocInode();
	         if (inode != -1) {//can only reach here if d != -1 && i != -1
	   	   //set up parents metadata in inode and data block
	           seekEntry[j].inum = inode;
		   sprintf(seekEntry[j].name, name);
		   parent->size += sizeof(dir_ent_t);
		   //new inode metadata
		   inode_t* childDir = root_inode + inode;//root inode is pointer at start of region + i offset
		   childDir->type = MFS_DIRECTORY;
		   childDir->size = 2*sizeof(dir_ent_t);
		   childDir->direct[0] = s->data_region_addr + inode;//pointing to itself
		   childDir->direct[1] = parent->direct[0];//parent points to itself, equal to that
		   //new data block data
		   dir_ent_t *trivial = (dir_ent_t*)(image + childDir->direct[0] * UFS_BLOCK_SIZE);
		   trivial[0].inum = inode;
		   sprintf(trivial[0].name, ".");
		   trivial[1].inum = pinum;
		   sprintf(trivial[1].name, "..");
	           return 0;
	         } else {
		   printf("No available inodes to allocate\n");
		   return -1;
	         }

	      }
	      }
	   } 		

	} else {
	    printf("No available data blocks to allocate\n");
	    return -1;
	}
    } else if (type == MFS_REGULAR_FILE) {//need to get inode and data block, but data block will be empty
	
	int d = allocData();
	if (d != -1) {
	   for (int i = 0; i < DIRECT_PTRS; i++) {
              seekEntry = (dir_ent_t *)(image + parent->direct[i] * UFS_BLOCK_SIZE);
	      for(int j = 0; j < UFS_BLOCK_SIZE/sizeof(dir_ent_t); j++) {
	      if(seekEntry[j].inum == -1) {//found an unused directory entry
	         //find an available inode
	         int inode = allocInode();
	         if (inode != -1) {//can only reach here if d != -1 && i != -1
	   	   //set up parents metadata in inode and data block
	           seekEntry[j].inum = inode;
		   sprintf(seekEntry[j].name, name);
		   parent->size += sizeof(dir_ent_t);
		   //new inode metadata
		   inode_t* childDir = root_inode + inode;//root inode is pointer at start of region + i offset
		   childDir->type = MFS_REGULAR_FILE;
		   childDir->size = 0;// empty file created
		   childDir->direct[0] = s->data_region_addr + d;//points to the data block now
		   return 0;
		 } else {
			printf("no room to allocate\n");
			return -1;
		 }
	      }
	      }
	   }
	}
    } else {
	printf("Error reading type of file to create \n");
	return -1;
    }
    return -1;
}

int unlnk(int pinum, char *name) {
    if (pinum < 0 || pinum >= s->num_inodes) {
	    printf("Invalid inum\n");
	    return -1;
    }

    inode_t *parent = root_inode + pinum;
    if (parent->type != MFS_DIRECTORY) {
	printf("Server: Inum does not point to a directory\n");
	return -1;
    }
    dir_ent_t *seekEntry;
    for (int i = 0; i < DIRECT_PTRS; i++) { 
	seekEntry = (dir_ent_t*)(image + parent->direct[i] * UFS_BLOCK_SIZE);
	if (parent->direct[i] != -1) {
	   for( int j = 0; j < UFS_BLOCK_SIZE/sizeof(dir_ent_t); j++) {	
	   //valid entry
	    
             if (strcmp(name, seekEntry[j].name) == 0) {
	      inode_t *child = root_inode + seekEntry[j].inum;
	       
	       if(child->type = MFS_DIRECTORY && child->size <= 64) {
	         sprintf(seekEntry[j].name, "");
		 seekEntry[j].inum = -1;
		 return 0;
	       } else if (child->type = MFS_REGULAR_FILE) {
		 sprintf(seekEntry[j].name, "");
		 seekEntry[j].inum = -1;
		 return 0;
	       }
	      
	      
	       else {
		  printf("Target file not able to be unlinked\n");
		  return -1;
	       }
	     }
 	    }
	   
	}

    }
    return 0;
}


void* stats(int inum) {
    
    if (readIbit(inum) != 1) {
 	printf("Inode not used\n");
	return -1;
    }

    return (image + s->inode_region_addr * UFS_BLOCK_SIZE) + inum;

}

void debugreply(message_t reply){ 

printf("reply  mtype: %d \n type: %d \n pinum: %d \n name: %s \n inum: %d \n offset: %d \n nbytes: %d \n rc: %d \n",
	       	reply.mtype, reply.type, reply.pinum, reply.name, reply.inum, reply.offset, reply.nbytes, reply.rc);

}

