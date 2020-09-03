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
#include "sd_picture.h"


/******************************************************************************
Create a picture that will draw to a given window.

Entry:
  w - window to draw to; must have already been created
******************************************************************************/

Picture::Picture(Window2d *w)
{
  win = w;
  type = PICT_WINDOW_TYPE;

  int x, y;
  w->getsize(&x, &y);
  aspect_ratio = y / (float) x;
}


/******************************************************************************
Create a picture that will write a Postscript file.

Entry:
  aspect   - height over width
  filename - name of text file to write to
******************************************************************************/

Picture::Picture(float aspect, char *filename)
{
  type = PICT_POSTSCRIPT_TYPE;
  aspect_ratio = aspect;

  file_out = new std::ofstream(filename);
  write_postscript_start(file_out);
}


/******************************************************************************
Set the intensity of future lines and polygons.
******************************************************************************/

void Picture::set_intensity(float val)
{
  if (type == PICT_WINDOW_TYPE) {
    int intensity = (int) (255 * val);
    win->set_color_index(intensity);
  } else if (type == PICT_POSTSCRIPT_TYPE) {
    *file_out << (1 - val) << " setgray" << std::endl;
  }
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
  } else if (type == PICT_POSTSCRIPT_TYPE) {
    *file_out << thickness << " sw" << std::endl;
    *file_out << x1 << " " << y1 << " " << x2 << " " << y2 << " ln" << std::endl;
  }
}


/******************************************************************************
Start the definition of a polygon.
******************************************************************************/

void Picture::polygon_start()
{
  if (type == PICT_WINDOW_TYPE)
    win->polygon_start();
  else if (type == PICT_POSTSCRIPT_TYPE) {
    *file_out << "newpath " << std::endl;
    first_vertex = 1;
  }
}


/******************************************************************************
Define a vertex of a polygon.
******************************************************************************/

void Picture::polygon_vertex(float x, float y)
{
  if (type == PICT_WINDOW_TYPE)
    win->polygon_vertex(x, y);
  else if (type == PICT_POSTSCRIPT_TYPE) {
    if (first_vertex) {
      *file_out << x << " " << y << " moveto" << std::endl;
      first_vertex = 0;
    } else
      *file_out << x << " " << y << " lineto" << std::endl;
  }
}


/******************************************************************************
Fill a polygon that has been defined.
******************************************************************************/

void Picture::polygon_fill()
{
  if (type == PICT_WINDOW_TYPE)
    win->polygon_fill();
  else if (type == PICT_POSTSCRIPT_TYPE)
    *file_out << " closepath fill" << std::endl;
}


/******************************************************************************
Clear the window, if the picture is a window.
******************************************************************************/

void Picture::clear()
{
  if (type == PICT_WINDOW_TYPE)
    win->clear();
}


/******************************************************************************
Flush any buffers associated with the picture.
******************************************************************************/

void Picture::flush()
{
  if (type == PICT_WINDOW_TYPE)
    win->flush();
  else if (type == PICT_POSTSCRIPT_TYPE)
    file_out->flush();
}


/******************************************************************************
Write the start of a postscript file.
******************************************************************************/

void write_postscript_start(std::ofstream *file_out)
{
  /* header comment */

  *file_out << "%!" << std::endl;
  *file_out << "%" << std::endl;
  *file_out << "% Streamlines" << std::endl;
  *file_out << "%" << std::endl;

  /* scale and translate picture properly */

  *file_out << "/size 6 def" << std::endl;
  *file_out << "72 72 3 mul translate" << std::endl;
  *file_out << "72 size mul dup scale" << std::endl;
  *file_out << std::endl;

  /* definition for line drawing */

  *file_out << "/ln {" << std::endl;
  *file_out << "newpath" << std::endl;
  *file_out << "moveto lineto" << std::endl;
  *file_out << "stroke" << std::endl;
  *file_out << "} def" << std::endl;
  *file_out << std::endl;

  /* setting line widths */

  *file_out << "1 72 div size div setlinewidth" << std::endl;
  *file_out << "/init_width { 1 72 div size div setlinewidth } def" << std::endl;
  *file_out << "/sw { 72 div size div setlinewidth } def" << std::endl;
}


/******************************************************************************
Write the end of a postscript file.
******************************************************************************/

void write_postscript_end(std::ofstream *file_out, float aspect_ratio)
{
  /* border around image */

  *file_out << std::endl;
  *file_out << "0 setgray" << std::endl;
  *file_out << "2 72 div size div setlinewidth" << std::endl;
  *file_out << "0 0 moveto" << std::endl;
  *file_out << "1 0 lineto" << std::endl;
  *file_out << "1 " << aspect_ratio << " lineto" << std::endl;
  *file_out << "0 " << aspect_ratio << " lineto" << std::endl;
  *file_out << "closepath stroke" << std::endl;
  *file_out << std::endl;

  /* show the page */

  *file_out << std::endl;
  *file_out << "showpage" << std::endl;
  *file_out << std::endl;
}

