#ifndef INODE_H_INCLUDED
#define INODE_H_INCLUDED

#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "superblock.h"
#include "bitmap.h"
#include "utils.h"

struct s_inode
{
    uint32_t blocks[12];
    uint32_t iblock;
    uint32_t nlast;

    uint32_t ninode;
    uint32_t parent_inode;

    char name[32];

    uint32_t size;

    time_t rawtime;
    struct tm* ctime;
    char type;
};

void inode_init(struct s_inode** node,
                uint32_t ninode,
                uint32_t parent_inode,
                const char* name,
                char type)
{
    (*node) = (struct s_inode*)malloc(sizeof(struct s_inode));
    memset((*node)->blocks, 0, 12 * sizeof(uint32_t));
    (*node)->iblock = 0;
    (*node)->nlast = 0;

    (*node)->ninode = ninode;
    (*node)->parent_inode = parent_inode;

    strncpy((*node)->name, name, strlen(name));

    (*node)->size = 0;

    time(&((*node)->rawtime));
    (*node)->ctime = localtime(&((*node)->rawtime));

    (*node)->type = type;
}

void inode_del(struct s_inode* node)
{
    free(node);
}

void inode_read(struct s_inode* node, struct s_superblock* sb, int fd, uint32_t offset)
{
    pread(fd, node, sb->inode_size, offset);
}

void inode_write(struct s_inode* node, struct s_superblock* sb, int fd, uint32_t offset)
{
    pwrite(fd, node, sb->inode_size, offset);
}

#endif // INODE_H_INCLUDED
