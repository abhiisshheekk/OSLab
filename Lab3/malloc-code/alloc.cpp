#include "alloc.h"

#define ll long long

ll validMemory[AVAILABLE];
char *allocation;

int init_alloc() {
    // allocate 4KB memory with read write permission to the pages and private (not shared), anonymous (don't use a file). file descriptor: 0, offset: 0
    allocation = (char *) mmap(NULL, PAGESIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);

    // if allocation failed, return 1
    if(allocation == MAP_FAILED) return 1;

    // initialize validMemory array with values as -1
    memset(validMemory, (ll)_INIT, sizeof(validMemory));

    return 0;
}

int cleanup() {
    // deallocate the memory allocated 
    ll err = munmap(allocation, PAGESIZE);

    // reinitialize validMemory array with values as -1
    memset(validMemory, (ll)_INIT, sizeof(validMemory));

    return err;
}

char *alloc(int _size) {
    // if buffer size is not multiple of 8, return NULL
    if(_size % MINALLOC != 0)
        return NULL;

    int start = -1;
    bool isAvailable = false;
    int slots = _size/MINALLOC;
    int slot = 0;

    // get whether the buffer size is available, if available set start as the first index
    for(int i = 0; i < AVAILABLE; i++) {
        if(validMemory[i] == _INIT) {
            if(!isAvailable)
                start = i;
            isAvailable = true;
            slot++;
            if(slot == slots)
                break;
        }
        else {
            slot = 0;
            isAvailable = false;
        }
    }

    // if available memory is less, return NULL
    if(slot < slots)
        return NULL;

    // return address of the starting memory location 
    ll retAddress = (ll) (allocation + start*MINALLOC);

    // store return address in the validMemory array to keep track of the available memory
    for(int i = start; i < start + slot; i++) {
        validMemory[i] = retAddress;
    }

    return (char *) retAddress;
}

void dealloc(char * addr) {
    // deallocate the memory by setting the slots as -1
    for(int i = 0; i < AVAILABLE; i++) {
        if(validMemory[i] == (ll) addr) {
            validMemory[i] = _INIT;
        }
    }
    return;
}
