#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>


#include "ufs.h"

int main(int argc, char *argv[]) {
    int fd = open("test.img", O_RDWR);
    assert(fd > -1);

    struct stat sbuf;
    int rc = fstat(fd, &sbuf);
    assert(rc > -1);

    int image_size = (int) sbuf.st_size;

    void *image = mmap(NULL, image_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    assert(image != MAP_FAILED);

    super_t *s = (super_t *) image;
    printf("inode bitmap address %d [len %d]\n", s->inode_bitmap_addr, s->inode_bitmap_len);
    printf("data bitmap address %d [len %d]\n", s->data_bitmap_addr, s->data_bitmap_len);
    printf("inode region address %d [len %d]\n", s->inode_region_addr, s->inode_region_len);
    printf("data region address %d [len %d]\n", s->data_region_addr, s->data_region_len);

    inode_t *inode_table = image + (s->inode_region_addr * UFS_BLOCK_SIZE);
    inode_t *root_inode = inode_table;
    printf("\nroot type:%d root size:%d\n", root_inode->type, root_inode->size);
    printf("direct pointers[0]:%d [1]:%d\n", root_inode->direct[0], root_inode->direct[2]);

    dir_ent_t *root_dir = image + (root_inode->direct[0] * UFS_BLOCK_SIZE);
    printf("\nroot dir entries\n%d %s\n", root_dir[0].inum, root_dir[0].name);
    printf("%d %s\n", root_dir[1].inum, root_dir[1].name);
    //testing
    printf("\ntest added directory\n %d %s\n", root_dir[2].inum, root_dir[2].name);
    printf("\ntest added file\n %d %s\n", root_dir[3].inum, root_dir[3].name);
    //assert(root_dir[2].inum == -1);

    inode_t *parent = root_inode + 1;
    inode_t *file = root_inode + 2;
    dir_ent_t *testdir = image + (parent->direct[0] * UFS_BLOCK_SIZE);
    printf("\nparent type:%d parent size:%d, direct[0]%d\n", parent->type, parent->size, parent->direct[0]);
    printf("\ntest dir entries\n%d %s\n", testdir[0].inum, testdir[0].name);
    printf("%d %s\n", testdir[1].inum, testdir[1].name);
    
    printf("\nfile type:%d, file size:%d, direct[0]:%d\n", file->type, file->size, file->direct[0]);

    inode_t *testarith = parent - 2;
    printf("\nparent type:%d parent size:%d\n", testarith->type, testarith->size);

    printf("\ndata region addr: %d [len %d]\n", s->data_region_addr, s->data_region_len);

    printf("\nnumber inodes: %d number of data blocks: %d\n", s->num_inodes, s->num_data);

    int *inode_bitmap = image + (s->inode_bitmap_addr * UFS_BLOCK_SIZE);
    int *data_bitmap = image + (s->data_bitmap_addr * UFS_BLOCK_SIZE);
 
    //inode_bitmap += 3;
    //data_bitmap += 3;
    printf("\n inode bitmap: %u,  data bitmap: %u\n", *inode_bitmap, *data_bitmap);
    //int bit = 32;

   /* for (int bit = 0; bit < 32; bit++) {
    	int readbit = !!(inode_bitmap[bit/32] & (1 << (31 - bit % 32)));
    	int readdbit = !!(data_bitmap[bit/32] & (1 << (31 - bit % 32)));
	printf("\n inode bitmap bit %d value: %d || data: %d, value %d\n", bit, readbit, bit, readdbit);
    }*/

    close(fd);
    return 0;

}

