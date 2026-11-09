// Known-answer probe over libhb, built with normal flags. mayhem/test.sh asserts on its output.
#include <stdio.h>
#include <string.h>

#include "handbrake/handbrake.h"
#include "handbrake/nal_units.h"

static void hex(const char *label, const uint8_t *p, size_t n)
{
    printf("%s=", label);
    for (size_t i = 0; i < n; i++)
        printf("%02x", p[i]);
    printf("\n");
}

int main(void)
{
    hb_global_init_no_hardware();

    /* Annex B -> MP4 (4-byte lengths) and back. */
    static const uint8_t annexb[] = { 0, 0, 0, 1, 0x65, 0xAA, 0xBB, 0xCC, 0, 0, 1, 0x41, 0x11 };
    hb_buffer_t *b = hb_nal_bitstream_annexb_to_mp4(annexb, sizeof(annexb));
    if (b == NULL)
        return 2;
    hex("annexb_to_mp4", b->data, (size_t)b->size);
    hb_buffer_t *r = hb_nal_bitstream_mp4_to_annexb(b->data, (size_t)b->size, 4);
    if (r == NULL)
        return 2;
    hex("mp4_to_annexb", r->data, (size_t)r->size);
    hb_buffer_close(&r);
    hb_buffer_close(&b);

    /* Filter settings string <-> dict. */
    hb_dict_t *d = hb_parse_filter_settings("mode=3:parity=1");
    char *json = d != NULL ? hb_value_get_json(d) : NULL;
    printf("filter_dict=%s\n", json != NULL ? json : "(null)");
    free(json);
    char *str = d != NULL ? hb_filter_settings_string(HB_FILTER_DECOMB, d) : NULL;
    printf("filter_string=%s\n", str != NULL ? str : "(null)");
    free(str);
    hb_value_free(&d);

    /* JSON round trip and preset import normalization. */
    hb_value_t *v = hb_value_json("{ \"a\" : [1, 2, {\"b\": \"c\"}], \"n\": 2.5 }");
    json = v != NULL ? hb_value_get_json(v) : NULL;
    printf("json_roundtrip=%s\n", json != NULL ? json : "(null)");
    free(json);
    hb_value_free(&v);

    char *out = NULL;
    char *builtin = hb_presets_builtin_get_json();
    int rc = hb_presets_import_json(builtin, &out);
    printf("builtin_import_rc=%d has_fast1080=%d\n", rc,
           out != NULL && strstr(out, "\"Fast 1080p30\"") != NULL);
    free(out);
    free(builtin);

    out = NULL;
    rc = hb_presets_import_json(
        "{\"PresetList\":[{\"PresetName\":\"kat\",\"Type\":1,\"VideoQualitySlider\":21,"
        "\"FileFormat\":\"av_mkv\",\"Folder\":false}],"
        "\"VersionMajor\":76,\"VersionMinor\":0,\"VersionMicro\":0}", &out);
    printf("preset_import_rc=%d\n", rc);
    hb_value_t *imp = out != NULL ? hb_value_json(out) : NULL;
    hb_value_t *p0 = hb_value_array_get(hb_dict_get(imp, "PresetList"), 0);
    printf("preset_name=%s quality=%.1f format=%s audio_fallback=%s\n",
           hb_value_get_string(hb_dict_get(p0, "PresetName")),
           hb_value_get_double(hb_dict_get(p0, "VideoQualitySlider")),
           hb_value_get_string(hb_dict_get(p0, "FileFormat")),
           hb_value_get_string(hb_dict_get(p0, "AudioEncoderFallback")));
    hb_value_free(&imp);
    free(out);

    /* Name tables. */
    int x = 0, y = 0;
    hb_reduce(&x, &y, 1920, 1080);
    printf("reduce=%d/%d\n", x, y);
    printf("encoder_x264=%d name=%s\n", hb_video_encoder_get_from_name("x264") == HB_VCODEC_X264,
           hb_video_encoder_get_name(HB_VCODEC_X264));
    printf("samplerate_48000=%s\n", hb_audio_samplerate_get_name(48000));
    printf("container_mkv=%s\n", hb_container_get_name(HB_MUX_AV_MKV));

    hb_global_close();
    return 0;
}
