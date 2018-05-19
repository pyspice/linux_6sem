#ifndef FS_H_INCLUDED
#define FS_H_INCLUDED

#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "superblock.h"
#include "inode.h"
#include "utils.h"
#include "bitmap.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

void fs_update_ancestors_size(struct s_superblock* sb, int fd, struct s_inode* node, int32_t size)
{
    struct s_inode* tmp;
    inode_init(&tmp, 0, 0, "", 0);
    inode_copy(tmp, node);

    uint32_t offset;
    while (tmp->parent_inode != 0)
    {
        offset = get_block_offset(sb, tmp->parent_inode);
        inode_read(tmp, sb, fd, offset);
        tmp->size += size;
        inode_write(tmp, sb, fd, offset);
    }

    inode_del(tmp);
}

void fs_add_ninode(struct s_superblock* sb, int fd, struct s_inode* node, uint32_t nblock)
{
    if (node->iblock == 0 && node->nlast < 12)
    {
        node->blocks[node->nlast] = nblock;
        ++(node->nlast);
    }
    else
    {
        uint32_t offset;
        uint32_t* block = (uint32_t*)malloc(sb->block_size);

        if (node->iblock == 0)
        {
            uint32_t iblock = bitmap_get_available_block(sb, fd);
            bitmap_set_unavailable(sb, fd, iblock);

            offset = get_block_offset(sb, iblock);

            node->iblock = iblock;
            node->nlast = 1;

            block[0] = nblock;
        }
        else
        {
            offset = get_block_offset(sb, node->iblock);
            pread(fd, block, sb->block_size, offset);

            block[node->nlast] = nblock;
            ++(node->nlast);
        }

        pwrite(fd, block, sb->block_size, offset);
        free(block);
    }
}

uint32_t fs_find_ninode(struct s_superblock* sb, int fd, struct s_inode* node, const char* name)
{
    struct s_inode* tmp;
    inode_init(&tmp, 0, 0, "", '\0');

    uint32_t i, nblock = 0, len = (node->iblock ? 12 : node->nlast);
    int not_found = 1;
    for (i = 0; (i < len) && not_found; ++i)
    {
        inode_read(tmp, sb, fd, get_block_offset(sb, node->blocks[i]));
        not_found = strncmp(tmp->name, name, NAME_LEN);
    }

    if (node->iblock && not_found)
    {
        uint32_t* block = (uint32_t*)malloc(sb->block_size);

        pread(fd, block, sb->block_size, get_block_offset(sb, node->iblock));
        for (i = 0; (i < node->nlast) && not_found; ++i)
        {
            inode_read(tmp, sb, fd, get_block_offset(sb, block[i]));
            not_found = strncmp(tmp->name, name, NAME_LEN);
        }

        free(block);
    }

    if (not_found == 0)
        nblock = tmp->ninode;

    inode_del(tmp);

    return nblock;
}

void fs_mkdir(struct s_superblock* sb, int fd, struct s_inode* node, const char* name)
{
    if (strlen(name) == 0)
    {
        puts("mkdir: empty directory name");
        return;
    }

    if ((sb->blocks_remain == 0) || (sb->blocks_remain == 1 && node->nlast == 12 && node->iblock == 0))
    {
        puts("mkdir: cannot create directory: no available space");
        return;
    }

    if (node->iblock != 0 && node->nlast == sb->block_size / sizeof(uint32_t))
    {
        puts("mkdir: cannot create directory: max number of subdirectories/files reached");
        return;
    }

    uint32_t nblock = fs_find_ninode(sb, fd, node, name);
    if (nblock)
    {
        puts("mkdir: cannot create directory: object exists");
        return;
    }

    nblock = bitmap_get_available_block(sb, fd);
    printf("%d\n", nblock);
    bitmap_set_unavailable(sb, fd, nblock);

    struct s_inode* dir;
    inode_init(&dir, nblock, node->ninode, name, 'd');

    inode_write(dir, sb, fd, get_block_offset(sb, nblock));

    fs_add_ninode(sb, fd, node, nblock);
    inode_write(node, sb, fd, get_block_offset(sb, node->ninode));

    fs_update_ancestors_size(sb, fd, dir, dir->size);

    inode_del(dir);
}

void fs_ls(struct s_superblock* sb, int fd, struct s_inode* node)
{
    if (node->iblock == 0 && node->nlast == 0)
        return;

    struct s_inode* tmp;
    inode_init(&tmp, 0, 0, "", '\0');
    uint32_t i, len = (node->iblock ? 12 : node->nlast);

    for (i = 0; i < len; ++i)
    {
        inode_read(tmp, sb, fd, get_block_offset(sb, node->blocks[i]));

        printf("\t%c %s %10d %s\n", tmp->type, tmp->crtime, tmp->size, tmp->name);
    }

    if (node->iblock)
    {
        uint32_t* block = (uint32_t*)malloc(sb->block_size);

        pread(fd, block, sb->block_size, get_block_offset(sb, node->iblock));
        for (i = 0; i < node->nlast; ++i)
        {
            inode_read(tmp, sb, fd, get_block_offset(sb, block[i]));

            printf("\t%c %s %10d %s\n", tmp->type, tmp->crtime, tmp->size, tmp->name);
        }

        free(block);
    }

    inode_del(tmp);
}

uint32_t fs_cd(struct s_superblock* sb, int fd, struct s_inode* node, const char* name)
{
    if (strlen(name) == 0)
    {
        puts("cd: empty directory name");
        return 0;
    }

    int up = strncmp(name, "..", 2);
    if (up == 0 && node->parent_inode == 0)
        return 1;

    struct s_inode* tmp;
    inode_init(&tmp, 0, 0, "", '\0');

    uint32_t nblock = 0;
    if (up == 0)
    {
        inode_read(tmp, sb, fd, get_block_offset(sb, node->parent_inode));
        nblock = 1;
    }
    else if ((nblock = fs_find_ninode(sb, fd, node, name)) != 0)
        inode_read(tmp, sb, fd, get_block_offset(sb, nblock));

    if (nblock == 0 || tmp->type != 'd')
    {
        printf("cd: %s: no such directory\n", name);
        nblock = 0;
    }
    else
        inode_copy(node, tmp);

    inode_del(tmp);

    return nblock;
}

void fs_erase_file(struct s_superblock* sb, int fd, struct s_inode* node)
{
    uint32_t i, len = (node->iblock ? 12 : node->nlast);
    for (i = 0; i < len; ++i)
        bitmap_set_available(sb, fd, node->blocks[i]);

    if (node->iblock)
    {
        bitmap_set_available(sb, fd, node->iblock);
        uint32_t* block = (uint32_t*)malloc(sb->block_size);

        pread(fd, block, sb->block_size, get_block_offset(sb, node->iblock));
        for (i = 0; i < node->nlast; ++i)
            bitmap_set_available(sb, fd, block[i]);

        free(block);
    }
}

void fs_rm(struct s_superblock* sb, int fd, struct s_inode* node, const char* name)
{
    if (strlen(name) == 0)
    {
        puts("rm: empty object name");
        return;
    }

    struct s_inode* tmp;
    inode_init(&tmp, 0, 0, "", '\0');

    uint32_t nblock = fs_find_ninode(sb, fd, node, name);
    if (nblock == 0)
        printf("rm: %s: no such object\n", name);
    else
    {
        inode_read(tmp, sb, fd, get_block_offset(sb, nblock));

        if (tmp->type == '-')
            fs_erase_file(sb, fd, tmp);
        else if (tmp->nlast != 0)
        {
            puts("rm: cannot remove directory: directory is not empty");
            inode_del(tmp);
            return;
        }

        bitmap_set_available(sb, fd, nblock);

        uint32_t i, offset, len = (node->iblock ? 12 : node->nlast);
        for (i = 0; i < len && node->blocks[i] != nblock; ++i);

        uint32_t* block;
        if (node->iblock)
        {
            block = (uint32_t*)malloc(sb->block_size);
            offset = get_block_offset(sb, node->iblock);
            pread(fd, block, sb->block_size, offset);
        }

        if (i == len)
        {
            for (i = 0; i < node->nlast && block[i] != nblock; ++i);
            for (; i < node->nlast - 1; ++i)
                block[i] = block[i + 1];
        }
        else
        {
            for (; i < len - 1; ++i)
                node->blocks[i] = node->blocks[i + 1];

            if (node->iblock)
            {
                node->blocks[11] = block[0];
                for (i = 0; i < node->nlast; ++i)
                    block[i] = block[i + 1];
            }
        }

        --(node->nlast);

        if (node->iblock)
        {
            if (node->nlast == 0)
            {
                bitmap_set_available(sb, fd, node->iblock);
                node->iblock = 0;
                node->nlast = 12;
            }
            else
                pwrite(fd, block, sb->block_size, offset);
            free(block);
        }

        inode_write(node, sb, fd, get_block_offset(sb, node->ninode));

        fs_update_ancestors_size(sb, fd, tmp, -(tmp->size));
    }

    inode_del(tmp);
}

uint32_t fs_get_available_file_space(struct s_superblock* sb)
{
    uint32_t primary = sb->blocks_remain - 1;
    if (primary > 12)
        primary = 12;

    uint32_t secondary = sb->blocks_remain - primary - 2;
    if (secondary < 0)
        secondary = 0;
    if (secondary > sb->block_size / sizeof(uint32_t))
        secondary = sb->block_size / sizeof(uint32_t);

    return (primary + secondary) * sb->block_size;
}

int fs_pull(struct s_superblock* sb, int fd, struct s_inode* node, const char* from, const char* to)
{
    if (strlen(from) == 0 || strlen(to) == 0)
    {
        puts("pull: empty file name(s)");
        return 0;
    }

    if (fs_find_ninode(sb, fd, node, to))
    {
        printf("pull: cannot pull file %s: object exists\n", to);
        return 0;
    }

    int ifd = open(from, O_RDONLY);
    if (ifd == -1)
    {
        printf("pull: cannot open file %s\n", from);
        return 0;
    }

    struct stat st;
    fstat(ifd, &st);

    uint32_t space = fs_get_available_file_space(sb);
    if (st.st_size > space)
    {
        if (st.st_size > (sb->block_size / sizeof(uint32_t) + 12) * sb->block_size)
            printf("pull: cannot pull file %s: file too large\n", from);
        else
            printf("pull: cannot pull file %s: no available space\n", from);
        close(ifd);
        return 0;
    }

    uint32_t nblock, tblock = bitmap_get_available_block(sb, fd);
    bitmap_set_unavailable(sb, fd, tblock);

    struct s_inode* tmp;
    inode_init(&tmp, tblock, node->ninode, to, '-');
    tmp->size += st.st_size;

    fs_add_ninode(sb, fd, node, tblock);

    char* block = (char*)malloc(sb->block_size);
    int bytes = read(ifd, block, sb->block_size);
    while (bytes > 0)
    {
        nblock = bitmap_get_available_block(sb, fd);
        bitmap_set_unavailable(sb, fd, nblock);

        pwrite(fd, block, bytes, get_block_offset(sb, nblock));
        fs_add_ninode(sb, fd, tmp, nblock);

        bytes = read(ifd, block, sb->block_size);
    }

    inode_write(tmp, sb, fd, get_block_offset(sb, tblock));
    inode_write(node, sb, fd, get_block_offset(sb, node->ninode));

    fs_update_ancestors_size(sb, fd, tmp, tmp->size);

    close(ifd);
    inode_del(tmp);
    free(block);

    return 1;
}

uint32_t fs_push(struct s_superblock* sb, int fd, struct s_inode* node, const char* from, const char* to)
{
    if (strlen(from) == 0 || strlen(to) == 0)
    {
        puts("push: empty file name(s)");
        return 0;
    }

    uint32_t nblock = fs_find_ninode(sb, fd, node, from);

    struct s_inode* tmp;
    inode_init(&tmp, 0, 0, "", 0);
    if (nblock)
        inode_read(tmp, sb, fd, get_block_offset(sb, nblock));

    if (nblock == 0 || tmp->type != '-')
    {
        printf("push: cannot push file %s: file does not exist\n");
        inode_del(tmp);
        return 0;
    }

    int ofd = open(from, O_CREAT | O_WRONLY, 0666);
    if (ofd == -1)
    {
        printf("push: cannot open file %s\n", to);
        inode_del(tmp);
        return 0;
    }

    char* block = (char*)malloc(sb->block_size);
    uint32_t i, len = (tmp->iblock ? 12 : tmp->nlast);
    int32_t size = tmp->size - sb->block_size;
    for (i = 0; (i < len) && (size > 0); ++i)
    {
        pread(fd, block, sb->block_size, get_block_offset(sb, tmp->blocks[i]));
        write(ofd, block, (size < sb->block_size ? size : sb->block_size));
        size -= sb->block_size;
    }

    if (tmp->iblock && size > 0)
    {
        uint32_t* iblock = (uint32_t*)malloc(sb->block_size);
        pread(fd, iblock, sb->block_size, get_block_offset(sb, tmp->iblock));

        for (i = 0; i < tmp->nlast && size > 0; ++i)
        {
            pread(fd, block, sb->block_size, get_block_offset(sb, iblock[i]));
            write(ofd, block, (size < sb->block_size ? size : sb->block_size));
            size -= sb->block_size;
        }

        free(iblock);
    }

    inode_del(tmp);
    close(ofd);
    free(block);

    return 1;
}

#endif // FS_H_INCLUDED
