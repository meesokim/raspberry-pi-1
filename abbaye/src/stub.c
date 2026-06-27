#include <stddef.h>
#include <errno.h>

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
#ifdef __cplusplus
}
#endif