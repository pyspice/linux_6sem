#ifndef FS_H_INCLUDED
#define FS_H_INCLUDED

#include "superblock.h"
#include "inode.h"
#include "utils.h"
#include "bitmap.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

void fs_mkroot(struct s_superblock* sb, int fd)
{
    int32_t nblock = bitmap_get_available_block(sb, fd);
    bitmap_set_unavailable(sb, fd, nblock);

    struct s_inode* dir;
    inode_init(&dir, nblock, 0, "/", 'd');

    uint32_t offset = get_block_offset(sb, fd, nblock);
    inode_write(dir, sb, fd, offset);

    inode_del(dir);
}

void fs_mkdir(struct s_superblock* sb, int fd, struct s_inode* parent, const char* name)
{
    if ((sb->blocks_remain == 0) || (sb->blocks_remain == 1 && parent->blocks[11] != 0))
    {
        puts("cannot create directory: no available space");
        return;
    }

    if (parent->iblock != 0 && parent->nlast == sb->block_size >> 5)
    {
        puts("cannot create directory: max number of subdirectories/files reached");
        return;
    }

    uint32_t nblock = bitmap_get_available_block(sb, fd);
    bitmap_set_unavailable(sb, fd, nblock);

    struct s_inode* dir;
    inode_init(&dir, nblock, parent->ninode, name, 'd');

    uint32_t offset = get_block_offset(sb, fd, nblock);
    inode_write(dir, sb, fd, offset);

    if (parent->iblock == 0 && parent->nlast < 12)
    {
        parent->blocks[parent->nlast] = nblock;
        ++(parent->nlast);
    }
    else
    {
        uint32_t* block = (uint32_t*)malloc(sb->block_size);
        pread(fd, block, sb->block_size, get_block_offset(sb, fd, parent->iblock));

        if (parent->iblock == 0)
        {
            uint32_t iblock = bitmap_get_available_block(sb, fd);
            bitmap_set_unavailable(sb, fd, iblock);

            parent->iblock = iblock;
            parent->nlast = 1;

            block[0] = nblock;
        }
        else
        {
            block[parent->nlast] = nblock;
            ++(parent->nlast);
        }

        pwrite(fd, block, sb->block_size, get_block_offset(sb, fd, parent->iblock));
        free(block);
    }

    inode_del(dir);

    inode_write(parent, sb, fd, get_block_offset(sb, fd, parent->ninode));
}

void fs_ls(struct s_superblock* sb, int fd, struct s_inode* node)
{
    if (node->iblock == 0 && node->nlast == 0)
        return;

    struct s_inode* tmp;
    inode_init(&tmp, 0, 0, "", '\0');
    uint32_t i, len = (node->iblock ? 12 : node->nlast);

    char ctime[26] = {0};
    for (i = 0; i < len; ++i)
    {
        inode_read(tmp, sb, fd, get_block_offset(sb, fd, node->blocks[i]));
        printf("%d\n", strlen(tmp->name));
        strncpy(ctime, asctime(tmp->ctime), strlen(ctime) - 1);
        printf("\t%c %s %10d %s\n", tmp->type, ctime, tmp->size, tmp->name);
    }

    if (node->iblock)
    {
        uint32_t* block = (uint32_t*)malloc(sb->block_size);

        pread(fd, block, sb->block_size, get_block_offset(sb, fd, node->iblock));
        for (i = 0; i < node->nlast; ++i)
        {
            inode_read(tmp, sb, fd, get_block_offset(sb, fd, block[i]));
            strncpy(ctime, asctime(tmp->ctime), 25);
            ctime[strlen(ctime) - 1] = 0;
            printf("\t%c %s %10d %s\n", tmp->type, ctime, tmp->size, tmp->name);
        }

        free(block);
    }

    inode_del(tmp);
}

void fs_cd(struct s_superblock* sb, int fd, struct s_inode** node, const char* name)
{
    int up = strncmp(name, "..", 2);
    if (up == 0 && (*node)->parent_inode == 0)
        return;

    struct s_inode* tmp;
    inode_init(&tmp, 0, 0, "", '\0');

    int not_found = 1;
    if (up)
    {
        inode_read(tmp, sb, fd, get_block_offset(sb, fd, (*node)->parent_inode));
        not_found = 0;
    }

    uint32_t i, len = ((*node)->iblock ? 12 : (*node)->nlast);
    for (i = 0; i < len && not_found; ++i)
    {
        inode_read(tmp, sb, fd, get_block_offset(sb, fd, (*node)->blocks[i]));
        not_found = strncmp(tmp->name, name, 32);
    }

    if ((*node)->iblock && not_found)
    {
        uint32_t* block = (uint32_t*)malloc(sb->block_size);

        pread(fd, block, sb->block_size, get_block_offset(sb, fd, (*node)->iblock));
        for (i = 0; i < (*node)->nlast && not_found; ++i)
        {
            inode_read(tmp, sb, fd, get_block_offset(sb, fd, block[i]));
            not_found = strncmp(tmp->name, name, 32);
        }

        free(block);
    }

    if (not_found)
        printf("cd: %s: no such directory\n", name);
    else
    {
        struct s_inode* sw = *node;
        *node = tmp;
        tmp = sw;
    }

    inode_del(tmp);
}

#endif // FS_H_INCLUDED
