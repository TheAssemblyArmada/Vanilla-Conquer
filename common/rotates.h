/**
 * @file
 * @brief Bitwise rotate functions that compile to near optimum asm on most optimising compilers.
 */
#ifndef ROTATES_H
#define ROTATES_H

#include <stdint.h>

static inline uint8_t rotl8(uint8_t a, unsigned b)
{
    b &= 7;
    return (a << b) | (a >> (8 - b));
}

static inline uint8_t rotr8(uint8_t a, unsigned b)
{
    b &= 7;
    return (a >> b) | (a << (8 - b));
}

static inline uint16_t rotl16(uint16_t a, unsigned b)
{
    b &= 15;
    return (a << b) | (a >> (16 - b));
}

static inline uint16_t rotr16(uint16_t a, unsigned b)
{
    b &= 15;
    return (a >> b) | (a << (16 - b));
}

static inline uint32_t rotl32(uint32_t a, unsigned b)
{
    b &= 31;
    return (a << b) | (a >> (32 - b));
}

static inline uint32_t rotr32(uint32_t a, unsigned b)
{
    b &= 31;
    return (a >> b) | (a << (32 - b));
}

static inline uint64_t rotl64(uint64_t a, unsigned b)
{
    b &= 63;
    return (a << b) | (a >> (64 - b));
}

static inline uint64_t rotr64(uint64_t a, unsigned b)
{
    b &= 63;
    return (a >> b) | (a << (64 - b));
}

#endif /* ROTATES_H */
