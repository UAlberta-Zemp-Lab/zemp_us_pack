## `zemp_us_pack`

This provides a reference implementation of the Zemp US Pack
format used for bundling multiple frames of image data together
into a compressed format. The format description can be found in
`zemp_us_pack.h`.

The files are mostly self contained only requiring the final
binary be linked with [libzstd][]. On their own they don't even
require libc.

The suggested way to include in your application is to statically
link with your main application:

```c
#define ZUS_IMPLEMENTATION
#define ZUS_PACK_STATIC
#include "zemp_us_pack.h"
```

However `#define ZUS_STATIC` can be dropped if you prefer to build as a
dynamic library and link with your application.

The following sections give example usage.

### Packing

To pack a series of image frames into a pack file start by
allocating a `ZUSStream` structure and filling it in:

```c
ZUSStream zus = {
	.flush    = flush_function,
	.ctx      = ctx,
	.buffer   = data_buffer,
	.capacity = data_buffer_size,
};
```

Next fill out a `ZUSHeader`:

```c
ZUSHeader header = {
	.unix_timestamp    = data_creation_time_in_secs,
	.array_id          = array_id,
	.phantom_id        = phantom_id,
	.imaging_method_id = imaging_method_id,
	.user_data_tag     = user_data_tag,
	.user_data_size    = user_data_size,
	.user_data         = user_data_ptr,
};
```

The user data size must be specified ahead of time and cannot be
modified after the fact.

Then pass the `ZUSStream` and `ZUSHeader` with a frame of image
data to `zus_push_frame()`:

```c
i32 status = zus_push_frame(&zus, &header, raw_data_ptr, raw_data_size_in_bytes);
```

Note that the function is re-entrant and you can keep calling it
with new image frames if you don't want to preload them all at
once. You must specify the size of the user data upfront and

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
b32 my_flush_function(u8 *bytes, size number_of_bytes, size offset, void *ctx)
{
	FILE *f      = (FILE *)ctx;
	b32 result   = fseeko(f, offset, SEEK_SET) == 0;
	size written = fwrite(bytes, 1, number_of_bytes, f);
	return result && (written == number_of_bytes);
}
...

FILE *output = fopen("packed_data.zus", "w");
zus.flush    = my_flush_function;
zus.ctx      = output;
```

If your function returns `0` `zus_push_frame()` will return with
`ZUS_ERR_BUFFER_FLUSH_FAILURE` and you will be required to fix the
issue before calling `zus_push_frame()` again. Do not expect
`zus_push_frame()` to give correct results if, after fixing the
problem, you call it with a different data pointer.

Finally once you are done sending image frames finalize the stream:

```c
zus_pack_finish(&zus, &header);
```

After which you may discard your output buffer and finalize your
own output location.

### Unpacking

TODO: This needs to be updated. For instance this format no longer
cares about the userdata format.

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
