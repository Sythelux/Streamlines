/*

Traverse a rectangle of pixels in a random order.  From Mike Morton's
contribution to Graphics Gems (original volume).

Greg Turk, August 1995

--------------------------------------------------------------------

Copyright (c) 1996 The University of North Carolina.  All rights reserved.   
  
Permission to use, copy, modify and distribute this software and its   
documentation for any purpose is hereby granted without fee, provided   
that the above copyright notice and this permission notice appear in   
all copies of this software and that you do not sell the software.   
  
The software is provided "as is" and without warranty of any kind,   
express, implied or otherwise, including without limitation, any   
warranty of merchantability or fitness for a particular purpose.   

*/


#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "dissolve.h"

static int randmasks[] = {
        0x0,        //  0 (not used)
        0x0,        //  1 (not used)
        0x03,       //  2
        0x06,       //  3
        0x0c,       //  4
        0x14,       //  5
        0x30,       //  6
        0x60,       //  7
        0xb8,       //  8
        0x0110,     //  9
        0x0240,     // 10
        0x0500,     // 11
        0x0ca0,     // 12
        0x1b00,     // 13
        0x3500,     // 14
        0x6000,     // 15
        0xb400,     // 16
        0x00012000, // 17
        0x00020400, // 18
        0x00072000, // 19
};


/******************************************************************************
Get how wide the word has to be.
******************************************************************************/

static int bitwidth(int n)
{
  int width = 0;

  while (n != 0) {
    n >>= 1;
    width++;
  }

  return (width);
}


/******************************************************************************
Constructor for dissolve.
******************************************************************************/

Dissolve::Dissolve(int w, int h)
{
  width = w;
  height = h;
  initial_value = 1;

  int pixels = width * height;
  int lastnum = pixels - 1;
  regwidth = bitwidth(lastnum);
  if (regwidth < 2)
    regwidth = 2;
  mask = randmasks[regwidth];
  seq = 0;

#if 0
  printf ("w,h = %d %d\n", width, height);
  printf ("pixels = %d, regwidth = %d, mask = %d\n",
          pixels, regwidth, mask);
#endif
}


/******************************************************************************
Returns the next integer in the pseudo-random sequence.
******************************************************************************/

int Dissolve::next_value()
{
  /* must special-case zero because it doesn't appear in sequence */

  if (seq == 0) {
    seq = initial_value;
    return (0);
  }

  /* remember current value */

  int return_value = seq;

  /* compute next value in sequence */

  if (seq & 0x1)
    seq = (seq >> 1) ^ mask;
  else
    seq >>= 1;

  /* see if we've wrapped to beginning and if so, kluge the zero case */

  if (seq == initial_value)
    seq = 0;

  return (return_value);
}


/******************************************************************************
Returns a new position in the dissolve sequence.

Exit:
  x,y - new position
******************************************************************************/

void Dissolve::new_position(int &x, int &y)
{

#if 0
  x = (int) floor (width * drand48());
  y = (int) floor (height * drand48());
#endif

#if 1

  int row, column;

  do {
    int value = next_value();
    row = value / width;
    column = value % width;
  } while (row >= height);

  x = column;
  y = row;

#endif

}

