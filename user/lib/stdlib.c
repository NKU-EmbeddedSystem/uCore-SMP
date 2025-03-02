#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <ucore.h>

static uint64 seed;

void srand(unsigned s) {
    seed = s - 1;
}

int rand(void) {
    seed = 6364136223846793005ULL * seed + 1;
    return seed >> 33;
}

void panic(char* m) {
    puts(m);
    exit(-100);
}