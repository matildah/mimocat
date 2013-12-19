#include <stdlib.h>
#include <stddef.h>
#include <assert.h>
#include <stdint.h>
int main(int argc, char ** argv)
{
    uint8_t * t = malloc(10);
    uint8_t * q = t +  2;
    ptrdiff_t z  =  (q -t);
    free(t);
    assert(z == 2);
    return 0;
}

