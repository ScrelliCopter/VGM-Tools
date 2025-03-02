#include "stream.h"


extern inline size_t streamRead(StreamHandle hnd, void *restrict out, size_t size, size_t count);
extern inline size_t streamWrite(StreamHandle hnd, const void *restrict in, size_t size, size_t count);
extern inline int streamGetC(StreamHandle hnd);
extern inline int streamPutC(StreamHandle hnd, int c);
extern inline bool streamSkip(StreamHandle hnd, long offset);
extern inline bool streamSeek(StreamHandle hnd, long offset, StreamWhence whence);
extern inline bool streamTell(StreamHandle hnd, size_t* restrict result);
extern inline bool streamEOF(StreamHandle hnd);
extern inline bool streamError(StreamHandle hnd);
extern inline void streamClose(StreamHandle hnd);

extern inline size_t streamReadU32le(StreamHandle hnd, uint32_t* restrict out, size_t count);
extern inline size_t streamReadU32be(StreamHandle hnd, uint32_t* restrict out, size_t count);
extern inline size_t streamReadI32le(StreamHandle hnd, int32_t* restrict out, size_t count);
extern inline size_t streamReadU16le(StreamHandle hnd, uint16_t* restrict out, size_t count);
extern inline size_t streamReadU16be(StreamHandle hnd, uint16_t* restrict out, size_t count);
extern inline size_t streamReadI16be(StreamHandle hnd, int16_t* restrict out, size_t count);

extern inline size_t streamWriteU32le(StreamHandle hnd, uint32_t v);
extern inline size_t streamWriteU32be(StreamHandle hnd, uint32_t v);
extern inline size_t streamWriteI32be(StreamHandle hnd, int32_t v);
extern inline size_t streamWriteU16le(StreamHandle hnd, uint16_t v);
extern inline size_t streamWriteU16be(StreamHandle hnd, uint16_t v);
extern inline size_t streamWriteI16be(StreamHandle hnd, int16_t v);
