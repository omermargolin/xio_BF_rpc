#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#define BLOCK_SIZE 512

// Args: device_name, LBA offset in blocks of 512 bytes, number of blocks of 512 bytes to read, pointer to buffer where data expected.
int read_blkdev(char* dev_name, int offset, int num_blocks, char* buffer)
{
    int fd;
    //open file
    fd = open(dev_name, O_RDONLY, 0777);
    //error checking
    if(fd == -1) {
        perror("[error in open]\n");
        return 1;
    }

// //    // Get block size
// //    struct stat st;
// //    if (fstat(fd, &st)) return 1;
// //    printf("Media Block size = %s\n", st.st_blksize);

    if (pread(fd, buffer, num_blocks * BLOCK_SIZE, offset * BLOCK_SIZE) == -1) {
        perror("[error in read]\n");
        return 1;
    }

    // printf("[read data] from %s\n", dev_name);
    close(fd);

    return 0;
}
