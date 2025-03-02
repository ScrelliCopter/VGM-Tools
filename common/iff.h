#ifndef IFF_H
#define IFF_H

typedef union
{
	char c[4];
	uint32_t i;

} IffFourCC;

#define IFF_FOURCC(A, B, C, D) (IffFourCC){ .c = { A, B, C, D } }
#define IFF_FOURCC_CMP(L, R) ((L).i == (R).i)

#endif//IFF_H
