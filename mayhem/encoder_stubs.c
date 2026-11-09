// Stand-ins for libhb/encx265.c and libhb/encsvtav1.c. Debian's libx265 (4.1) and libsvtav1enc (2.3)
// predate the 4.3 APIs those files use, so they are not compiled here. The encoders are never run by
// any harness; these only satisfy hb_global_init()'s work-object registration and the
// hb_x265_encopt_name() reference from hb_dict.c.
#include <stddef.h>
#include "handbrake/handbrake.h"
#include "handbrake/common.h"

static int stub_init(hb_work_object_t *w, hb_job_t *job) { (void)w; (void)job; return -1; }
static int stub_work(hb_work_object_t *w, hb_buffer_t **in, hb_buffer_t **out)
{
    (void)w; (void)in; (void)out;
    return HB_WORK_ERROR;
}
static void stub_close(hb_work_object_t *w) { (void)w; }

hb_work_object_t hb_encx265 = { WORK_ENCX265, "H.265/HEVC encoder (stub)", stub_init, stub_work, stub_close };
hb_work_object_t hb_encsvtav1 = { WORK_ENCSVTAV1, "AV1 encoder (stub)", stub_init, stub_work, stub_close };

const char *hb_x265_encopt_name(const char *name) { return name; }
