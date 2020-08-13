/*

Line clipping to a window.

Greg Turk, March 1995

---------------------------------------------------------------------

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
#include <fstream>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

/* the clipping window */
static float xmin = 0.0;
static float xmax = 1.0;
static float ymin = 0.0;
static float ymax = 1.0;


/******************************************************************************
Specify a clipping window.

Entry:
  x0,x1 - left and right boundaries for clipping window
  y0,y1 - bottom and top boundaries
******************************************************************************/

void set_clip_window(float x0, float x1, float y0, float y1)
{
  xmin = x0;
  xmax = x1;
  ymin = y0;
  ymax = y1;
}


/******************************************************************************
Given a point P outside the window and the rise and run of a line, return
the intersection of line with window that is nearest P.

Entry:
  dx,dy - run and rise of line
  x,y   - the given point P

Exit:
  ix,iy - intersection point
  return 1 if there was a valid intersection, 0 if not
******************************************************************************/

int clip_helper(float dx, float dy, float x, float y, float &ix, float &iy)
{
  /* if line not vertical, check against left and right edges of window */

  if (dx != 0) {

    /* check against left edge */
    iy = dy / dx * (xmin - x) + y;
    if (xmin > x && iy > ymin && iy < ymax) {
      ix = xmin;
      return (1);
    }

    /* check against right edge */
    iy = dy / dx * (xmax - x) + y;
    if (xmax < x && iy > ymin && iy < ymax) {
      ix = xmax;
      return (1);
    }
  }

  /* if line not horizontal, check against top and bottom edges of window */

  if (dy != 0) {

    /* check against bottom edge */
    ix = dx / dy * (ymin - y) + x;
    if (ymin > y && ix > xmin && ix < xmax) {
      iy = ymin;
      return (1);
    }

    /* check against top edge */
    ix = dx / dy * (ymax - y) + x;
    if (ymax < y && ix > xmin && ix < xmax) {
      iy = ymax;
      return (1);
    }
  }

  /* if we get here, we found no intersection */
  return (0);
}


/******************************************************************************
Clip a line segment to a pre-specified window.

Entry:
  x0,y0 - first line segment endpoint
  x1,y1 - second endpoint

Exit:
  x0,y0,x1,y1 - clipped endpoint positions
  returns 1 if segment is at least partially in window,
  returns 0 if segment is entirely outside window
******************************************************************************/

int clip_line(float &x0, float &y0, float &x1, float &y1)
{
  int code04 = (x0 < xmin) ? 1 : 0;
  int code03 = (x0 > xmax) ? 1 : 0;
  int code02 = (y0 < ymin) ? 1 : 0;
  int code01 = (y0 > ymax) ? 1 : 0;

  int code14 = (x1 < xmin) ? 1 : 0;
  int code13 = (x1 > xmax) ? 1 : 0;
  int code12 = (y1 < ymin) ? 1 : 0;
  int code11 = (y1 > ymax) ? 1 : 0;

#if 0
printf ("clip segment: %f %f %f %f\n", x0, y0, x1, y1);
#endif

  int sum0 = code01 + code02 + code03 + code04;
  int sum1 = code11 + code12 + code13 + code14;

#if 0
printf ("sum0 sum1: %d %d\n", sum0, sum1);
#endif

  /* completely inside window? */
  if (sum0 == 0 && sum1 == 0)
    return (1);

  /* check for trivial invisibility (both endpoints on wrong side of */
  /* a single side of the window) */

  if (code01 && code11 || code02 && code12 || code03 && code13 ||
      code04 && code14) {
#if 0
printf ("trivially invisible\n");
#endif
    return (0);
  }

  /* compute run and rise */
  float dx = x1 - x0;
  float dy = y1 - y0;

  /* case: only x0,y0 is inside window */
  if (sum0 == 0) {
    int dummy = clip_helper (dx, dy, x1, y1, x1, y1);
#if 0
printf ("clip only x1,y1\n");
#endif
    return (1);
  }

  /* case: only x1,y1 is inside window */
  if (sum1 == 0) {
    int dummy = clip_helper (dx, dy, x0, y0, x0, y0);
#if 0
printf ("clip only x0,y0\n");
#endif
    return (1);
  }

  /* neither endpoint is inside the window */

  int count = 0;
  count += clip_helper (dx, dy, x0, y0, x0, y0);
  count += clip_helper (dx, dy, x1, y1, x1, y1);

#if 0
printf ("clip both endpoints\n");
#endif

  if (count)
    return (1);
  else
    return (0);
}

