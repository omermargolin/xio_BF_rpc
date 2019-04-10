#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

int main(int argc, char *argv[])
{
   if (argc < 4) {
        fprintf(stderr, "Usage: %s <device> <LBA_offset> <num_of_blocks>\n", argv[0]);
        return 1;
    }

    int fd;
    char * dev_name = argv[1];  //{"pwrite.txt"};
    int offset = atoi(argv[2]);
    int num_blocks = atoi(argv[3]);
    char buffer[num_blocks * 512];

    //open file
    // fd = open(dev_name, O_RDWR|O_CREAT, 0777);
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

    if (pread(fd, &buffer, sizeof(buffer), offset) == -1) {
        perror("[error in read]\n");
        return 1;
    } else {
        printf("[reading data] from %s\n", dev_name);
        printf("[%s]\n", buffer);
    }
    return 0;
}
