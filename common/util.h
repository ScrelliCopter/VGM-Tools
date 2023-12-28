#ifndef COMMON_UTIL_H
#define COMMON_UTIL_H

#define MIN(A, B) (((A) < (B)) ? (A) : (B))
#define MAX(A, B) (((A) > (B)) ? (A) : (B))
#define CLAMP(X, A, B) (MIN(MAX((X), (A)), (B)))

#if (defined(__GNUC__) && (__GNUC__ >= 4)) || defined(__clang__)
# define FORCE_INLINE inline __attribute__((always_inline))
#elif defined(_MSC_VER)
# define FORCE_INLINE __forceinline
#else
# define FORCE_INLINE inline
#endif

#endif//COMMON_UTIL_H
