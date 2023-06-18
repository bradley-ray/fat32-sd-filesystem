#include "hal.h"

int _write(int file, char* ptr, int len) {
    if (file == 1)
        uart2_write_buff((uint8_t*)ptr, (uint32_t)len);
    return -1;
}

int _fstat(int fd, void *st) {
    (void)fd, (void)st;
    return -1;
}

void* _sbrk(int incr) {
    extern uint8_t _sheap;
    static uint8_t *heap_ptr;
    uint8_t *prev_heap_ptr;

    if (heap_ptr == 0) {
        heap_ptr = &_sheap;
    }

    prev_heap_ptr = heap_ptr;

    // limit heap size to 0x200
    // TODO: probably just better to make sure no collision with stack
    if ( (heap_ptr + incr) - &_sheap > 0x200 ) {
        return (void*)heap_ptr;
    }
    heap_ptr += incr;

    return (void*)prev_heap_ptr; 
}

int _close(int fd) {
    (void)fd;
    return -1;
}

int _isatty(int fd) {
    (void)fd;
    return 1;
}


int _read(int fd, char* ptr, int len) {
    (void)fd, (void)ptr, (void)len;
    return -1;
}

int _lseek(int fd, int ptr, int dir) {
    (void)fd, (void)ptr, (void)dir;
    return 0;
}