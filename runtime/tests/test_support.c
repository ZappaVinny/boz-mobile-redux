#include "s3e_host_internal.h"

#include <stdarg.h>

uint32_t s3e_guest_call(const void *fn, int argc, ...) {
    uint32_t a[6] = {0};
    va_list ap;
    va_start(ap, argc);
    for (int i = 0; i < argc && i < 6; ++i) {
        a[i] = va_arg(ap, uint32_t);
    }
    va_end(ap);
    typedef uint32_t (*native6_fn)(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);
    return ((native6_fn)(uintptr_t)fn)(a[0], a[1], a[2], a[3], a[4], a[5]);
}

__attribute__((weak)) void present_rect(int32_t width, int32_t height, int32_t *rect) {
    rect[0] = 0;
    rect[1] = 0;
    rect[2] = width;
    rect[3] = height;
}

__attribute__((weak)) int32_t s3eDeviceRequestQuit(void) {
    return 0;
}
