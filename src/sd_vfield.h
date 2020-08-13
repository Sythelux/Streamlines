//
//  vector field class
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

#ifndef _VECTOR_FIELD_CLASS_
#define _VECTOR_FIELD_CLASS_

#include "floatimage.h"

class VectorField {
  float *values;
  float aspect;
  float aspect_recip;
public:
  int xsize,ysize;
  VectorField(int w, int h) {
    xsize = w;
    ysize = h;
    aspect = h / (float) w;
    aspect_recip = 1.0 / aspect;
    values = new float [xsize * ysize * 2];
  }
  VectorField(char *filename);
  ~VectorField() {
    delete values;
  }
  float xval(float x, float y);
  float yval(float x, float y);
  float xyval(float, float, int, float&, float&);
  float& xval(int x, int y) {
    return (values[2 * (y * xsize + x)]);
  }
  float& xval(int index) {
    return (values[2 * index]);
  }
  float& yval(int x, int y) {
    return (values[2 * (y * xsize + x) + 1]);
  }
  float& yval(int index) {
    return (values[2 * index + 1]);
  }
  float integrate(float,float,float,int,float&,float&);
  int getwidth() { return (xsize); }
  int getheight() { return (ysize); }
  float getaspect() { return (aspect); }
  FloatImage *get_magnitude();
  FloatImage *get_vorticity(int,int);
  FloatImage *get_divergence(int,int);
  void normalize();
  void write_file (char *);
};

void set_integration(int);  /* set which type of integrator to use */

#define EULER        1
#define MIDPOINT     2
#define RUNGE_KUTTA  3

#endif /* _VECTOR_FIELD_CLASS_ */

