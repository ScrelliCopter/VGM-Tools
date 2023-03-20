#ifndef COMMON_ENDIAN_H
#define COMMON_ENDIAN_H

#ifdef __APPLE__
# include <machine/endian.h>
#elif defined( __linux__ ) || defined( __CYGWIN__ ) || defined( __OpenBSD__ )
# include <endian.h>
#elif defined( __NetBSD__ ) || defined( __FreeBSD__ ) || defined( __DragonFly__ )
# include <sys/endian.h>
# ifdef __FreeBSD__
#  define LITTLE_ENDIAN _LITTLE_ENDIAN
#  define BIG_ENDIAN    _BIG_ENDIAN
#  define BYTE_ORDER    _BYTE_ORDER
# endif
#elif defined( _MSC_VER ) || defined( _WIN16 ) || defined( _WIN32 ) || defined( _WIN64 )
# ifdef _MSC_VER
#  define LITTLE_ENDIAN 1234
#  define BIG_ENDIAN    4321
#  if defined( _M_IX86 ) || defined( _M_X64 ) || defined( _M_AMD64 ) || defined( _M_IA64 )
#   define BYTE_ORDER LITTLE_ENDIAN
#  elif defined( _M_PPC )
// Probably not reliable but eh
#   define BYTE_ORDER BIG_ENDIAN
#  endif
# elif defined( __GNUC__ )
#  include <sys/param.h>
# endif
#endif

#if !defined( BYTE_ORDER ) || !defined( LITTLE_ENDIAN ) || !defined( BIG_ENDIAN ) || \
    !( BYTE_ORDER == LITTLE_ENDIAN || BYTE_ORDER == BIG_ENDIAN )
# error "Couldn't determine endianness or unsupported platform"
#endif

#ifdef _MSC_VER
# define swap32(X) _byteswap_ulong((X))
# define swap16(X) _byteswap_ushort((X))
#elif ( __GNUC__ == 4 && __GNUC_MINOR__ >= 8 ) || ( __GNUC__ > 4 )
# pragma message("CLion penis")
# define swap32(X) __builtin_bswap32((X))
# define swap16(X) __builtin_bswap16((X))
// Apparently smelly GCC 5 blows up on this test so this is done separately for Clang
#elif defined( __has_builtin ) && __has_builtin( __builtin_bswap32 ) && __has_builtin( __builtin_bswap16 )
# define swap32(X) __builtin_bswap32((X))
# define swap16(X) __builtin_bswap16((X))
#else
static inline uint32_t swap32(uint32_t v)
{
	return (v << 24U) | ((v << 8U) & 0x00FF0000U) | ((v >> 8U) & 0x0000FF00U) | (v >> 24U);
}
static inline uint16_t swap16(uint16_t v)
{
	return (v << 8U) | (v >> 8U);
}
#endif

#if BYTE_ORDER == LITTLE_ENDIAN
# define SWAP_LE32(V) (V)
# define SWAP_LE16(V) (V)
# define SWAP_BE32(V) swap32((V))
# define SWAP_BE16(V) swap16((V))
#elif BYTE_ORDER == BIG_ENDIAN
# define SWAP_LE32(V) swap32((V))
# define SWAP_LE16(V) swap16((V))
# define SWAP_BE32(V) (V)
# define SWAP_BE16(V) (V)
#endif

#endif//COMMON_ENDIAN_H
