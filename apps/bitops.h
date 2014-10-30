#ifndef BITOPS_H
#define BITOPS_H

#include "string.h"      /* due to memset */



/* determines if bit number "bit_nb" is set in array "arr" */
#define IS_BIT_SET(arr, bit_nb) (((unsigned char*) arr)[(bit_nb) >> 3] & \
    (((unsigned char) 1) << ((bit_nb) & 7)))

/* determines if bit number "bit_nb" is set in array "arr" */
#define IS_BIT_CLEARED(arr, bit_nb) (! IS_BIT_SET(arr, bit_nb))

/* resets bit "bit_nb" in array "arr" */
#define RESET_BIT(arr, bit_nb) (((unsigned char*) arr)[(bit_nb) >> 3] &= ~(((unsigned char) 1) << ((bit_nb) & 7)))

/* sets bit "bit_nb" in array "arr" */
#define SET_BIT(arr, bit_nb)   (((unsigned char*) arr)[(bit_nb) >> 3] |=   ((unsigned char) 1) << ((bit_nb) & 7))



/* set the first nb_bits in array arr */
inline void SET_ALL_BITS(unsigned char* arr, unsigned long nb_bits)
{
    memset(arr, 255, nb_bits >> 3);
    if(nb_bits & 7) {
        arr[nb_bits >> 3] |= ((unsigned char) 255) >> (8 - (nb_bits & 7));
    }
}

/* reset the first nb_bits in array arr */
inline void RESET_ALL_BITS(unsigned char* arr, unsigned long nb_bits)
{
    memset(arr, 0, nb_bits >> 3);
    if(nb_bits & 7) {
        arr[nb_bits >> 3] &= ((unsigned char) 255) << (nb_bits & 7);
    }
}

#endif
