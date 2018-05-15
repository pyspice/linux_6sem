#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <ctype.h>

int main()
{
    uint32_t block[32];

    int fd = open("fs.img", O_RDWR);
    if (fd == -1)
    {
        perror("cannot open fs.img file");
        return 1;
    }



    close(fd);
}
