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

    int len = strlen(name);
    strncpy((*node)->name, name, len);
    (*node)->name[len] = 0;

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
    pread(fd, node->blocks, 12 * sizeof(uint32_t), offset);
    offset += 12 * sizeof(uint32_t);

    pread(fd, &node->iblock, sizeof(uint32_t), offset);
    offset += sizeof(uint32_t);

    pread(fd, &node->nlast, sizeof(uint32_t), offset);
    offset += sizeof(uint32_t);

    pread(fd, &node->ninode, sizeof(uint32_t), offset);
    offset += sizeof(uint32_t);

    pread(fd, &node->parent_inode, sizeof(uint32_t), offset);
    offset += sizeof(uint32_t);

    pread(fd, node->name, 32 * sizeof(char), offset);
    offset += 32 * sizeof(char);

    pread(fd, &node->size, sizeof(uint32_t), offset);
    offset += sizeof(uint32_t);

    pread(fd, &node->rawtime, sizeof(node->rawtime), offset);
    offset += sizeof(node->rawtime);

    pread(fd, node->ctime, sizeof(struct tm*), offset);
    offset += sizeof(struct tm*);

    pread(fd, &node->type, sizeof(char), offset);
}

void inode_write(struct s_inode* node, struct s_superblock* sb, int fd, uint32_t offset)
{
    pwrite(fd, node->blocks, 12 * sizeof(uint32_t), offset);
    offset += 12 * sizeof(uint32_t);

    pwrite(fd, &node->iblock, sizeof(uint32_t), offset);
    offset += sizeof(uint32_t);

    pwrite(fd, &node->nlast, sizeof(uint32_t), offset);
    offset += sizeof(uint32_t);

    pwrite(fd, &node->ninode, sizeof(uint32_t), offset);
    offset += sizeof(uint32_t);

    pwrite(fd, &node->parent_inode, sizeof(uint32_t), offset);
    offset += sizeof(uint32_t);

    pwrite(fd, node->name, 32 * sizeof(char), offset);
    offset += 32 * sizeof(char);

    pwrite(fd, &node->size, sizeof(uint32_t), offset);
    offset += sizeof(uint32_t);

    pwrite(fd, &node->rawtime, sizeof(node->rawtime), offset);
    offset += sizeof(node->rawtime);

    pwrite(fd, node->ctime, sizeof(struct tm*), offset);
    offset += sizeof(struct tm*);

    pwrite(fd, &node->type, sizeof(char), offset);
}

#endif // INODE_H_INCLUDED
