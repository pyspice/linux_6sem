#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include "options.h"
#include "inode.h"
#include "superblock.h"
#include "bitmap.h"
#include "utils.h"
#include "fs.h"

void flush_blocks(struct s_superblock* sb, int fd, uint32_t br, uint32_t pr)
{
    uint8_t* buf = (uint8_t*)malloc(br * sizeof(uint8_t));

    memset(buf, 0, br * sizeof(uint8_t));
    write(fd, buf, br * sizeof(uint8_t));

    buf = (uint8_t*)realloc(buf, sb->block_size * sizeof(uint8_t));
    memset(buf, 0, sb->block_size * sizeof(uint8_t));
    for (; pr; --pr)
        write(fd, buf, sb->block_size * sizeof(uint8_t));

    free(buf);
}

int main()
{
    int fd = open("fs.img", O_CREAT | O_RDWR, 0666);
    if (fd == -1)
    {
        perror("cannot create fs.img file");
        return 1;
    }

    uint32_t block_offset = (sizeof(struct s_superblock) + (BLOCKS_TOTAL >> 3) + BLOCK_SIZE - 1) >> 7;

    struct s_superblock* sb;
    superblock_init(&sb, BLOCKS_TOTAL, BLOCKS_TOTAL - block_offset, BLOCK_SIZE, sizeof(struct s_inode), sizeof(struct s_superblock), block_offset, MAGIC);

    superblock_write(sb, fd);
    bitmap_write(fd, block_offset, BLOCKS_TOTAL);
    flush_blocks(sb, fd, sb->block_size - sizeof(struct s_superblock), sb->blocks_remain);

    fs_mkroot(sb, fd);

    superblock_del(sb);
    close(fd);

    return 0;
}
