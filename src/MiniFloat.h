#ifndef _MINIFLOAT_
#define _MINIFLOAT_

extern "C" {

#include <stdio.h>
#include <string.h>
#include <stdint.h>

typedef uint8_t minifloat;

/*
    minifloat variable: sign bit, then three bits of exponent, and four bits of
    mantissa.  Like the 1:4:3 minifloat, but instead with the exponent as the
    least significant bits.

    |Sx|XXX|Sv|VVV|
*/

#define M_ALL 0xFF
#define M_SGN 0x08
#define M_VAL 0x0F
#define M_PRT 0x07
#define M_ESN 0x80
#define M_EXP 0xF0
#define M_EPT 0x70


/* converts a minifloat value to the corresponding big float value. */
float mini_to_float(minifloat v)
{
  /*uses a representational conversion mechanism*/

  /*define a "product accumulator" value*/
  float acc = 1;

  /*separate the value part away from the exponent part, and alter the
   accumulator accordingly*/
  acc *= ((v & M_SGN) ?
          (-1 * (M_ALL ^ (M_EXP | (int8_t) v))) :
          (M_VAL & v));

  /*now separate the exponential part away from the value part and alter
    the accumulator accordingly. */
  acc *= ((v & M_ESN) ? 1 / (float) (1 << ((M_ALL ^ v) >> 4)) : 1 << (v >> 4));

  return acc;
}

/*creates the additive inverse of v*/
minifloat addinv(minifloat v)
{
  return v ^ M_VAL;
}

/*performs addition on v */
minifloat add(minifloat u, minifloat v)
{
  /*first check to see if u is greater than v*/
  return 0;
}


int test()
{
  minifloat one = 0x01;
  minifloat two = 0x02;
  minifloat mone = 0x0E;
  minifloat half = 0xE1;
  minifloat quart = 0xD1;
  minifloat four = 0x04;
  minifloat alsofour = 0x12;
  minifloat stillfour = 0x21;

  printf("value: 0X%02X %4.2f\n", one, mini_to_float(one));
  printf("value: 0X%02X %4.2f\n", two, mini_to_float(two));
  printf("value: 0X%02X %4.2f\n", mone, mini_to_float(mone));
  printf("value: 0X%02X %4.2f\n", half, mini_to_float(half));
  printf("value: 0X%02X %4.2f\n", quart, mini_to_float(quart));
  printf("value: 0X%02X %4.2f\n", four, mini_to_float(four));
  printf("value: 0X%02X %4.2f\n", alsofour, mini_to_float(alsofour));
  printf("value: 0X%02X %4.2f\n", stillfour, mini_to_float(stillfour));
}
};
#endif /* _MINIFLOAT_ */
