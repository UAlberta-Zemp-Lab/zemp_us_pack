#define ZUS_PACK_STATIC
#define ZUS_IMPLEMENTATION
#include "zemp_us_pack.h"

/* NOCOMMIT */
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#define ARRAY_COUNT(a) (sizeof(a) / sizeof(*a))
#define MEGABYTE (1024UL * 1024UL)

/* NOTE: this writes zeros into the buffer up until the next alignment.
 * Be careful about not writing zeros because if you reuse the buffer
 * later there may be unknown data left behind from previous runs */
static void
advance_stream_for_frame(ZUSStream *zus)
{
}

static i32
test_zus_pack_frame(ZUSStream *zus, i16 *frame, size frame_size_in_bytes)
{
	i32 status = 0;
	size written, total = 0;
	for (;;) {
		written = ZSTD_compressCCtx(zus->zstd_ctx, zus->buffer + zus->widx,
		                            zus->capacity - zus->widx, frame,
		                            frame_size_in_bytes - total, ZUS_COMPRESSION_LEVEL);
		total += written;
		if (total == zus->capacity)
			break;
		/* TODO: flush the stream */

		printf("zstd buffer pos = 0x%0x\n", (uintptr_t)(zus.buffer + zus.widx));
		advance_stream_for_frame(&zus);
		printf("rounded         = 0x%0x\n", (uintptr_t)(zus.buffer + zus.widx));
	}
	return status;
}

#if 0
static b32
test_zstd_unpack_frame(ZUSStream *zus)
{
	ZSTD_compressStream2(zus->zstd_ctx, &zus->zstd_ob, &zus->zstd_ib, ZSTD_e_continue);
	return 0;
}
#endif
i32
main(void)
{
	ZUSStream zus;
	zus.capacity = 1024 * MEGABYTE;
	zus.buffer   = malloc(zus.capacity);

	zus.zstd_ctx     = ZSTD_createCCtx();
	zus.zstd_ob.dst  = zus.buffer;
	zus.zstd_ob.size = zus.capacity;
	zus.zstd_ob.pos  = 0;

	ZSTD_inBuffer zstd_empty = {0};
	char *frames[] = {
		"/tmp/downloads/240823_ATS539_Resolution_HERCULES-TxRow_Intensity_00_rf_raw.bin",
		"/tmp/downloads/240823_ATS539_Resolution_HERCULES-TxRow_Intensity_01_rf_raw.bin",
		"/tmp/downloads/240823_ATS539_Resolution_HERCULES-TxRow_Intensity_02_rf_raw.bin",
		"/tmp/downloads/240823_ATS539_Resolution_HERCULES-TxRow_Intensity_03_rf_raw.bin",
	};
	for (u32 i = 0; i < ARRAY_COUNT(frames); i++) {
		struct stat sb;
		if (stat(frames[i], &sb) < 0)
			return 1;

		i32 fd = open(frames[i], O_RDONLY);
		if (fd < 0)
			return 1;

		i16 *frame = mmap(0, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
		test_zus_pack_frame(&zus, frame, sb.st_size);

		munmap(frame, sb.st_size);
		close(fd);
	}
	ZSTD_compressStream2(zus.zstd_ctx, &zus.zstd_ob, &zstd_empty, ZSTD_e_end);

	i32 fd = open("/tmp/downloads/out.zus", O_WRONLY|O_CREAT|O_TRUNC, 0660);
	if (fd < 0)
		return 1;

	size written = write(fd, zus.buffer, zus.zstd_ob.pos);
	printf("wrote %zd/%zd\n", written, zus.zstd_ob.pos);
	close(fd);

	return 0;
}
