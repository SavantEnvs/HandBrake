// Fuzz HandBrake's H.264/H.265 NAL unit bitstream converters (libhb/nal_units.c).
#include "common.h"
#include "handbrake/nal_units.h"

int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    (void)argc; (void)argv;
    mayhem_init();
    return 0;
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (size < 2 || size > (1u << 20))
        return -1;

    uint8_t nal_length_size = 1 + (data[0] & 3);
    const uint8_t *buf = data + 1;
    size_t len = size - 1;

    hb_buffer_t *b = hb_nal_bitstream_annexb_to_mp4(buf, len);
    hb_buffer_close(&b);

    b = hb_nal_bitstream_mp4_to_annexb(buf, len, nal_length_size);
    hb_buffer_close(&b);

    return 0;
}
