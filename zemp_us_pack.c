/* NOTE: Reference implementation of the zemp_us_pack (.zus) format
 *
 * Format Description
 *
 * IMPORTANT: This format is encoded as little endian. It is ordered such that on a little
 * endian machine it can be mem-mapped/read into a buffer and cast at the appropriate offset
 * into a C struct. The implementation provided here makes no such as assumptions and should
 * run on any host regardless of the endianess.
 *
 * IMPORTANT: Beamformer Parameters and Compressed Data Frames lie on 256 Byte Boundaries this
 * allows the you to use AVX-512 aligned load instructions which require 64-Byte aligned
 * addresses and should allow for future devices which may be even wider.
 *
 * 64-Bytes of Metadata:
 *
 * Constant between versions:
 * Bytes 00-03: Pack File Magic (0x???? ????)
 * Bytes 04-07: Pack File Version
 *
 * Subject to change between versions:
 * Bytes 10-11: zstd frame count
 * Bytes 12-15: zstd frame size in bytes
 * Bytes 16-19: blocks per zstd frame (each block is one frame of image data)
 *
 * Bytes 20-23: number of extra padding bytes at the end of each image frame after decompression
 *              padding bytes are used to key everything aligned on 64-Byte boundaries.
 *
 * Bytes 28-31: Unix Time Stamp (64-bit time_t)
 * Bytes 32-35: Array ID
 * Bytes 36-39: Subject ID
 * Bytes 40-43: Imaging Method ID
 *
 * Bytes 44-47: Total Number of Packed Image Frames
 *              zstd_frame_count * blocks_per_zstd_from - total_packed_image_frames will give
 *              the number of empty image frames at the end of the last zstd frame.
 *
 * Bytes 48-63: Unused/padding
 *
 * 2048-Bytes of Paramater Header
 *
 * Bytes 64-2111: Beamformer Parameters Struct
 *
 * N-Bytes of Stacked zstd Data Frames
 *
 * Bytes 2112-(2112 + zstd_frame_size_in_bytes - 1): zstd frame 0
 * ...   - EOF: zstd frame N (N = zstd_frame_count)
 *
 * NOTE: While not required it is suggested that transmit channel data be discarded
 * prior to packing to save space. The implementation provided below will do this implicitly.
 *
 */

#ifndef f32
#include <stdint.h>
#include <stddef.h>

typedef float     f32;
typedef uint8_t   u8;
typedef int16_t   i16;
typedef uint32_t  b32;
typedef uint32_t  u32;
typedef ptrdiff_t size;

typedef struct { f32 x, y; }       v2;
typedef struct { u32 x, y; }       uv2;
typedef struct { u32 x, y, z, w; } uv4;
#endif

#include "beamformer_parameters.h"

#ifndef ZUS_PACK_STATIC
#include "zemp_us_pack.h"
#else
typedef struct {
	b32   (*flush)(u8 *, size, void *ctx);
	void  *ctx;
	u8    *buffer;
	size   capacity;
	size   widx;
	b32    wrote_header;
} ZUSStream;
#endif
