//
//  floating-point image class
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

#ifndef _FLOAT_IMAGE_CLASS_
#define _FLOAT_IMAGE_CLASS_

#include <errno.h>
#include "window.h"

class FloatImage {
  int xsize,ysize;
  float saved_min,saved_max;
  float *pixels;
  float aspect;
  float aspect_recip;
public:
  FloatImage(char *);
  FloatImage(int w, int h) {
    xsize = w;
    ysize = h;
    aspect = h / (float) w;
    aspect_recip = 1.0 / aspect;
    pixels = new float [xsize * ysize];
  }
  ~FloatImage() {
    delete pixels;
  }

  // pixel addressing
  float& operator () (int x, int y) { return (pixels[y * xsize + x]); }
  float& pixel(int x, int y) { return (pixels[y * xsize + x]); }
  float& operator () (int index) { return (pixels[index]); }
  float& pixel(int index) { return (pixels[index]); }

  float get_value (float x, float y);

  // setting all values in an image
  void operator = (float val) {
    for (int i = 0; i < xsize*ysize; i++)
      pixels[i] = val;
  }
  void setimage (float val) {
    for (int i = 0; i < xsize*ysize; i++)
      pixels[i] = val;
  }

  int getwidth() { return (xsize); }
  int getheight() { return (ysize); }
  int getsize() { return (xsize * ysize); }
  float getaspect() { return (aspect); }
  void get_extrema(float&,float&);
  FloatImage *normalize();
  void gradient (float, float, float *, float *);
  void draw(Window2d *);
  void update_row(Window2d *, int);
  void draw_clamped(Window2d *, float, float);
  int read_pgm(char *);
  void write_pgm(char *, float, float);
  int read_file(char *);
  void write_file(char *);
  FloatImage *copy();
  void blur(int);
  void remap(float,float);
  void bias(float);
  void gain(float);
  void contrast(float) {}
};

#endif /* _FLOAT_IMAGE_CLASS_ */

