#ifndef IFF_H
#define IFF_H

#include <stdint.h>
#include <stddef.h>

static inline size_t iffRealSize(uint32_t size) { return (uint32_t)size + ((uint32_t)size & 0x1U); }

#define IFF_NEEDS_PAD(S) (((S) & 0x1) == 0x1)

typedef union
{
	char c[4];
	uint32_t i;

} IffFourCC;

#define FOURCC_FORM IFF_FOURCC('F', 'O', 'R', 'M')
#define FOURCC_RIFF IFF_FOURCC('R', 'I', 'F', 'F')

#define IFF_FOURCC(A, B, C, D) (IffFourCC){ .c = { A, B, C, D } }
#define IFF_FOURCC_CMP(L, R) ((L).i == (R).i)

#define IFF_CHUNK_HEAD_SIZE 8
typedef struct
{
	IffFourCC fourcc;
	uint32_t  size;

} IffChunk;

#endif//IFF_H
