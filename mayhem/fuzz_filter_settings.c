// Fuzz HandBrake's filter settings-string parser and validators (libhb/common.c, param.c, hb_dict.c).
#include "common.h"

int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    (void)argc; (void)argv;
    mayhem_init();
    return 0;
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (size < 2 || size > 4096)
        return -1;

    /* First byte picks the filter; the rest is the settings string. */
    int filter_id = HB_FILTER_FIRST + data[0] % (HB_FILTER_LAST - HB_FILTER_FIRST);
    char *str = mayhem_cstr(data + 1, size - 1);

    hb_dict_t *dict = hb_parse_filter_settings(str);
    if (dict != NULL)
    {
        char *s = hb_filter_settings_string(filter_id, dict);
        free(s);
        hb_validate_filter_settings(filter_id, dict);
        hb_value_free(&dict);
    }

    hb_validate_filter_string(filter_id, str);

    hb_dict_t *gen = hb_generate_filter_settings(filter_id, NULL, NULL, str);
    hb_value_free(&gen);

    char *tune = strchr(str, ':');
    if (tune != NULL)
    {
        *tune++ = '\0';
        hb_validate_filter_preset(filter_id, str, tune, NULL);
        gen = hb_generate_filter_settings(filter_id, str, tune, NULL);
        hb_value_free(&gen);
    }

    free(str);
    return 0;
}
