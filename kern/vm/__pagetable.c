#include <__pagetable.h>
#include <types.h>
#include <vm.h>

int pagetable_translate(int32_t * address) {
    int32_t _address = (int32_t) address;

    // VIRTUAL_MEMORY_ADDRESS
    //   |       |       \ 
    //   |       \        12 bits - offset
    //   \       10 bits - level 2 offset
    //    10 bits - level 1 offset



    // FIXME: Check for Kernel address?
    
    int page_number = (_address & PAGE_FRAME) >> 12;
    int second_index = page_number & 0x3FF;
    int first_index = (page_number >> 10) & 0x3FF;
    int offset = _address & ~PAGE_FRAME;


    // int offset = _address & 0xFFF; // Last 12 bits
    // int index = (_address >>= 12) & 0x3FF; // 
    // int table_index = (_address >>= 10) & 0x3FF;


    // TODO: Do lookup

    // return (lookup << 20) | offset;
    return 0;
}

int pagetable_init() {
    
}