//
//  Picture class that allows drawing into a window or to a Postscript file
//

/*
Copyright (c) 1996 The University of North Carolina.  All rights reserved.   
  
Permission to use, copy, modify and distribute this software and its   
documentation for any purpose is hereby granted without fee, provided   
that the above copyright notice and this permission notice appear in   
all copies of this software and that you do not sell the software.   
  
The software is provided "as is" and without warranty of any kind,   
express, implied or otherwise, including without limitation, any   
warranty of merchantability or fitness for a particular purpose.   
*/

#include <math.h>
#include <iostream>
#include "window.h"

#ifndef _PICTURE_CLASS_
#define _PICTURE_CLASS_

#define  PICT_WINDOW_TYPE      1
#define  PICT_POSTSCRIPT_TYPE  2

extern void write_postscript_start (ofstream *);
extern void write_postscript_end (ofstream *, float);

class Picture {

  int type;            /* window or postscript file? */
  int portrait;        /* portrait or landscape, if Postscript */
  Window2d *win;       /* window to draw to */
  ofstream *file_out;  /* postscript file to draw to */
  int first_vertex;    /* whether we're still on the 1st vert of a polygon */
  float aspect_ratio;  /* ratio of height to width */

public:

  Picture(Window2d*);
  Picture(float,char*);
  ~Picture() {
    flush();
    if (type == PICT_POSTSCRIPT_TYPE) {
      write_postscript_end (file_out, aspect_ratio);
      delete file_out;
    }
  }
  void polygon_start();
  void polygon_vertex(float,float);
  void polygon_fill();
  void thick_line(float,float,float,float,float);
  void set_intensity(float);
  void clear();
  void flush();
};

#endif /* _PICTURE_CLASS_ */

