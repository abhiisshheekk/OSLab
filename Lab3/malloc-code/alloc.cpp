#include "alloc.h"

#define ll long long

ll validMemory[AVAILABLE];
char *allocation;
ll startAddr;

int init_alloc() {
    allocation = (char *) mmap(NULL, PAGESIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);

    if(allocation == MAP_FAILED) return 1;
    startAddr = (ll) allocation;
    memset(validMemory, (ll)_INIT, sizeof(validMemory));

    return 0;
}

int cleanup() {
    ll err = munmap(allocation, PAGESIZE);
    memset(validMemory, (ll)_INIT, sizeof(validMemory));
    return err;
}

char *alloc(int _size) {
    if(_size % MINALLOC != 0)
        return NULL;

    int slots = _size/MINALLOC;
    int slot = 0;
    for(int i = 0; i < AVAILABLE; i++) {
        if(validMemory[i] == _INIT) {
            slot++;
        }
    }
    if(slot < slots)
        return NULL;
    for(int i = 0; i < AVAILABLE; i++) {
        if(validMemory[i] == _INIT) {
            ll retAddress = (ll) (allocation + i*MINALLOC);
            for(int j = i; j < i+slots; j++)
                validMemory[j] = retAddress;
            return (char *) retAddress;
        }
    }
    return NULL;
}

void dealloc(char * addr) {
    for(int i = 0; i < AVAILABLE; i++) {
        if(validMemory[i] == (ll) addr) {
            validMemory[i] = _INIT;
        }
    }
    return;
}
