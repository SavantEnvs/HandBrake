// Fuzz HandBrake's source-file scanner: container probing + title scan + packet reads
// (libhb/stream.c: MPEG-TS/PS probing, FFmpeg demuxers, hb_stream_title_scan, hb_stream_read).
#include "common.h"
#include "handbrake/internal.h"

#include <unistd.h>

static hb_handle_t *handle;

int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    (void)argc; (void)argv;
    mayhem_init();
    handle = hb_init(0);
    return 0;
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (size < 16 || size > (1u << 22))
        return -1;

    const char *tmp = getenv("TMPDIR");
    char path[4096];
    snprintf(path, sizeof(path), "%s/hb-stream-XXXXXX", tmp != NULL && *tmp ? tmp : "/tmp");
    int fd = mkstemp(path);
    if (fd < 0)
        return -1;
    size_t off = 0;
    while (off < size)
    {
        ssize_t n = write(fd, data + off, size - off);
        if (n <= 0)
        {
            close(fd);
            unlink(path);
            return -1;
        }
        off += (size_t)n;
    }
    close(fd);

    hb_title_t *title = hb_title_init(path, 0);
    hb_stream_t *stream = hb_stream_open(handle, path, title, 1);
    int opened = stream != NULL;
    if (opened)
    {
        hb_title_t *scanned = hb_stream_title_scan(stream, title);
        if (scanned != NULL)
        {
            for (int i = 0; i < 16; i++)
            {
                hb_buffer_t *buf = hb_stream_read(stream);
                if (buf == NULL)
                    break;
                hb_buffer_close(&buf);
            }
        }
        hb_stream_close(&stream);
    }
    hb_title_close(&title);
    unlink(path);
    return opened ? 0 : -1;
}
