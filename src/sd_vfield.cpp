/*

Vector field manipulation.

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
#include <strings.h>
#include <fstream>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <unistd.h>
#include <fcntl.h>
#include "window.h"
#include "floatimage.h"
#include "sd_vfield.h"

static int integrator = MIDPOINT;


/******************************************************************************
Create a new vector field by reading in a file.
******************************************************************************/

VectorField::VectorField(char *filename)
{
  int index = 0;
  int done = 0;
  int linefeeds = 0;
  int zsize;
  int rank;

  /* append ".vec" to the file name if necesary */

  char name[80];
  strcpy (name, filename);
  if (strlen (name) < 4 || strcmp (name + strlen (name) - 4, ".vec") != 0)
    strcat (name, ".vec");

  ifstream infile(name, ios::in);

  if (!infile) {
    fprintf (stderr, "Error openning '%s'\n", name);
    exit (-1);
  }

  printf ("reading %s\n", filename);

  char *str = new char[80];

  while (!done) {
    infile.get (str[index]);
    if (str[index] == '\n')
      linefeeds++;
    if (linefeeds == 3 || index >= 100)
      done = 1;
    index++;
  }
  str[index] = '\0';

  if (index >= 100) {
    fprintf (stderr, "Header bad in file.\n");
    exit (-1);
  }

  sscanf (str, "VF\n%d %d %d\n%d\n", &xsize, &ysize, &zsize, &rank);
  aspect = ysize / (float) xsize;
  aspect_recip = 1.0 / aspect;

//  cout << "size: " << xsize << " " << ysize << endl;

  values = new float[xsize * ysize * 2];
  infile.read ((char *)values, xsize * ysize * 2 * sizeof (float));

  infile.close();
}


/******************************************************************************
Write the vector field to a file.
******************************************************************************/

void VectorField::write_file(char *filename)
{
  int zsize = 1;
  int rank = 2;

  /* append ".vec" to the file name if necesary */

  char name[80];
  strcpy (name, filename);
  if (strlen (name) < 4 || strcmp (name + strlen (name) - 4, ".vec") != 0)
    strcat (name, ".vec");

  int fd = open(name, O_CREAT|O_TRUNC|O_WRONLY, 0666);
  if (fd < 0) {
    fprintf(stderr, "Unable to open %s: %s\n", name, strerror(errno));
    exit(-1);
  }

  char str[80];
  sprintf (str, "VF\n%d %d %d\n%d\n", xsize, ysize, zsize, rank);
  int cc = write(fd, str, strlen(str));
  if (cc == -1) {
    fprintf (stderr, "Can't write to file.\n");
    exit (-1);
  }

  cc = write(fd, (char *) values, sizeof(float) * xsize * ysize * 2);

  if (cc != sizeof(float) * xsize * ysize * 2) {
    fprintf(stderr, "Write returned short: %s\n", strerror(errno));
    close(fd);
    exit(-1);
  }

  close(fd);
}


/******************************************************************************
Return the interpolated X vector value of a given position in a vector field.

Entry:
  x,y - position in vector field, in [0,1] x [0,1]

Exit:
  returns interpolated x value
******************************************************************************/

float VectorField::xval(float x, float y)
{
  if (x < 0 || x > 1 || y < 0 || y > aspect)
    return (0.0);

  x = x * (xsize-1);
  y = y * (ysize-1) * aspect_recip;

  int i = (int) x;
  int j = (int) y;

  float xfract = x - i;
  float yfract = y - j;

  int index = 2 * (j * xsize + i);
  float x00 = values[index];
  float x01 = values[index + 2];
  float x10 = values[index + 2 * xsize];
  float x11 = values[index + 2 * xsize + 2];

  float x0 = x00 + xfract * (x01 - x00);
  float x1 = x10 + xfract * (x11 - x10);
  float xinterp = x0 + yfract * (x1 - x0);

  return (xinterp);
}


/******************************************************************************
Return the interpolated Y vector value of a given position in a vector field.

Entry:
  x,y - position in vector field, in [0,1] x [0,1]

Exit:
  returns interpolated y value
******************************************************************************/

float VectorField::yval(float x, float y)
{
  if (x < 0 || x > 1 || y < 0 || y > aspect)
    return (0.0);

  x = x * (xsize-1);
  y = y * (ysize-1) * aspect_recip;

  int i = (int) x;
  int j = (int) y;

  float xfract = x - i;
  float yfract = y - j;

  int index = 2 * (j * xsize + i) + 1;
  float y00 = values[index];
  float y01 = values[index + 2];
  float y10 = values[index + 2 * xsize];
  float y11 = values[index + 2 * xsize + 2];

  float y0 = y00 + xfract * (y01 - y00);
  float y1 = y10 + xfract * (y11 - y10);
  float yinterp = y0 + yfract * (y1 - y0);

  return (yinterp);
}


/******************************************************************************
Return the interpolated X and Y vector value of a given position in a
vector field.

Entry:
  x,y       - position in vector field, in [0,1] x [0,1]
  normalize - 1 if we're to normalize the result, 0 if not

Exit:
  xval,yval - the interpolated x and y values
  returns the length of the vector
******************************************************************************/

float VectorField::xyval(
  float x,
  float y,
  int normalize,
  float& xval,
  float& yval
)
{
  int index;
  float xv,yv;

  if (x < 0 || x > 1 || y < 0 || y > aspect) {
    xval = 0.0;
    yval = 0.0;
    return (0.0);
  }

  x = x * (xsize-1);
  y = y * (ysize-1) * aspect_recip;

  int i = (int) x;
  int j = (int) y;

  float xfract = x - i;
  float yfract = y - j;

  index = 2 * (j * xsize + i);
  float x00 = values[index];
  float x01 = values[index + 2];
  float x10 = values[index + 2 * xsize];
  float x11 = values[index + 2 * xsize + 2];

  float x0 = x00 + xfract * (x01 - x00);
  float x1 = x10 + xfract * (x11 - x10);
  xv = x0 + yfract * (x1 - x0);

  index = 2 * (j * xsize + i) + 1;
  float y00 = values[index];
  float y01 = values[index + 2];
  float y10 = values[index + 2 * xsize];
  float y11 = values[index + 2 * xsize + 2];

  float y0 = y00 + xfract * (y01 - y00);
  float y1 = y10 + xfract * (y11 - y10);
  yv = y0 + yfract * (y1 - y0);

  float len = sqrt (xv * xv + yv * yv);

  if (normalize) {
    if (len != 0.0) {
      xv /= len;
      yv /= len;
    }
  }

  xval = xv;
  yval = yv;

  return (len);
}


/******************************************************************************
Set the type of the integrator (EULER, MIDPOINT, RUNGE_KUTTA).
******************************************************************************/

void set_integration(int type)
{
  integrator = type;
}


/******************************************************************************
Take one integration step within the vector field.

Entry:
  x,y       - initial position
  delta     - step size
  normalize - whether to normalize the step lengths

Exit:
  xnew,ynew - new position
  returns the length of the step
******************************************************************************/
float VectorField::integrate(
  float x,
  float y,
  float delta,
  int normalize,
  float& xnew,
  float& ynew
)
{
  float xv,yv;
  float len;

  if (integrator == EULER) {

    len = xyval (x, y, normalize, xv, yv);
    xnew = x + delta * xv;
    ynew = y + delta * yv;

  }
  else if (integrator == MIDPOINT) {

    len = xyval (x, y, normalize, xv, yv);
    float x2 = x + 0.5 * delta * xv;
    float y2 = y + 0.5 * delta * yv;
    len = xyval (x2, y2, normalize, xv, yv);
    xnew = x + delta * xv;
    ynew = y + delta * yv;

  }
  else if (integrator == RUNGE_KUTTA) {
    return (0.0);
  }
  else {
    fprintf (stderr, "Invalid integrator: %d\n", integrator);
    return (0.0);
  }

  return (len);
}


/******************************************************************************
Make all non-zero vectors have magnitude 1.
******************************************************************************/

void VectorField::normalize()
{
  for (int i = 0; i < xsize * ysize * 2; i += 2) {
    float x2 = values[i] * values[i];
    float y2 = values[i+1] * values[i+1];
    float len = sqrt (x2 + y2);
    if (len != 0) {
      float recip = 1 / len;
      values[i] *= recip;
      values[i+1] *= recip;
    }
  }
}


/******************************************************************************
Return a scalar image that contains the magnitude of the vector field.
******************************************************************************/

FloatImage *VectorField::get_magnitude()
{
  FloatImage *image = new FloatImage(xsize, ysize);

  for (int i = 0; i < xsize * ysize * 2; i += 2) {
    float x = values[i];
    float y = values[i+1];
    image->pixel(i/2) = sqrt (x*x + y*y);
  }

  return (image);
}


/******************************************************************************
Return a scalar image that contains the vorticity of a vector field.

Entry:
  xs,ys - size of image to return

Exit:
  the image containing vorticity
******************************************************************************/

FloatImage *VectorField::get_vorticity(int xs, int ys)
{
  int i,j;

  FloatImage *image = new FloatImage(xsize-2, ysize-2);

  float d = 0.1 / xsize;

  for (i = 1; i < xsize-1; i++)
    for (j = 1; j < ysize-1; j++) {
      float dx = yval(i+1, j) - yval(i-1, j);
      float dy = xval(i, j+1) - xval(i, j-1);
      float vort = (dx/d) - (dy/d);
      image->pixel(i-1, j-1) = vort;
    }

  FloatImage *image2 = new FloatImage(xs, ys);

  for (i = 0; i < xs; i++)
    for (j = 0; j < ys; j++) {
      float x = (i + 0.5) / xs;
      float y = (j + 0.5) / ys;
      image2->pixel(i,j) = image->get_value(x,y);
    }

  delete image;
  return (image2);
}


/******************************************************************************
Return a scalar image that contains the divergence of a vector field.

Entry:
  xs,ys - size of image to return

Exit:
  the image containing divergence
******************************************************************************/

FloatImage *VectorField::get_divergence(int xs, int ys)
{
  int i,j;

  FloatImage *image = new FloatImage(xsize-2, ysize-2);

  float d = 0.1 / xsize;

  for (i = 1; i < xsize-1; i++)
    for (j = 1; j < ysize-1; j++) {
      float dx = xval(i+1, j) - xval(i-1, j);
      float dy = yval(i, j+1) - yval(i, j-1);
      float div = (dx + dy) / (2 * d);
      image->pixel(i-1, j-1) = div;
    }

  FloatImage *image2 = new FloatImage(xs, ys);

  for (i = 0; i < xs; i++)
    for (j = 0; j < ys; j++) {
      float x = (i + 0.5) / xs;
      float y = (j + 0.5) / ys;
      image2->pixel(i,j) = image->get_value(x,y);
    }

  delete image;
  return (image2);
}

