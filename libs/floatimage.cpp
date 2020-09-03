/*

Floating-point image class.

Greg Turk, 1995

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

#include <stdio.h>
#include <strings.h>
#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include <cstring>
#include "window.h"
#include "floatimage.h"


/******************************************************************************
Re-map the range of values in the image.

Entry:
  tmin - target minimum value
  tmax - target maximum value
******************************************************************************/

void FloatImage::remap(float tmin, float tmax)
{
  float min, max;

  /* make sure the target values are ordered properly */

  if (tmin > tmax) {
    float temp = tmin;
    tmin = tmax;
    tmax = temp;
  }

  /* get extrema in the current image */

  get_extrema(min, max);

  /* avoid division by zero */
  if (min == max) {
    min = max - 1.0;
  }

  for (int i = 0; i < xsize * ysize; i++) {
    float t = (pixels[i] - min) / (max - min);
    pixels[i] = tmin + t * (tmax - tmin);
  }
}


/******************************************************************************
Change the bias (contrast) of the image.

Entry:
  b - bias value (0.5 = no change)
******************************************************************************/

void FloatImage::bias(float b)
{
  /* range check on bias value */

  if (b < 0 || b > 1) {
    fprintf(stderr, "bias: bad value = %f\n", b);
    return;
  }

  /* get extrema of the current image */

  float min, max;
  get_extrema(min, max);

  /* avoid division by zero */
  if (min == max)
    return;

  float expon = log(b) / log(0.5);

  for (int i = 0; i < xsize * ysize; i++) {

    /* map to [0,1] */
    float t = (pixels[i] - min) / (max - min);

    /* perform bias */
    float val = pow(t, expon);

    /* map back to original range */
    pixels[i] = min + val * (max - min);
  }
}


/******************************************************************************
Change the gain (spread away from center) of the image.

Entry:
  g - gain value (0.5 = no change)
******************************************************************************/

void FloatImage::gain(float g)
{
  /* range check on gain value */

  if (g < 0 || g > 1) {
    fprintf(stderr, "gain: bad value = %f\n", g);
    return;
  }

  /* get extrema of the current image */

  float min, max;
  get_extrema(min, max);

  /* avoid division by zero */
  if (min == max)
    return;

  float expon = log(1 - g) / log(0.5);

  for (int i = 0; i < xsize * ysize; i++) {

    /* map to [0,1] */
    float t = (pixels[i] - min) / (max - min);

    /* perform gain */

    float val;

    if (t < 0.5)
      val = pow(2 * t, expon) * 0.5;
    else
      val = 1 - pow(2 - 2 * t, expon) * 0.5;

    /* map back to original range */
    pixels[i] = min + val * (max - min);
  }
}


/******************************************************************************
Return a copy of an image.
******************************************************************************/

FloatImage *FloatImage::copy()
{
  FloatImage *image = new FloatImage(xsize, ysize);

  for (int i = 0; i < xsize * ysize; i++)
    image->pixel(i) = pixels[i];

  return (image);
}


/******************************************************************************
Return the value of an image at a particular (floating-point) location.  This
uses linear interpolation.

Entry:
  x,y - position in image; x is in [0,1] and y is in [0,aspect]

Exit:
  returns value at the given position, or zero if we're off the image
******************************************************************************/

float FloatImage::get_value(float x, float y)
{
  if (x < 0 || x > 1 || y < 0 || y > aspect)
    return (0.0);

  y *= aspect_recip;

  x = x * (xsize - 1);
  y = y * (ysize - 1);

  int i = (int) x;
  int j = (int) y;

  float xfract = x - i;
  float yfract = y - j;

  if (i >= xsize - 1) {
    i = xsize - 2;
    xfract = 1.0;
  }

  if (j >= ysize - 1) {
    j = ysize - 2;
    yfract = 1.0;
  }

  int index = j * xsize + i;
  float x00 = pixels[index];
  float x01 = pixels[index + 1];
  float x10 = pixels[index + xsize];
  float x11 = pixels[index + xsize + 1];

  float x0 = x00 + xfract * (x01 - x00);
  float x1 = x10 + xfract * (x11 - x10);
  float xinterp = x0 + yfract * (x1 - x0);

  return (xinterp);
}


/******************************************************************************
Compute the gradient at a given point.

Entry:
  x,y - point at which to compute the gradient

Exit:
  gx,gy - the computed gradient
******************************************************************************/

void FloatImage::gradient(float x, float y, float *gx, float *gy)
{

#if 0
  *gx = drand48() - 0.5;
  *gy = drand48() - 0.5;
#endif

#if 1

  int i = (int) (x * xsize);
  int j = (int) (y * ysize * aspect_recip);
  *gx = pixel(i + 1, j) - pixel(i - 1, j);
  *gy = pixel(i, j + 1) - pixel(i, j - 1);

#endif

}


/******************************************************************************
Blur an image by diffusion.

Entry:
  steps - number of diffusion steps
******************************************************************************/

void FloatImage::blur(int steps)
{
  int i, j;
  int i0, i1, j0, j1;
  float val;
  FloatImage *image = new FloatImage(xsize, ysize);

  /* blur several times */

  for (int k = 0; k < steps; k++) {

    /* one step of blurring */

    for (i = 0; i < xsize; i++) {

      i0 = i - 1;
      i1 = i + 1;
      if (i == 0)
        i0 = 0;
      if (i == xsize - 1)
        i1 = xsize - 1;

      for (j = 0; j < ysize; j++) {

        j0 = j - 1;
        j1 = j + 1;
        if (j == 0)
          j0 = 0;
        if (j == ysize - 1)
          j1 = ysize - 1;

        val = pixel(i0, j) + pixel(i1, j) + pixel(i, j0) + pixel(i, j1);
        val += 4 * pixel(i, j);
        val *= 0.125;
        image->pixel(i, j) = val;
      }
    }

    /* copy result into original array */
    for (i = 0; i < xsize; i++)
      for (j = 0; j < ysize; j++)
        pixel(i, j) = image->pixel(i, j);
  }

  delete image;
}


/******************************************************************************
Find the minimum and maximum values of an image.

Exit:
  min - minimum value
  max - maximum value
******************************************************************************/

void FloatImage::get_extrema(float &min, float &max)
{
  int count = getwidth() * getheight();

  /* find minimum and maximum values */

  min = pixel(0);
  max = pixel(0);

  for (int i = 0; i < count; i++) {
    if (pixel(i) < min)
      min = pixel(i);
    if (pixel(i) > max)
      max = pixel(i);
  }
}


/******************************************************************************
Create a new image that is a normalized version of another image.  This means
all values are scaled to the interval [0,1].

Entry:
  image - image to normalize

Exit:
  returns normalized image
******************************************************************************/

FloatImage *FloatImage::normalize()
{
  int count = getwidth() * getheight();

  FloatImage *newimage = new FloatImage(getwidth(), getheight());

  /* find minimum and maximum values */

  float min, max;
  get_extrema(min, max);
  if (max == min)
    min = max - 1;

  float scale = 255 * (max - min);
  float trans = -min;

  for (int i = 0; i < count; i++)
    newimage->pixel(i) = (pixel(i) - min) / (max - min);

  return (newimage);
}


/******************************************************************************
Update one row of an image.  Must have called "FloatImage::draw()" first
because of the max and min values.

Entry:
  win - window to display in
  row - row to display
******************************************************************************/

void FloatImage::update_row(Window2d *win, int row)
{
  int i, j;
  int win_width, win_height;
  int dup;

  win->getsize(&win_width, &win_height);

  dup = (int) (win_width / xsize);

  unsigned char *image_data;
  image_data = new unsigned char[xsize * dup * dup];

  for (j = 0; j < dup; j++)
    for (i = 0; i < xsize * dup; i++) {
      int ii = (int) (i / dup);
      image_data[i + j * xsize] =
              (int) (255.0 * (pixel(ii, ysize - row - 1) - saved_min) /
                     (saved_max - saved_min));
    }

  win->draw_offset_image(xsize * dup, dup, image_data, 0, row * dup);
  delete image_data;
}


/******************************************************************************
Display an image containing floats in a window.

Entry:
  win - window to display in
******************************************************************************/

void FloatImage::draw(Window2d *win)
{
  int i, j;
  int count = xsize * ysize;
  int win_width, win_height;
  int dup;

  win->getsize(&win_width, &win_height);

  /* find minimum and maximum values */

  float min, max;
  get_extrema(min, max);

  std::cout << "min max: " << min << " " << max << std::endl;

  if (max == min)
    min = max - 1;

  saved_min = min;
  saved_max = max;

  dup = (int) (win_width / xsize);

  unsigned char *image_data;
  image_data = new unsigned char[count * dup * dup];

  for (j = 0; j < ysize * dup; j++)
    for (i = 0; i < xsize * dup; i++) {
      int ii = (int) (i / dup);
      int jj = (int) (j / dup);
      image_data[i + j * xsize * dup] =
              (int) (255.0 * (pixel(ii, jj) - min) / (max - min));
    }

  win->clear();
  win->draw_image(xsize * dup, ysize * dup, image_data);
  delete image_data;
}


/******************************************************************************
Display an image containing floats in a window.  Values are clamped to be
within a specified range.

Entry:
  win - window to display in
  min - minimum of range
  max - maximum of range
******************************************************************************/

void FloatImage::draw_clamped(Window2d *win, float min, float max)
{
  int i, j;
  int count = xsize * ysize;
  int win_width, win_height;
  int dup;

  win->getsize(&win_width, &win_height);

  dup = (int) (win_width / xsize);

  unsigned char *image_data;
  image_data = new unsigned char[count * dup * dup];

  for (j = 0; j < ysize * dup; j++)
    for (i = 0; i < xsize * dup; i++) {

      int ii = (int) (i / dup);
      int jj = (int) (j / dup);
      int val = (int) (255 * (pixel(ii, jj) - min) / (max - min));

      /* clamp values */
      if (val < 0) val = 0;
      if (val > 255) val = 255;
      image_data[i + j * xsize * dup] = val;
    }

  win->clear();
  win->draw_image(xsize * dup, ysize * dup, image_data);
  delete image_data;
}


/******************************************************************************
Get header information (first few lines) from a file.

Entry:
  fd     - file descriptor
  nfeeds - number of lines to get

Exit:
  returns pointer to header info
******************************************************************************/

static char *get_header(int fd, int nfeeds)
{
  char *str = (char *) malloc(100);

  int index = 0;
  int linefeeds = 0;
  int done = 0;

  while (!done) {
    read(fd, &str[index], 1);
    if (str[index] == '\n')
      linefeeds++;
    if (linefeeds == nfeeds || index >= 100)
      done = 1;
    index++;
  }
  str[index] = '\0';

  if (index >= 100) {
    fprintf(stderr, "Header bad in file.\n");
    exit(-1);
  }

  return (str);
}


/******************************************************************************
Read an image from a PGM (portable gray map) file.

Entry:
  filename - name of PGM file to read

Exit:
  places the contents of the PGM file into the image
  returns 0 if successful, 1 if an error occured
******************************************************************************/

int FloatImage::read_pgm(char *filename)
{
  /* append ".pgm" to the file name if necesary */

  char name[80];
  strcpy(name, filename);
  if (strlen(name) < 4 || strcmp(name + strlen(name) - 4, ".pgm") != 0)
    strcat(name, ".pgm");

  int fd = open(name, O_RDONLY);

  if (fd < 0) {
    fprintf(stderr, "Unable to open %s: %s\n", filename, strerror(errno));
    return (1);
  }

  char *str = get_header(fd, 3);
  sscanf(str, "P5\n%d %d\n255\n", &xsize, &ysize);
  aspect = ysize / (float) xsize;
  aspect_recip = 1.0 / aspect;
  int img_size = xsize * ysize * sizeof(unsigned char);

  unsigned char *data = new unsigned char[img_size];

  if (data == NULL) {
    fprintf(stderr, "Unable to allocate %d bytes for input image\n",
            img_size);
    return (1);
  }

  if (read(fd, data, img_size) != img_size) {
    fprintf(stderr, "Unable to read %d bytes from input image\n", img_size);
    return (1);
  }

  close(fd);

  /* convert [0,255] data into the range [0,1] */

  pixels = new float[xsize * ysize];

  for (int i = 0; i < xsize * ysize; i++)
    pixels[i] = data[i] / 255.0;

  free(data);

  /* end successfully */
  return (0);
}


/******************************************************************************
Read an image from a PGM (portable gray map) file.

Entry:
  filename - name of PGM file to read

Exit:
  returns a newly created FloatImage
******************************************************************************/

FloatImage::FloatImage(char *filename)
{
  read_pgm(filename);
}


/******************************************************************************
Write an image to a PGM (portable gray map) file.

Entry:
  filename - name of PGM file to write
  mini,maxi  - range to clamp to; if both zero, calculate range
******************************************************************************/

void FloatImage::write_pgm(char *filename, float mini, float maxi)
{
  int i, j;
  int count = xsize * ysize;

  /* maybe find minimum and maximum values */

  float min = mini;
  float max = maxi;

  if (min == 0 && max == 0) {

    get_extrema(min, max);

    if (max == min)
      min = max - 1;
  }

  unsigned char *image_data;
  image_data = new unsigned char[count];

  for (j = 0; j < ysize; j++)
    for (i = 0; i < xsize; i++) {

      int val = (int) (255 * (pixel(i, j) - min) / (max - min));

      /* clamp values */
      if (val < 0) val = 0;
      if (val > 255) val = 255;
      image_data[i + j * xsize] = val;
    }

  int fd = open(filename, O_CREAT | O_TRUNC | O_WRONLY, 0666);
  if (fd < 0) {
    fprintf(stderr, "Unable to open %s: %s\n", filename, strerror(errno));
    exit(-1);
  }

  char str[80];
  sprintf(str, "P5\n%d %d\n255\n", xsize, ysize);
  int cc = write(fd, str, strlen(str));
  if (cc == -1) {
    fprintf(stderr, "Can't write to file.\n");
    exit(-1);
  }

  cc = write(fd, image_data, sizeof(unsigned char) * xsize * ysize);

  if (cc != sizeof(unsigned char) * xsize * ysize) {
    fprintf(stderr, "Write returned short: %s\n", strerror(errno));
    close(fd);
    exit(-1);
  }

  close(fd);
}


/******************************************************************************
Read an image from a binary file.

Entry:
  filename - name of scalar file to read

Exit:
  places the contents of the file into the image
  returns 0 if successful, 1 if an error occured
******************************************************************************/

int FloatImage::read_file(char *filename)
{
  /* append ".flt" to the file name if necesary */

  char name[80];
  strcpy(name, filename);
  if (strlen(name) < 4 || strcmp(name + strlen(name) - 4, ".flt") != 0)
    strcat(name, ".flt");

  int fd = open(name, O_RDONLY);

  if (fd < 0) {
    fprintf(stderr, "Unable to open %s: %s\n", filename, strerror(errno));
    return (1);
  }

  char *str = get_header(fd, 2);
  sscanf(str, "FL\n%d %d\n", &xsize, &ysize);
  aspect = ysize / (float) xsize;
  aspect_recip = 1.0 / aspect;

  pixels = new float[xsize * ysize];
  int img_size = xsize * ysize * sizeof(float);

  if (pixels == NULL) {
    fprintf(stderr, "Unable to allocate %d bytes for input image\n", img_size);
    return (1);
  }

  if (read(fd, pixels, img_size) != img_size) {
    fprintf(stderr, "Unable to read %d bytes from input image\n", img_size);
    return (1);
  }

  close(fd);

  /* end successfully */
  return (0);
}


/******************************************************************************
Write an image to a binary file.

Entry:
  filename - name of file to write
******************************************************************************/

void FloatImage::write_file(char *filename)
{
  /* append ".flt" to the file name if necesary */

  char name[80];
  strcpy(name, filename);
  if (strlen(name) < 4 || strcmp(name + strlen(name) - 4, ".flt") != 0)
    strcat(name, ".flt");

  int fd = open(name, O_CREAT | O_TRUNC | O_WRONLY, 0666);
  if (fd < 0) {
    fprintf(stderr, "Unable to open %s: %s\n", name, strerror(errno));
    exit(-1);
  }

  char str[80];
  sprintf(str, "FL\n%d %d\n", xsize, ysize);
  int cc = write(fd, str, strlen(str));
  if (cc == -1) {
    fprintf(stderr, "Can't write to file.\n");
    exit(-1);
  }

  cc = write(fd, pixels, sizeof(float) * xsize * ysize);

  if (cc != sizeof(float) * xsize * ysize) {
    fprintf(stderr, "Write returned short: %s\n", strerror(errno));
    close(fd);
    exit(-1);
  }

  close(fd);
}

