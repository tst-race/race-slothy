#include "util/hex.hpp"

// Debugging function for hexdumping memory
void print_hex(uint8_t *ptr, size_t len){
    printf("\t ");
    for (int i=0; i<len; i++)
    {
        printf("%02x ",ptr[i]);
    }
    printf("\n");
}