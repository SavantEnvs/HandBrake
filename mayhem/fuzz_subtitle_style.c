// Fuzz HandBrake's SSA/ASS subtitle style parsing and tx3g conversion (libhb/ssautil.c).
#include "common.h"
#include "handbrake/ssautil.h"

int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    (void)argc; (void)argv;
    mayhem_init();
    return 0;
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (size < 2 || size > (1u << 18))
        return -1;

    /* First byte: tx3g video height. Then the SSA header; an optional NUL separates it from a
       dialogue event line (defaults to the same bytes). */
    int height = (data[0] + 1) * 8;
    const uint8_t *ssa = data + 1;
    int ssa_size = (int)(size - 1);
    const uint8_t *sep = memchr(ssa, 0, (size_t)ssa_size);
    const uint8_t *ev = ssa;
    size_t ev_size = (size_t)ssa_size;
    if (sep != NULL)
    {
        ssa_size = (int)(sep - ssa);
        ev = sep + 1;
        ev_size = (size_t)(size - 1 - (size_t)ssa_size - 1);
    }

    hb_subtitle_style_context_t *sctx = hb_subtitle_style_init(ssa, ssa_size);
    hb_subtitle_style_close(&sctx);

    hb_tx3g_style_context_t *ctx = hb_tx3g_style_init(height, ssa, ssa_size);
    if (ctx == NULL)
        return -1;

    char *line = mayhem_cstr(ev, ev_size);
    uint8_t *out = NULL, *style = NULL;
    uint16_t style_size = 0;
    hb_muxmp4_process_subtitle_style(ctx, (uint8_t *)line, &out, &style, &style_size);
    free(out);
    free(style);
    free(line);

    hb_tx3g_style_close(&ctx);
    return 0;
}
