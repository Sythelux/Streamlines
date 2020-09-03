/*

Drawing to a window or to a Postscript file.  The goal of this class is to
be able to write drawing code (such as arrows for streamlines) that doesn't
have to know whether it is drawing to a window or a to file.

Greg Turk, July 1996

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
#include "picture.h"


/******************************************************************************
Create a picture that will draw to a given window.

Entry:
  w - window to draw to; must have already been created
******************************************************************************/

Picture::Picture(Window2d *w)
{
  win = w;
}


/******************************************************************************
Create a picture that will write a Postscript file.

Entry:
  width    - width of image, in inches
  aspect   - height over width
  portrait - orient drawing in portrait mode (1) or landscape mode (0)
  filename - name of text file to write to
******************************************************************************/

Picture::Picture(float width, float aspect, int portrait, char *filename)
{

}


/******************************************************************************
Set the intensity of future lines and polygons.
******************************************************************************/

void Picture::set_intensity(float val)
{
}


/******************************************************************************
Draw a thick line.

Entry:
  x1,y1     - one endpoint of line
  x2,y2     - the other endpoint
  thickness - the line thickness
******************************************************************************/

void Picture::thick_line(
        float x1,
        float y1,
        float x2,
        float y2,
        float thickness
)
{
  if (type == PICT_WINDOW_TYPE) {
    int t = (int) thickness;
    win->thick_line(x1, y1, x2, y2, t);
  }
}


/******************************************************************************
Start the definition of a polygon.
******************************************************************************/

void Picture::polygon_start()
{
  if (type == PICT_WINDOW_TYPE)
    win->polygon_start();
}


/******************************************************************************
Define a vertex of a polygon.
******************************************************************************/

void Picture::polygon_vertex(float x, float y)
{
  if (type == PICT_WINDOW_TYPE)
    win->polygon_vertex(x, y);
}


/******************************************************************************
Fill a polygon that has been defined.
******************************************************************************/

void Picture::polygon_fill()
{
  if (type == PICT_WINDOW_TYPE)
    win->polygon_fill();
}

