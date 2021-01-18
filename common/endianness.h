/** 
* @file
* @brief  Convert Endianness of shorts, longs, long longs, regardless of architecture/OS
*
* Defines (without pulling in platform-specific network include headers):
* bswap16, bswap32, bswap64, ntoh16, hton16, ntoh32 hton32, ntoh64, hton64
*
* Should support linux / macos / solaris / windows.
* Supports GCC (on any platform, including embedded), MSVC2015, and clang,
* and should support intel, solaris, and ibm compilers as well.
*
* Released under MIT license
* 
* Based on https://gist.github.com/jtbr/7a43e6281e6cca353b33ee501421860c
*/

#ifndef ENDIANNESS_H
#define ENDIANNESS_H

#include <stdlib.h>
#include <stdint.h>

/* Detect platform endianness at compile time */

/* When available, these headers can improve platform endianness detection */
#ifdef __has_include // C++17, supported as extension to C++11 in clang, GCC 5+, vs2015
#if __has_include(<endian.h>)
#include <endian.h> // gnu libc normally provides, linux
#elif __has_include(<machine/endian.h>)
#include <machine/endian.h> //open bsd, macos
#elif __has_include(<sys/param.h>)
#include <sys/param.h> // mingw, some bsd (not open/macos)
#elif __has_include(<sys/isadefs.h>)
#include <sys/isadefs.h> // solaris
#endif
#endif

#if !defined(__LITTLE_ENDIAN__) && !defined(__BIG_ENDIAN__)
#if (defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)                                                \
    || (defined(__BYTE_ORDER) && __BYTE_ORDER == __BIG_ENDIAN) || (defined(_BYTE_ORDER) && _BYTE_ORDER == _BIG_ENDIAN) \
    || (defined(BYTE_ORDER) && BYTE_ORDER == BIG_ENDIAN)                                                               \
    || (defined(__sun) && defined(__SVR4) && defined(_BIG_ENDIAN)) || defined(__ARMEB__) || defined(__THUMBEB__)       \
    || defined(__AARCH64EB__) || defined(_MIBSEB) || defined(__MIBSEB) || defined(__MIBSEB__) || defined(_M_PPC)
#define __BIG_ENDIAN__
#elif (defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__) || /* gcc */                              \
    (defined(__BYTE_ORDER) && __BYTE_ORDER == __LITTLE_ENDIAN)                  /* linux header */                     \
    || (defined(_BYTE_ORDER) && _BYTE_ORDER == _LITTLE_ENDIAN)                                                         \
    || (defined(BYTE_ORDER) && BYTE_ORDER == LITTLE_ENDIAN)              /* mingw header */                            \
    || (defined(__sun) && defined(__SVR4) && defined(_LITTLE_ENDIAN)) || /* solaris */                                 \
    defined(__ARMEL__) || defined(__THUMBEL__) || defined(__AARCH64EL__) || defined(_MIPSEL) || defined(__MIPSEL)      \
    || defined(__MIPSEL__) || defined(_M_IX86) || defined(_M_X64) || defined(_M_IA64)                                  \
    ||              /* msvc for intel processors */                                                                    \
    defined(_M_ARM) /* msvc code on arm executes in little endian mode */
#define __LITTLE_ENDIAN__
#endif
#endif

/* Undefine these macros if the platform header already defined them */
#ifdef bswap16
#undef bswap16
#endif

#ifdef bswap32
#undef bswap32
#endif

#ifdef bswap64
#undef bswap64
#endif

/* Check if we have the facility to check if a compiler supports a given builtin function */
#ifndef __has_builtin
#define __has_builtin(x) 0
#endif

/* Define unconditional byte-swap functions, using fast processor-native built-ins where possible */
#if defined(_MSC_VER) /* needs to be first because msvc doesn't short-circuit after failing defined(__has_builtin) */
#define bswap16(x) _byteswap_ushort((x))
#define bswap32(x) _byteswap_ulong((x))
#define bswap64(x) _byteswap_uint64((x))
#else

#if (__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 8) || __has_builtin(__builtin_bswap16)
#define bswap16(x) __builtin_bswap16((x))
#endif

#if (__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 3) || __has_builtin(__builtin_bswap32)
#define bswap32(x) __builtin_bswap32((x))
#endif

#if (__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 3) || __has_builtin(__builtin_bswap64)
#define bswap64(x) __builtin_bswap64((x))
#endif

#endif

/* even in this case, compilers often optimize by using native instructions */
#ifndef bswap16
static inline uint16_t bswap16(uint16_t x)
{
    return (((x >> 8) & 0xffu) | ((x & 0xffu) << 8));
}
#endif

#ifndef bswap32
static inline uint32_t bswap32(uint32_t x)
{
    return (((x & 0xff000000u) >> 24) | ((x & 0x00ff0000u) >> 8) | ((x & 0x0000ff00u) << 8)
            | ((x & 0x000000ffu) << 24));
}
#endif

#ifndef bswap64
static inline uint64_t bswap64(uint64_t x)
{
    return (((x & 0xff00000000000000ull) >> 56) | ((x & 0x00ff000000000000ull) >> 40)
            | ((x & 0x0000ff0000000000ull) >> 24) | ((x & 0x000000ff00000000ull) >> 8)
            | ((x & 0x00000000ff000000ull) << 8) | ((x & 0x0000000000ff0000ull) << 24)
            | ((x & 0x000000000000ff00ull) << 40) | ((x & 0x00000000000000ffull) << 56));
}
#endif

/* Check if we have the facility to check if a compiler supports a given attribute */
#ifndef __has_attribute
#define __has_attribute(x) 0
#endif

/* Convert 32-bit float from host to network byte order */
static inline float bswapf(float f)
{
    /* We need to do some type punning, so create types that can violate strict aliasing */
#if __has_attribute(__may_alias__)
    typedef float __attribute__((__may_alias__)) float_a;
    typedef uint32_t __attribute__((__may_alias__)) uint32_a;
#else
    /* MSVC doesn't currently enforce strict aliasing and thus has no extension to disable it. */
    typedef float float_a;
    typedef uint32_t uint32_a;
#endif

    uint32_t val = bswap32(*(const uint32_a*)(&f));
    return *((float_a*)(&val));
}

static inline double bswapd(double f)
{
    /* We need to do some type punning, so create types that can violate strict aliasing */
#if __has_attribute(__may_alias__)
    typedef double __attribute__((__may_alias__)) double_a;
    typedef uint64_t __attribute__((__may_alias__)) uint64_a;
#else
    /* MSVC doesn't currently enforce strict aliasing and thus has no extension to disable it. */
    typedef double double_a;
    typedef uint64_t uint64_a;
#endif

    uint64_t val = bswap64(*(const uint64_a*)(&f));
    return *((double_a*)(&val));
}

/* Defines byte swaps as needed depending upon platform endianness */
#if !defined(htobe32) /* Assume all are defined if this is. */
#if defined(__LITTLE_ENDIAN__)
#define htobe16(x) bswap16(x)
#define htole16(x) (x)
#define be16toh(x) bswap16(x)
#define le16toh(x) (x)

#define htobe32(x) bswap32(x)
#define htole32(x) (x)
#define be32toh(x) bswap32(x)
#define le32toh(x) (x)

#define htobe64(x) bswap64(x)
#define htole64(x) (x)
#define be64toh(x) bswap64(x)
#define le64toh(x) (x)

#define beftoh(x) bswapf(x)
#define leftoh(x) (x)

#define bedtoh(x) bswapd(x)
#define ledtoh(x) (x)
#elif defined(__BIG_ENDIAN__)
#define htobe16(x) (x)
#define htole16(x) bswap16(x)
#define be16toh(x) (x)
#define le16toh(x) bswap16(x)

#define htobe32(x) (x)
#define htole32(x) bswap32(x)
#define be32toh(x) (x)
#define le32toh(x) bswap32(x)

#define htobe64(x) (x)
#define htole64(x) bswap64(x)
#define be64toh(x) (x)
#define le64toh(x) bswap64(x)

#define beftoh(x) (x)
#define leftoh(x) bswapf(x)

#define bedtoh(x) (x)
#define ledtoh(x) bswapd(x)
#else
#warning "Unknown Platform / endianness; network / host byte swaps not defined."
#endif
#endif

/* Some aliases similar to the standard sockets byte swaps for network order transmission. */
#define ntoh16(x) be16toh((x))
#define hton16(x) htobe16((x))
#define ntoh32(x) be32toh((x))
#define hton32(x) htobe32((x))
#define ntoh64(x) be64toh((x))
#define hton64(x) htobe64((x))

#endif /* ENDIANNESS_H */
