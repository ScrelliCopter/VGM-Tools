#ifndef COMMON_H
#define COMMON_H

#ifdef HAVE_ENDIAN_H
# include <endian.h>
# define BYTE_ORDER    __BYTE_ORDER
# define LITTLE_ENDIAN __LITTLE_ENDIAN
# define BIG_ENDIAN    __BIG_ENDIAN
#else
# define LITTLE_ENDIAN 1234
# define BIG_ENDIAN    4321
# if defined( __APPLE__ ) || defined( _WIN32 )
#  define BYTE_ORDER LITTLE_ENDIAN
# else
#  error Can't detect endianness
# endif
#endif

static inline uint32_t swap32(uint32_t v)
{
	return (v << 24) | ((v << 8) & 0x00FF0000) | ((v >> 8) & 0x0000FF00) | (v >> 24);
}

static inline uint16_t swap16(uint16_t v)
{
	return (v << 8) | (v >> 8);
}

#if BYTE_ORDER == LITTLE_ENDIAN
# define SWAP_LE32(V) (V)
# define SWAP_LE16(V) (V)
# define SWAP_BE32(V) swap32((uint32_t)V)
# define SWAP_BE16(V) swap16((uint16_t)V)
#elif BYTE_ORDER == BIG_ENDIAN
# define SWAP_LE32(V) swap32((uint32_t)V)
# define SWAP_LE16(V) swap16((uint16_t)V)
# define SWAP_BE32(V) (V)
# define SWAP_BE16(V) (V)
#endif

#define MIN(A, B) (((A) < (B)) ? (A) : (B))
#define MAX(A, B) (((A) > (B)) ? (A) : (B))

#endif//COMMON_H
