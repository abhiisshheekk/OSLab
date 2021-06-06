#include "ealloc.h"

#define ll long long

int USEDPAGES = 0;

ll validMemory[4][AVAILABLE];
char *allocation[4];
ll startAddr;

void init_alloc() {
    // initialize validMemory array with values as -1
    memset(validMemory, (ll)_INIT, sizeof(validMemory));

    // allocate 4KB memory to the first page with read write permission to the pages and private (not shared), anonymous (don't use a file). file descriptor: 0, offset: 0
    allocation[0] = (char *)mmap(NULL, PAGESIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);

    // increment the pageused
    USEDPAGES++;
}

void cleanup() {
    // initialize validMemory array with values as -1
    memset(validMemory, (ll)_INIT, sizeof(validMemory));
}

char *alloc(int _size) {
    // if buffer size is not multiple of 8, return NULL
    if (_size % MINALLOC != 0)
        return NULL;

    int slots = _size / MINALLOC;

    int slot = 0;
    int pageNumber;
    ll start = -1;
    bool isPageAvailable = false;

    // get whether the buffer size is available in the current page, if available set start as the first index
    for(pageNumber = 0; pageNumber < USEDPAGES; pageNumber++) {
        bool isAvailable = false;
        for (int i = 0; i < AVAILABLE; i++) {
            if (validMemory[pageNumber][i] == _INIT) {
                if (!isAvailable)
                    start = i;
                isAvailable = true;
                slot++;
                if (slot == slots) {
                    isPageAvailable = true;
                    break;
                }
            }
            else {
                isAvailable = false;
                slot = 0;
            }
        }

        if (isPageAvailable)
            break;
        
        start = -1;
        slot = 0;
    }

    // if the required memory is not available in the current page and it is the last page, return NULL
    if (!isPageAvailable && USEDPAGES == MAXPAGE)
        return NULL;
    
    // if it is not the last page, allocate memory to the next page and increment the used pages, and set start index as 0
    else if (!isPageAvailable && USEDPAGES < MAXPAGE) {
        allocation[USEDPAGES] = (char *)mmap(NULL, PAGESIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
        pageNumber = USEDPAGES++;
        start = 0;
    }
    
    // return address of the starting memory location 
    ll retAddress = (ll)(allocation[pageNumber] + start * MINALLOC);

    // store return address in the validMemory array to keep track of the available memory
    for (int i = start; i < start + slots; i++)
        validMemory[pageNumber][i] = retAddress;

    return (char *)retAddress;
}

void dealloc(char *addr) {
    // deallocate the memory by setting the slots as -1
    for (int i = 0; i < USEDPAGES*AVAILABLE; i++) {
        if (validMemory[i/AVAILABLE][i%AVAILABLE] == (ll)addr)
           validMemory[i/AVAILABLE][i%AVAILABLE]= _INIT;
    }
    return;
}