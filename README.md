## `zemp_us_pack`

This provides a reference implementation of the Zemp US Pack
format used for bundling multiple frames of image data together
into a compressed format. The format description can be found in
`zemp_us_pack.c`.

The files are mostly self contained only requiring the final
binary be linked with [libzstd][]. On their own they don't even
require libc.

The suggested way to include in your application is to statically
link with your main application:

```c
#define ZUS_PACK_STATIC
#include "zemp_us_pack.c"
```

However `zemp_us_pack.h` is provided if you prefer to build as a
dynamic library and link with your application.

The following sections give example usage.

### Packing

To pack a series of image frames into a pack file start by
allocating a `ZUSPStream` structure and filling it in:

```c
ZUSPStream zus = {
	.flush    = flush_function,
	.ctx      = ctx,
	.buffer   = data_buffer,
	.capacity = data_buffer_size,
};
```

Then pass it along with the a `ZUSMetadata`, a
`BeamformerParameters`, and one or more frames of image data to
`zus_pack()`:

```c
ZUSMetadata metadata = {
	.unix_timestamp    = creation_time_in_secs,
	.array_id          = array_id,
	.phantom_id        = phantom_id,
	.imaging_method_id = imaging_method_id,
};
i32 status = zus_pack(&zus, &metadata, beamformer_params_ptr, image_data_ptr,
                      image_data_size_in_bytes, frame_count);
```

Note that the function is re-entrant and you can keep calling it
with new image frames if you don't want to preload them all at
once.

The minimum amount of space required for the `buffer` member can
be queried as follows:

```c
size min_buffer_size = zus_min_buffer_size(single_image_frame_size_in_bytes);
zus.buffer           = malloc(min_buffer_size);
```

Any size larger can be used to improve performance. It is highly
recommended that a larger size be a multiple of the system page
size.

The `flush` and `ctx` members are used by the library to dump the
buffer when it is full. One possible example using libc stdio is
as follows:

```c
b32 my_flush_function(u8 *bytes, size number_of_bytes, void *ctx)
{
	FILE *f      = (FILE *)ctx;
	size written = fwrite(bytes, 1, number_of_bytes, f);
	return written == number_of_bytes;
}
...

FILE *output = fopen("packed_data.zus", "w");
zus.flush    = my_flush_fuunction;
zus.ctx      = output;
```

If your function returns `0` `zus_pack()` will return with
`ZUS_BUFFER_FLUSH_FAILURE` and you will be required to fix the
issue before calling `zus_pack()` again. Do not expect
`zus_pack()` to give correct results if, after fixing the problem,
you call it with a different data pointer.

Finally once you are done sending image frames finalize the stream:

```c
zus_finish_pack(&zus);
```

After which you may discard your output buffer and finalize your
own output location.

### Unpacking

To unpack a pack file start by reading the header into memory and
passing it to `zus_unpack_header()`:

```c
ZUSMetadata metadata;
BeamformerParameters bp;
u8 *header = my_os_read_file("packed.zus", ZUS_HEADER_SIZE);
i32 status = zus_unpack_header(header, &metadata, &bp);
```

If `zus_unpack_header()` determines that the memory does not look
like a **Zemp US Pack** it will return `ZUS_INVALID_HEADER` otherwise
it will unpack the header into the provided `ZUSMetadata` and
`BeamformerParameters` structs.

To unpack image data start by mapping the **Zemp US Pack** into
memory at the start of the data section and create a `ZUSUStream`
structure:

```c
ZUSUStream zus;
u8 *packed_data         = os_map_file_at_file_offset("packed.zus", ZUS_HEADER_SIZE);
size output_buffer_size = zus_create_unpack_stream(&zus, &metadata, packed_data);
```

The `zus_create_unpack_stream()` function will return the size
needed to unpack a single zstd frame (TODO: N image frames). You
can then allocate an output buffer and pass it in to
`zus_unpack_frame()`:

```c
i16 *raw_data_buffer = malloc(output_buffer_size);
size frame_offset    = zus_unpack_frame(&zus, raw_data_buffer, image_frame_number);
```

The function will return an offset into the `raw_data_buffer`
which gives the location of the requested image frame number. When
you want a different image frame just call the function again with
the same or a new data buffer.

TODO: if we store only a single image frame each zstd frame we
don't need to return an offset and we can (more easily) support
decompressing ranges of image frames at a time.

[libzstd]: https://github.com/facebook/zstd
