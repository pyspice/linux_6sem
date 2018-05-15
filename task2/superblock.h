#ifndef SUPERBLOCK_H_INCLUDED
#define SUPERBLOCK_H_INCLUDED

#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

struct s_superblock
{
    uint32_t blocks_total;
    uint32_t blocks_remain;

    uint32_t block_size;
    uint32_t inode_size;

    uint32_t bitmap_offset;
    uint32_t root_block;

    uint32_t magic;
};

void superblock_init(
    struct s_superblock** sb,
    uint32_t blocks_total,
    uint32_t blocks_remain,
    uint32_t block_size,
    uint32_t inode_size,
    uint32_t bitmap_offset,
    uint32_t root_block,
    uint32_t magic)
{
    *sb = (struct s_superblock*)malloc(sizeof(struct s_superblock));

    (*sb)->blocks_total = blocks_total;
    (*sb)->blocks_remain = blocks_remain;
    (*sb)->block_size = block_size;
    (*sb)->inode_size = inode_size;
    (*sb)->bitmap_offset = bitmap_offset;
    (*sb)->root_block = root_block;
    (*sb)->magic = magic;
}

void superblock_del(struct s_superblock* sb)
{
    free(sb);
}

void superblock_read(struct s_superblock* sb, int fd)
{
    pread(fd, sb, sizeof(struct s_superblock), 0);
}

void superblock_write(struct s_superblock* sb, int fd)
{
    pwrite(fd, sb, sizeof(struct s_superblock), 0);
}

#endif // SUPERBLOCK_H_INCLUDED
