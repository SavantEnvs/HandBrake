// Fuzz HandBrake's preset JSON import/clean/apply path (libhb/preset.c, hb_dict.c, hb_value.c).
#include "common.h"

int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    (void)argc; (void)argv;
    mayhem_init();
    return 0;
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (size == 0 || size > (1u << 20))
        return -1;

    char *json = mayhem_cstr(data, size);
    hb_value_t *in = hb_value_json(json);
    free(json);
    if (in == NULL)
        return -1;

    hb_value_t *out = NULL;
    if (hb_presets_import(in, &out) == 0 && out != NULL)
    {
        char *s = hb_value_get_json(out);
        free(s);

        hb_value_t *job = hb_dict_init();
        hb_preset_apply_mux(out, job);
        hb_preset_apply_video(out, job);
        hb_preset_apply_filters(out, job);
        hb_value_free(&job);
    }

    hb_presets_clean(in);
    char *s = hb_value_get_json(in);
    free(s);

    hb_value_free(&out);
    hb_value_free(&in);
    return 0;
}
