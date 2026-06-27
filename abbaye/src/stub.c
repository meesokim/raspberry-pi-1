#include <stddef.h>
#include <errno.h>
#include <stdlib.h>

void bz_internal_error(int n) {
}

int _getentropy(void *buffer, size_t length) {
    errno = ENOSYS;
    return -1;
}

#ifdef __cplusplus
extern "C" {
#endif
void __cxa_pure_virtual() {
    while (1);
}

void *miniz_def_alloc_func(void *opaque, size_t items, size_t size) {
    (void)opaque;
    return malloc(items * size);
}

void miniz_def_free_func(void *opaque, void *address) {
    (void)opaque;
    free(address);
}

void *miniz_def_realloc_func(void *opaque, void *address, size_t items, size_t size) {
    (void)opaque;
    return realloc(address, items * size);
}
#ifdef __cplusplus
}
#endif