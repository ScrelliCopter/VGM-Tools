#ifndef STREAM_H
#define STREAM_H

#include <stddef.h>
#include <stdbool.h>
#include <assert.h>
#include "endian.h"

typedef enum
{
	STREAM_SEEK_SET = 0, // From start of file
	STREAM_SEEK_CUR = 1, // From current position in file
	STREAM_SEEK_END = 2  // From EOF
} StreamWhence;

typedef struct StreamIoCb
{
	// Basic read, required for readable streams
	size_t (*read)(void* restrict user, void* restrict out, size_t size, size_t num);
	// Basic write, required for writable streams
	size_t (*write)(void* restrict user, const void* restrict src, size_t size, size_t num);
	// fgetc, optional
	int    (*getc)(void* restrict user);
	// fputc, optional
	int    (*putc)(void* restrict user, int c);
	// Stream seek, optional, may only support STREAM_SEEK_CUR
	bool   (*seek)(void* restrict user, long offset, StreamWhence whence);
	// Get stream position, optional
	bool   (*tell)(void* restrict user, size_t* restrict outPosition);
	// True if stream has reached end-of-file, optional
	bool   (*eof)(void* restrict user);
	// True if stream has encountered an error
	bool   (*error)(void* restrict user);
	// Close & free stream, optional if stream has no hidden state to clean up
	void   (*close)(void* restrict user);
} StreamIoCb;

typedef struct StreamHandle
{
	void* restrict user;
	const StreamIoCb* restrict cb;

} StreamHandle;


// Stream user interface

inline size_t streamRead(StreamHandle hnd, void *restrict out, size_t size, size_t count)
{
	assert(hnd.cb && hnd.cb->read && out);
	return hnd.cb->read(hnd.user, out, size, count);
}

inline size_t streamWrite(StreamHandle hnd, const void *restrict in, size_t size, size_t count)
{
	assert(hnd.cb && hnd.cb->write && in);
	return hnd.cb->write(hnd.user, in, size, count);
}

inline int streamGetC(StreamHandle hnd)
{
	assert(hnd.cb);
	if (hnd.cb->getc)
		return hnd.cb->getc(hnd.user);
	assert(hnd.cb->read);
	unsigned char uc;
	return (hnd.cb->read(hnd.user, (void*)&uc, 1, 1) == 1) ? (int)uc : -1;
}

inline int streamPutC(StreamHandle hnd, int c)
{
	assert(hnd.cb);
	if (hnd.cb->putc)
		return hnd.cb->putc(hnd.user, c);
	assert(hnd.cb->write);
	const unsigned char u = (unsigned char)c;
	return (hnd.cb->write(hnd.user, (const void*)&u, 1, 1) == 1) ? (int)c : -1;
}

inline bool streamSkip(StreamHandle hnd, long offset)
{
	assert(hnd.cb);
	if (hnd.cb->seek)
		return hnd.cb->seek(hnd.user, offset, STREAM_SEEK_CUR);
	assert(hnd.cb->read);
	uint8_t tmp;
	for (long i = 0; i < offset; ++i)
		hnd.cb->read(hnd.user, &tmp, 1, 1);
	return true;
}

inline bool streamSeek(StreamHandle hnd, long offset, StreamWhence whence)
{
	assert(hnd.cb);
	return hnd.cb->seek && hnd.cb->seek(hnd.user, offset, whence);
}

inline bool streamTell(StreamHandle hnd, size_t* restrict outPosition)
{
	assert(hnd.cb && outPosition);
	return hnd.cb->tell && hnd.cb->tell(hnd.user, outPosition);
}

inline bool streamEOF(StreamHandle hnd)
{
	assert(hnd.cb);
	return hnd.cb->eof && hnd.cb->eof(hnd.user);
}

inline bool streamError(StreamHandle hnd)
{
	assert(hnd.cb);
	return hnd.cb->error && hnd.cb->error(hnd.user);
}

inline void streamClose(StreamHandle hnd)
{
	if (hnd.user && hnd.cb && hnd.cb->close)
		hnd.cb->close(hnd.user);
}


// Stream fixed reads

inline size_t streamReadU32le(StreamHandle hnd, uint32_t* restrict out, size_t count)
{
	assert(hnd.cb && hnd.cb->read);
	size_t r = hnd.cb->read(hnd.user, out, sizeof(uint32_t), count);
#if BYTE_ORDER == BIG_ENDIAN
	for (size_t i = 0; i < count; ++i)
		out[i] = swap32(out[i]);
#endif
	return r;
}

inline size_t streamReadU32be(StreamHandle hnd, uint32_t* restrict out, size_t count)
{
	assert(hnd.cb && hnd.cb->read);
	size_t r = hnd.cb->read(hnd.user, out, sizeof(uint32_t), count);
#if BYTE_ORDER == LITTLE_ENDIAN
	for (size_t i = 0; i < count; ++i)
		out[i] = swap32(out[i]);
#endif
	return r;
}

inline size_t streamReadI32le(StreamHandle hnd, int32_t* restrict out, size_t count)
{
	assert(hnd.cb && hnd.cb->read);
	size_t r = hnd.cb->read(hnd.user, out, sizeof(int32_t), count);
#if BYTE_ORDER == BIG_ENDIAN
	for (size_t i = 0; i < count; ++i)
		out[i] = (int32_t)swap32((uint32_t)out[i]);
#endif
	return r;
}

inline size_t streamReadU16le(StreamHandle hnd, uint16_t* restrict out, size_t count)
{
	assert(hnd.cb && hnd.cb->read);
	size_t r = hnd.cb->read(hnd.user, out, sizeof(uint16_t), count);
#if BYTE_ORDER == BIG_ENDIAN
	for (size_t i = 0; i < count; ++i)
		out[i] = swap16(out[i]);
#endif
	return r;
}

inline size_t streamReadU16be(StreamHandle hnd, uint16_t* restrict out, size_t count)
{
	assert(hnd.cb && hnd.cb->read);
	size_t r = hnd.cb->read(hnd.user, out, sizeof(uint16_t), count);
#if BYTE_ORDER == LITTLE_ENDIAN
	for (size_t i = 0; i < count; ++i)
		out[i] = swap16(out[i]);
#endif
	return r;
}

inline size_t streamReadI16be(StreamHandle hnd, int16_t* restrict out, size_t count)
{
	assert(hnd.cb && hnd.cb->read);
	size_t r = hnd.cb->read(hnd.user, out, sizeof(int16_t), count);
#if BYTE_ORDER == LITTLE_ENDIAN
	for (size_t i = 0; i < count; ++i)
		out[i] = (int16_t)swap16((uint16_t)out[i]);
#endif
	return r;
}


// Stream fixed writes

inline size_t streamWriteU32le(StreamHandle hnd, uint32_t v)
{
	assert(hnd.cb && hnd.cb->write);
	v = SWAP_LE32(v);
	return hnd.cb->write(hnd.user, (const void*)&v, sizeof(uint32_t), 1);
}

inline size_t streamWriteU32be(StreamHandle hnd, uint32_t v)
{
	assert(hnd.cb && hnd.cb->write);
	v = SWAP_BE32(v);
	return hnd.cb->write(hnd.user, (const void*)&v, sizeof(uint32_t), 1);
}

inline size_t streamWriteI32be(StreamHandle hnd, int32_t v)
{
	assert(hnd.cb && hnd.cb->write);
	v = (int32_t)SWAP_BE32((uint32_t)v);
	return hnd.cb->write(hnd.user, (const void*)&v, sizeof(int32_t), 1);
}

inline size_t streamWriteU16le(StreamHandle hnd, uint16_t v)
{
	assert(hnd.cb && hnd.cb->write);
	v = SWAP_LE16(v);
	return hnd.cb->write(hnd.user, (const void*)&v, sizeof(uint16_t), 1);
}

inline size_t streamWriteU16be(StreamHandle hnd, uint16_t v)
{
	assert(hnd.cb && hnd.cb->write);
	v = SWAP_BE16(v);
	return hnd.cb->write(hnd.user, (const void*)&v, sizeof(uint16_t), 1);
}

inline size_t streamWriteI16be(StreamHandle hnd, int16_t v)
{
	assert(hnd.cb && hnd.cb->write);
	v = (int16_t)SWAP_BE16((uint16_t)v);
	return hnd.cb->write(hnd.user, (const void*)&v, sizeof(int16_t), 1);
}


// Open file stream with platform native IO
int streamFileOpen(StreamHandle* restrict outHnd, const char* restrict path, const char* restrict mode);

#endif//STREAM_H
