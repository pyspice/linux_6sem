#ifndef INODE_H_INCLUDED
#define INODE_H_INCLUDED

#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "superblock.h"
#include "bitmap.h"
#include "utils.h"

#define NAME_LEN 31
#define TIME_LEN 25

struct s_inode
{
    uint32_t blocks[12];
    uint32_t iblock;
    uint32_t nlast;

    uint32_t ninode;
    uint32_t parent_inode;

    char name[NAME_LEN];

    char crtime[TIME_LEN];

    uint32_t size;

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

    strncpy((*node)->name, name, NAME_LEN - 1);

    (*node)->size = sizeof(struct s_inode);

    time_t rawtime;
    time(&rawtime);
    struct tm* timeinfo = localtime(&rawtime);
    strncpy((*node)->crtime, asctime(timeinfo), TIME_LEN - 1);

    (*node)->type = type;
}

void inode_del(struct s_inode* node)
{
    free(node);
}

void inode_copy(struct s_inode* node, struct s_inode* other)
{
    memcpy(node->blocks, other->blocks, 12 * sizeof(uint32_t));
    node->iblock = other->iblock;
    node->nlast = other->nlast;

    node->ninode = other->ninode;
    node->parent_inode = other->parent_inode;

    strncpy(node->name, other->name, NAME_LEN);

    strncpy(node->crtime, other->crtime, TIME_LEN);

    node->size = other->size;

    node->type = other->type;
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

    pread(fd, node->name, NAME_LEN * sizeof(char), offset);
    offset += NAME_LEN * sizeof(char);

    pread(fd, node->crtime, TIME_LEN * sizeof(char), offset);
    offset += TIME_LEN * sizeof(char);

    pread(fd, &node->size, sizeof(uint32_t), offset);
    offset += sizeof(uint32_t);

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

    pwrite(fd, node->name, NAME_LEN * sizeof(char), offset);
    offset += NAME_LEN * sizeof(char);

    pwrite(fd, node->crtime, TIME_LEN * sizeof(char), offset);
    offset += TIME_LEN * sizeof(char);

    pwrite(fd, &node->size, sizeof(uint32_t), offset);
    offset += sizeof(uint32_t);

    pwrite(fd, &node->type, sizeof(char), offset);
}

#endif // INODE_H_INCLUDED
