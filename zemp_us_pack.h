/* NOTE: To use this file include it in a C-file as follows:
 *
 * #define ZUS_IMPLEMENTATION
 * #include "zemp_us_pack.h"
 *
 * If you will only call it from with the same translation unit you can:
 *
 * #define ZUS_STATIC
 *
 */

/* NOTE: You can redifine these types to whatever you like but they must be the correct size */

/* TODO: this won't work */
#ifndef zus_u8
#include <stdint.h>
#include <stddef.h>

typedef float     zus_f32;
typedef uint8_t   zus_u8;
typedef int16_t   zus_i16;
typedef uint32_t  zus_b32;
typedef uint32_t  zus_u32;
typedef int32_t   zus_i32;
typedef ptrdiff_t zus_size;
#endif

/* NOTE: Reference implementation of the zemp_us_pack (.zus) format
 *
 * Format Description
 *
 * IMPORTANT: This format is encoded as little endian. It is ordered such that on a little
 * endian machine it can be mem-mapped/read into a buffer and cast at the appropriate offset
 * into a C struct. The implementation provided here makes no such as assumptions and should
 * run on any host regardless of the endianess.
 *
 * IMPORTANT: User Data and Compressed Data Frames lie on 256 Byte Boundaries. This
 * allows the use AVX-512 aligned load instructions which require 64 Byte aligned
 * addresses and should allow for future devices which may be even wider.
 *
 * 64-Bytes of Metadata:
 *
 * Constant between versions:
 * Bytes 00-03 (u32): Pack File Magic ('P' << 24 | 'S' << 16 | 'U' << 8 | 'Z' << 0)
 * Bytes 04-07 (u32): Pack File Version
 *
 * Subject to change between versions:
 * Bytes 08-15 (u64): Largest Decompressed Frame Size (use this to preallocate an output buffer)
 * Bytes 16-19 (u32): Packed Frame Count (we store 1 data frame per zstd frame)
 *
 * Bytes 20-23 (u32): Array ID
 * Bytes 24-27 (u32): Subject ID
 * Bytes 28-31 (u32): Imaging Method ID
 * Bytes 32-39 (u64): Unix Time Stamp
 *
 * Bytes 39-43 (u32): User Data Tag
 * Bytes 44-47 (u32): User Data Size
 *
 * Bytes 47-255: Unused/padding
 *
 * Bytes 256 - (256 + User Data Size): User Data
 *
 * N-Bytes of Stacked zstd Data Frames
 *
 * First Frame starts at 256 + User Data Size + Alignment to next 256 byte boundary.
 * The end of the first frame is stored in the zstd header portion of this region. It can be
 * extracted using ZSTD_findFrameCompressedSize(). Then the next frame starts at the next 256
 * byte boundary after this size.
 *
 */

#include <zstd.h>

#define ZUS_ALIGNMENT         256UL
#define ZUS_COMPRESSION_LEVEL ZSTD_CLEVEL_DEFAULT

#define ZUS_ALIGN(p) ((p) + (ZUS_ALIGNMENT - (uintptr_t)(p) & ZUS_ALIGNMENT))

typedef struct {
	ZSTD_CCtx      *zstd_ctx;
	ZSTD_outBuffer  zstd_ob;
	ZSTD_inBuffer   zstd_ib;

	b32   (*flush)(u8 *, size, size, void *ctx);
	void  *ctx;
	u8    *buffer;
	size   capacity;
	size   widx;
	size   output_offset;
} ZUSStream;

#ifdef ZUS_STATIC
#define ZUS_EXPORT static
#else
#ifdef _WIN32
#define ZUS_EXPORT __declspec(dllexport)
#else
#define ZUS_EXPORT
#end
#end

/********************/
/* NOTE: Prototypes */
/********************/

/* NOTE: Data Packing */

ZUS_EXPORT i32 zus_push_frame(ZUSStream *zs, ZUSHeader *zh, u8 *raw, size raw_size);
ZUS_EXPORT i32 zus_pack_finish(ZUSStream *zs, ZUSHeader *zh);

/* NOTE: Data Unpacking */

/* Extracts data frame `frame_idx` (counting from 0) into out_buffer
 * returns:
 * ZUS_ERR_NONE: Success
 * ZUS_ERR_INVALID_FRAME: `frame_idx` >= zh->frame_count
 * ZUS_ERR_INVALID_DATA: `compressed_data` doesn't seem to hold zstd frames
 * ZUS_ERR_BUFFER_TOO_SMALL: output buffer doesn't have enough room to store decompressed data
 */
ZUS_EXPORT i32 zus_get_frame(ZUSHeader *zh, u8 *compressed_data, u32 frame_idx,
                             u8 *out_buffer, size out_buffer_size);
/* Same as zus_get_frame() except reads the frame at the base of compressed data.
 * see also:
 * zus_get_frame()
 * zus_extract_frame_pointers()
 */
ZUS_EXPORT i32 zus_get_frame_at(ZUSHeader *zh, u8 *compressed_data,
                                u8 *out_buffer, size out_buffer_size);

/* Extracts pointers to the base of each frame and stores them in out_buffer
 * returns:
 * ZUS_ERR_NONE: Success
 * ZUS_ERR_INVALID_DATA: `compressed_data` doesn't seem to hold zstd frames
 * ZUS_ERR_BUFFER_TOO_SMALL: output buffer doesn't have enough room to store zh->frame_count pointers
 */
ZUS_EXPORT i32 zus_extract_frame_pointers(ZUSHeader *zh, u8 *compressed_data,
                                          u8 *out_buffer[], size out_buffer_size);

/* NOTE: Implementations */
#ifdef ZUS_IMPLEMENTATION
#endif
