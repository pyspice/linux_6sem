#ifndef UTILS_H_INCLUDED
#define UTILS_H_INCLUDED

#include <stdint.h>
#include <ctype.h>

uint8_t log2_ceil(uint32_t n)
{
    uint8_t i;
    for (i = 0; n; ++i)
        n >>= 1;
    return i;
}

uint32_t get_block_offset(struct s_superblock* sb, int fd, uint32_t nblock)
{
    return (sb->root_block + nblock) * sizeof(sb->block_size);
}

#define mod_base2(n, base2) (n & (base2 - 1))
#define get8_bit(n, nbit) (n & (1 << (8 - nbit - 1)))

#endif // UTILS_H_INCLUDED
