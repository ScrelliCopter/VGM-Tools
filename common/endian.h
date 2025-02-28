#ifndef COMMON_ENDIAN_H
#define COMMON_ENDIAN_H

#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__DragonFly__)
# include <machine/endian.h>
#elif defined(__linux__) || defined(__CYGWIN__) || defined(__OpenBSD__)
# include <endian.h>
#elif defined(__NetBSD__) || defined(BSD)
# include <sys/endian.h>
#elif defined(__GNUC__)
# include <sys/param.h>
#else
# if defined(__ORDER_LITTLE_ENDIAN__)
#  define LITTLE_ENDIAN __ORDER_LITTLE_ENDIAN__
# else
#  define LITTLE_ENDIAN 1234
# endif
# if defined(__ORDER_BIG_ENDIAN__)
#  define BIG_ENDIAN    __ORDER_BIG_ENDIAN__
# else
#  define BIG_ENDIAN    4321
# endif
# if (defined(__BYTE_ORDER__) && __BYTE_ORDER__ == LITTLE_ENDIAN) || defined(__LITTLE_ENDIAN__)
#  define BYTE_ORDER LITTLE_ENDIAN
# elif (defined(__BYTE_ORDER__) && __BYTE_ORDER__ == BIG_ENDIAN) || defined(__BIG_ENDIAN__)
#  define BYTE_ORDER BIG_ENDIAN
# elif defined(_MSC_VER) && (defined(_WIN16) || defined(_WIN32) || defined(_WIN64) || defined(_XBOX))
#  ifdef _X360
#   define BYTE_ORDER BIG_ENDIAN
#  elif defined(_M_I86) || defined(_M_IX86) || defined(_M_X64) || defined(_M_AMD64) || defined(_M_IA64) \
     || defined(_M_ARM) || defined(_M_ARM64) || defined(_M_PPC) || defined(_M_MIPS) || defined(_M_ALPHA)
#   define BYTE_ORDER LITTLE_ENDIAN
#  endif
# elif defined(__MIPSEL__) || defined(__MIPSEL) || defined(_MIPSEL) \
    || defined(__ARMEL__) || defined(__THUMBEL__) || defined(__AARCH64EL__) \
    || defined(__i386__) || defined(__i386) || defined(__X86__) || defined(__I86__) || defined(__386) \
    || defined(__amd64__) || defined(__amd64) || defined(__x86_64__) || defined(__x86_64) \
    || defined(__INTEL__) || defined(__itanium__)
#  define BYTE_ORDER LITTLE_ENDIAN
# elif defined(__MIPSEB__) || defined(__MIPSEB) || defined(_MIPSEB) \
    || defined(__ARMEB__) || defined(__THUMBEB__) || defined(__AARCH64EB__) \
    || defined(__m68k__) || defined(mc68000) || defined(M68000) || defined(__MC68K__) || defined(_M_M68K) \
    || defined(__PPCGECKO__) || defined(__PPCBROADWAY__) || defined(_XENON) \
    || defined(__hppa__) || defined(__hppa) || defined(__HPPA__) || defined(__sparc_v8__) || defined(__sparcv8)
#  define BYTE_ORDER BIG_ENDIAN
# endif
#endif

#if !defined(BYTE_ORDER) || !defined(LITTLE_ENDIAN) || !defined(BIG_ENDIAN) || \
    !(BYTE_ORDER == LITTLE_ENDIAN || BYTE_ORDER == BIG_ENDIAN)
# error "Couldn't determine endianness or unsupported platform"
#endif

#ifdef _MSC_VER
# define swap64(X) _byteswap_uint64((X))
# define swap32(X) _byteswap_ulong((X))
# define swap16(X) _byteswap_ushort((X))
#elif (__GNUC__ == 4 && __GNUC_MINOR__ >= 8) || (__GNUC__ > 4)
# define swap64(X) __builtin_bswap32((X))
# define swap32(X) __builtin_bswap32((X))
# define swap16(X) __builtin_bswap16((X))
// Apparently smelly GCC 5 blows up on this test so this is done separately for Clang
#elif defined(__has_builtin) && __has_builtin(__builtin_bswap64) && __has_builtin(__builtin_bswap32) && __has_builtin(__builtin_bswap16)
# define swap64(X) __builtin_bswap64((X))
# define swap32(X) __builtin_bswap32((X))
# define swap16(X) __builtin_bswap16((X))
#else
static inline uint64_t swap64(uint64_t v)
{
	return v                 << 56ul |
	(v &           0xFF00ul) << 40ul |
	(v &         0xFF0000ul) << 24ul |
	(v &       0xFF000000ul) <<  8ul |
	(v &     0xFF00000000ul) >>  8ul |
	(v &   0xFF0000000000ul) >> 24ul |
	(v & 0xFF000000000000ul) >> 40ul |
	v                        >> 56ul;
	
}
static inline uint32_t swap32(uint32_t v)
{
	return v << 24u | (v & 0xFF00u) << 8u | (v & 0xFF0000u) >> 8u | v >> 24u;
}
static inline uint16_t swap16(uint16_t v)
{
	return v << 8u | v >> 8u;
}
#endif

#if BYTE_ORDER == LITTLE_ENDIAN
# define SWAP_LE64(V) (V)
# define SWAP_LE32(V) (V)
# define SWAP_LE16(V) (V)
# define SWAP_BE64(V) swap64((V))
# define SWAP_BE32(V) swap32((V))
# define SWAP_BE16(V) swap16((V))
#elif BYTE_ORDER == BIG_ENDIAN
# define SWAP_LE64(V) swap64((V))
# define SWAP_LE32(V) swap32((V))
# define SWAP_LE16(V) swap16((V))
# define SWAP_BE64(V) (V)
# define SWAP_BE32(V) (V)
# define SWAP_BE16(V) (V)
#endif

#endif//COMMON_ENDIAN_H
