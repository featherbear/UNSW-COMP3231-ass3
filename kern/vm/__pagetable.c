#include <__pagetable.h>
#include <types.h>

int pagetable_translate(int32_t * address) {
    int32_t _address = (int32_t) address;

    int offset = _address & 0xFFF;
    int index = (_address >>= 12) & 0x3FF;
    int table_index = (_address >>= 10) & 0x1FF;
    int isKernelAddress = (_address >>= 9) & 1;

    return 0;
}