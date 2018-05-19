#ifndef BITMAP_H_INCLUDED
#define BITMAP_H_INCLUDED

#include <stdint.h>
#include <stdlib.h>
#include <limits.h>
#include <memory.h>
#include "options.h"
#include "utils.h"
#include "assert.h"
#include "superblock.h"

#define BYTE 8
#define BUFSIZE 1024

void bitmap_write(int fd, uint32_t blocks_used, uint32_t blocks_total)
{
    uint32_t cells = (blocks_used + BYTE - 1) >> 3;
    uint32_t rest = (blocks_total >> 3) - cells;

    uint8_t buf[BUFSIZE] = { 255 };

    for (; cells >= BUFSIZE; cells -= BUFSIZE)
        write(fd, buf, sizeof(buf));

    for (; cells > 1; --cells)
        write(fd, buf, sizeof(uint8_t));

    uint32_t sh = BYTE - mod_base2(blocks_used, BYTE);
    if (sh < BYTE)
        buf[0] = (buf[0] >> sh) << sh;

    write(fd, buf, sizeof(uint8_t));

    memset(buf, 0, sizeof(buf));
    for (; rest >= BUFSIZE; rest -= BUFSIZE)
        write(fd, buf, sizeof(buf));

    for (; rest; --rest)
        write(fd, buf, sizeof(uint8_t));
}

uint32_t bitmap_get_cell_offset(struct s_superblock* sb, int fd, uint32_t nblock)
{
    return sizeof(struct s_superblock) + (nblock >> 3);
}

uint8_t bitmap_get_cell(struct s_superblock* sb, int fd, uint32_t nblock)
{
    uint8_t cell;
    uint32_t offset = bitmap_get_cell_offset(sb, fd, nblock);

    pread(fd, &cell, sizeof(uint8_t), offset);
    return cell;
}

uint8_t bitmap_block_is_unavailable(struct s_superblock* sb, int fd, uint32_t nblock)
{
    if (nblock >= sb->blocks_total)
        return 1;

    uint8_t cell = bitmap_get_cell(sb, fd, nblock);
    uint8_t r = mod_base2(nblock, BYTE);

    return cell & (1 << (BYTE - r - 1));
}

uint32_t bitmap_get_available_block(struct s_superblock* sb, int fd)
{
    if (sb->blocks_remain == 0)
        return 0;

    uint8_t buf[BUFSIZE];
    for (int k = sizeof(struct s_superblock); k < (sb->blocks_total >> 3) + sizeof(struct s_superblock); k += sizeof(buf))
    {
        pread(fd, buf, sizeof(buf), k);
        for (int i = 0; i < BUFSIZE; ++i)
            for (int j = 0; j < BYTE; ++j)
                if (!get8_bit(buf[i], j))
                    return ((k - sizeof(struct s_superblock) + i) << 3) + j;
    }
}

void bitmap_set_unavailable(struct s_superblock* sb, int fd, uint32_t nblock)
{
    uint8_t cell;
    uint32_t offset = bitmap_get_cell_offset(sb, fd, nblock);

    pread(fd, &cell, sizeof(uint8_t), offset);

    uint8_t rest = mod_base2(nblock, BYTE);
    cell |= 1 << (BYTE - rest - 1);

    pwrite(fd, &cell, sizeof(uint8_t), offset);

    --(sb->blocks_remain);
}

void bitmap_set_available(struct s_superblock* sb, int fd, uint32_t nblock)
{
    uint8_t cell;
    uint32_t offset = bitmap_get_cell_offset(sb, fd, nblock);

    pread(fd, &cell, sizeof(uint8_t), offset);

    uint8_t rest = mod_base2(nblock, BYTE);
    cell &= ~(1 << (BYTE - rest - 1));

    pwrite(fd, &cell, sizeof(uint8_t), offset);

    ++(sb->blocks_remain);
}


#endif // BITMAP_H_INCLUDED
