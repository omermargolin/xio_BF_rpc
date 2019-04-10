#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#define BLOCK_SIZE 512

// Args: device_name, LBA offset in blocks of 512 bytes, number of blocks of 512 bytes to read
int main(int argc, char *argv[])
{
   if (argc < 4) {
        fprintf(stderr, "Usage: %s <device> <LBA_offset> <content_to_write>\n", argv[0]);
        return 1;
    }

    int fd;
    char * dev_name = argv[1];  //{"pwrite.txt"};
    int offset = atoi(argv[2]) * BLOCK_SIZE;
    char * data = argv[3];  // pointer to char[]
    int buff_len = strlen(data);
    printf("length of actual data:%d\n", buff_len);
    printf("sizeof buffer:%d\n", sizeof(data));
    printf("data:%s\n", data);
    
    //open file
    fd = open(dev_name, O_RDWR|O_CREAT, 0777);
    //error checking
    if (fd == -1) {
        perror("[error in open]\n");
        return 1;
    }

// //    // Get block size
// //    struct stat st;
// //    if (fstat(fd, &st)) return 1;
// //    printf("Media Block size = %s\n", st.st_blksize);

    if (pwrite(fd, data, buff_len, offset) == -1) {
        perror("[error in write]\n");
        return 1;
    }

    //error checking for close process
    if (close(fd) == -1){
        perror("[error in close]\n");
        return 1;
    }

    printf("[succeeded in write/close]\n");
    return 0;
}
