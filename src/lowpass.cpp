/*

Low-pass versions of an image of streamlines.

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
#include "../libs/window.h"
#include "../libs/floatimage.h"
#include "vfield.h"
#include "streamline.h"
#include "stplace.h"
#include "lowpass.h"
#include "visparams.h"

#define Min(a, b) ((a) > (b) ? (b) : (a))
#define Max(a, b) ((a) > (b) ? (a) : (b))
#define Square(a) ((a)*(a))


const int samples = 30;
float filter_sum[samples][samples];
int filter_made = 0;


/******************************************************************************
Create a lowpass image.
******************************************************************************/

Lowpass::Lowpass(int x, int y, float rad, float targ)
{
  xsize = x;
  ysize = y;
  image = new FloatImage(x, y);
  image->setimage(0.0);
  target = targ;
  sum = xsize * ysize * target * target;

  rad_image = new FloatImage(x, y);
  for (int i = 0; i < x; i++)
    for (int j = 0; j < y; j++) {
      float xx = i / (float) x;
      float yy = j / (float) x;
      rad_image->pixel(i, j) = vis_get_blur_radius(xx, yy);
    }

  bundle = new Bundle();
}


/******************************************************************************
Compute a line passing through two points.

Entry:
  x1,y1 - first point to put line through
  x2,y2 - second point

Exit:
  a,b,c - equation of line
******************************************************************************/

void compute_line(
        float x1,
        float y1,
        float x2,
        float y2,
        float &a,
        float &b,
        float &c
)
{
  a = y2 - y1;
  b = x1 - x2;
  c = y1 * x2 - x1 * y2;

  float len = sqrt(a * a + b * b);

  if (len > 0) {
    a /= len;
    b /= len;
    c /= len;
  }
}


/******************************************************************************
Create the radially-symmetric filter for quick table lookup.
******************************************************************************/

void create_radial_filter()
{
  int i, j;

  float delta = 1.0 / (samples - 1);

  for (i = 0; i < samples; i++)
    for (j = 0; j < samples; j++)
      filter_sum[i][j] = 0;

  for (i = 0; i < samples; i++) {
    float r = i * delta;
    float sum = 0;
    for (j = 0; j < samples; j++) {
      float h = j * delta;
      float t = sqrt(r * r + h * h);
      float f = (2 * t - 3) * t * t + 1;
      if (t > 1)
        f = 0;
      filter_sum[i][j] = sum;
      sum += f * delta;
      /*
      printf ("%.3f ", filter_sum[i][j]);
      */
    }
    /*
    printf ("\n");
    */
  }

  filter_made = 1;
}


/******************************************************************************
Return the value along a summed line of the filter.

Entry:
  x,y - point along line

Exit:
  returns summed value
******************************************************************************/

float radial_value(float x, float y)
{
  x = fabs(x);
  y = fabs(y);

  int i = (int) floor((samples - 1) * x);
  int j = (int) floor((samples - 1) * y);

  if (i == samples - 1)
    i = samples - 2;

  if (j == samples - 1)
    j = samples - 2;

  float tx = (samples - 1) * x - i;
  float ty = (samples - 1) * y - j;

  float s00 = filter_sum[i][j];
  float s01 = filter_sum[i][j + 1];
  float s10 = filter_sum[i + 1][j];
  float s11 = filter_sum[i + 1][j + 1];

  float s0 = s00 + ty * (s01 - s00);
  float s1 = s10 + ty * (s11 - s10);
  float s = s0 + tx * (s1 - s0);

  return (s);
}


/******************************************************************************
Find the contribution of a line segment to a single pixel in a low-pass
filtered version of an image.  This version uses a rotationally symmetric
filter from a pre-built table.

Entry:
  i,j   - pixel coordinate
  x0,y0 - one segment endpoint
  x1,y1 - other endpoint
  a,b,c - equation of line through endpoints

Exit:
  returns contribution to the pixel
******************************************************************************/
float Lowpass::better_filter_segment(
        int i,
        int j,
        float x0,
        float y0,
        float x1,
        float y1,
        float a,
        float b,
        float c
)
{
#if 0
  printf ("\n");
  printf ("i j: %d %d\n", i, j);
  printf ("a b c: %f %f %f\n", a, b, c);
  printf ("old x0 y0 x1 y1: %f %f %f %f\n", x0, y0, x1, y1);
#endif

  /* translate line into coordinates so that pixel is at (0,0) */

  x0 -= i;
  y0 -= j;
  x1 -= i;
  y1 -= j;

  /* rotate line so that it points vertically */

  float xx0 = x0;
  float yy0 = y0;
  float xx1 = x1;
  float yy1 = y1;

  x0 = a * xx0 + b * yy0;
  y0 = -b * xx0 + a * yy0;
  x1 = a * xx1 + b * yy1;
  y1 = -b * xx1 + a * yy1;

  /* maybe flip the line across the y-axis */

  if (x0 < 0) {
    x0 *= -1.0;
    x1 *= -1.0;
  }

  /* scale segment by the radius of the filter */

  float recip = 1.0 / rad_image->pixel(i, j);
  x0 *= recip;
  y0 *= recip;
  x1 *= recip;
  y1 *= recip;

#if 0
  printf ("new x0 y0 x1 y1: %f %f %f %f\n", x0, y0, x1, y1);
#endif

  /* clip to maximum radius */

  if (x0 > 1.0)
    return (0.0);

  if (y0 < -1.0) y0 = -1.0;
  if (y0 > 1.0) y0 = 1.0;
  if (y1 < -1.0) y1 = -1.0;
  if (y1 > 1.0) y1 = 1.0;

#if 0
  printf ("clipped x0 y0 x1 y1: %f %f %f %f\n", x0, y0, x1, y1);
#endif

  /* determine filter contribution */

  if (y0 * y1 > 0) {  /* same side of x-axis */
    float diff = radial_value(x0, y0) - radial_value(x1, y1);
    return (fabs(diff));
  } else {              /* opposite sides of x-axis */
    float sum = radial_value(x0, y0) + radial_value(x1, y1);
    return (sum);
  }
}


/******************************************************************************
What is the new quality measure of a low-pass image, given a new
streamline?

Entry:
  st - new streamline

Exit:
  returns quality measure
******************************************************************************/

float Lowpass::new_quality(Streamline *st)
{
  float a, b, c;

  if (!filter_made) {
    create_radial_filter();
    /*
    for (int k = 0; k <= 300; k++) {
      float tt = k / 300.0;
      printf ("t radial_value: %f %f\n", tt, radial_value (1.0, tt));
    }
    */
  }

  /* clear out the list of value contributions to the pixels */
  st->clear_values();

  /* examine each line segment in a streamline to find their contribution */
  /* to a low-pass filtered version of the image */

  for (int n = 0; n < st->samples - 1; n++) {

    /* find coordinates of line segment ends */

    float x0 = st->xs(n);
    float y0 = st->ys(n);
    float x1 = st->xs(n + 1);
    float y1 = st->ys(n + 1);
    float taper_scale = 0.5 * (st->pts[n].intensity + st->pts[n + 1].intensity);

    float rad = rad_image->get_value((x0 + x1) * 0.5, (y0 + y1) * 0.5);

    x0 = x0 * xsize - 0.5;
    y0 = y0 * ysize / image->getaspect() - 0.5;
    x1 = x1 * xsize - 0.5;
    y1 = y1 * ysize / image->getaspect() - 0.5;

    /* find which pixels the segment can affect */

    int i0 = (int) Min (floor(x0 - rad), floor(x1 - rad));
    int i1 = (int) Max (ceil(x0 + rad), ceil(x1 + rad));
    int j0 = (int) Min (floor(y0 - rad), floor(y1 - rad));
    int j1 = (int) Max (ceil(y0 + rad), ceil(y1 + rad));

    /* clamp these values to the image size */
    i0 = Max (i0, 0);
    j0 = Max (j0, 0);
    i1 = Min (i1, xsize - 1);
    j1 = Min (j1, ysize - 1);

#if 0
    printf ("x0 y0 x1 y1: %f %f %f %f\n", x0, y0, x1, y1);
    printf ("i0 j0 i1 j1: %d %d %d %d\n", i0, j0, i1, j1);
    printf ("\n");
#endif

    /* compute equation for line through the segment */

    compute_line(x0, y0, x1, y1, a, b, c);

    /* loop over the possibly affected pixels */

    for (int i = i0; i <= i1; i++) {
      for (int j = j0; j <= j1; j++) {
        float value = better_filter_segment(i, j, x0, y0, x1, y1, a, b, c);
        value *= taper_scale;
#if 0
        printf ("i j value: %d %d %f\n", i, j, value);
#endif
        if (value > 0)
          st->add_value(i, j, value);
      }
    }
  }

  /* compute what the average pixel value would be if we added the streamline */

  float new_sum = sum;

  for (int n = 0; n < st->num_values; n++) {
    int i, j;
    float value;
    st->get_value(n, i, j, value);
    new_sum -= Square (target - image->pixel(i, j));
    image->pixel(i, j) += value;
    new_sum += Square (target - image->pixel(i, j));
  }

  /* take out what we added in */
  for (int n = 0; n < st->num_values; n++) {
    int i, j;
    float value;
    st->get_value(n, i, j, value);
    image->pixel(i, j) -= value;
  }

#if 0
  printf ("sum new_sum: %f %f\n", sum, new_sum);
#endif

  return (new_sum);
}


/******************************************************************************
Filter a line segment, placing the result in an image.

Entry:
  x0,y0 - one endpoint of line segment
  x1,y1 - other endpoint
  scale - scale factor for result
******************************************************************************/

void Lowpass::filter_segment(
        float x0,
        float y0,
        float x1,
        float y1,
        float scale
)
{
  /* create the radial filter if it hasn't already been made */
  if (!filter_made)
    create_radial_filter();

  /* get the filter radius at the segment's center */
  float rad = rad_image->get_value((x0 + x1) * 0.5, (y0 + y1) * 0.5);

  /* find coordinates of line segment ends */
  x0 = x0 * xsize - 0.5;
  y0 = y0 * ysize / image->getaspect() - 0.5;
  x1 = x1 * xsize - 0.5;
  y1 = y1 * ysize / image->getaspect() - 0.5;

  /* find which pixels the segment can affect */

  int i0 = (int) Min (floor(x0 - rad), floor(x1 - rad));
  int i1 = (int) Max (ceil(x0 + rad), ceil(x1 + rad));
  int j0 = (int) Min (floor(y0 - rad), floor(y1 - rad));
  int j1 = (int) Max (ceil(y0 + rad), ceil(y1 + rad));

  /* clamp these values to the image size */
  i0 = Max (i0, 0);
  j0 = Max (j0, 0);
  i1 = Min (i1, xsize - 1);
  j1 = Min (j1, ysize - 1);

  /* compute equation for line through the segment */
  float a, b, c;
  compute_line(x0, y0, x1, y1, a, b, c);

  /* loop over the possibly affected pixels, adding the segment's */
  /* contribution to the image */

  for (int i = i0; i <= i1; i++)
    for (int j = j0; j <= j1; j++) {
      float value = better_filter_segment(i, j, x0, y0, x1, y1, a, b, c);
      image->pixel(i, j) += value * scale;
    }
}


/******************************************************************************
Add a streamline to an image.
******************************************************************************/

void Lowpass::add_line(Streamline *st)
{
  int i, j;
  float value;

  /* add these values to the image */

  for (int n = 0; n < st->num_values; n++) {
    st->get_value(n, i, j, value);
    sum -= Square (target - image->pixel(i, j));
    image->pixel(i, j) += value;
    sum += Square (target - image->pixel(i, j));
  }

  /* add new streamline to bundle */
  bundle->add_line(st);
}


/******************************************************************************
Remove a streamline from an image.
******************************************************************************/

void Lowpass::delete_line(Streamline *st)
{
  int i, j;
  float value;

  /* subtract these values from the image */

  for (int n = 0; n < st->num_values; n++) {
    st->get_value(n, i, j, value);
    sum -= Square (target - image->pixel(i, j));
    image->pixel(i, j) -= value;
    sum += Square (target - image->pixel(i, j));
  }

  /* remove streamline from bundle */
  bundle->delete_line(st);
}


/******************************************************************************
Draw all streamlines.
******************************************************************************/

void Lowpass::draw_lines(Window2d *win)
{
  for (int i = 0; i < bundle->num_lines; i++)
    bundle->get_line(i)->draw(win);
}


/******************************************************************************
Calculate the image quality from scratch.
******************************************************************************/

float Lowpass::recalculate_quality()
{
  int i, j;
  float tsum = 0;

  /* re-create image */

  image->setimage(0.0);

  for (int num = 0; num < bundle->num_lines; num++) {
    Streamline *st = bundle->get_line(num);
    for (int n = 0; n < st->num_values; n++) {
      float value;
      st->get_value(n, i, j, value);
      image->pixel(i, j) += value;
    }
  }

  /* calculate sum */

  for (i = 0; i < xsize; i++)
    for (j = 0; j < ysize; j++)
      tsum += Square (target - image->pixel(i, j));

  sum = tsum;

  return (tsum);
}


/******************************************************************************
Is a particular pixel below the threshold at which we want a birth?

Entry:
  x,y          - index of pixel to check
  birth_thresh - threshold against which to check

Exit:
  whether we're below the threshold
******************************************************************************/

int Lowpass::birth_test(float x, float y, float birth_thresh)
{
  return (image->get_value(x, y) < birth_thresh);
}


/******************************************************************************
Delete all streamlines of this lowpass image.
******************************************************************************/

void Lowpass::delete_streamlines()
{
  for (int i = 0; i < bundle->num_lines; i++) {
    Streamline *st = bundle->get_line(i);
    delete st;
  }
}


/******************************************************************************
Set the value of the filter radius in a gradient from left to right.

Entry:
  r1 - radius at left of image
  r2 - radius at right of image
******************************************************************************/

void Lowpass::set_radius(float r1, float r2)
{
  for (int i = 0; i < xsize; i++) {
    float t = i / (float) (xsize - 1);
    float r = r1 + t * (r2 - r1);
    for (int j = 0; j < ysize; j++) {
      rad_image->pixel(i, j) = r;
    }
  }
}


/******************************************************************************
Determine the quality of a steamline by sampling it semi-randomly.

Entry:
  st       - streamline we want the quality of
  radius   - radius of nearby random samples
  nsamples - number of samples
  dist     - another distance factor
  win      - window to plot samples in, for debugging

Exit:
  returns quality
******************************************************************************/

float Lowpass::random_samples_quality(
        Streamline *st,
        float radius,
        int nsamples,
        float dist,
        Window2d *win
)
{
  int i;
  int num;
  int index;
  float x, y;
  float rad;

  float sum = 0;

  float xmin = 2.0 / xsize;
  float ymin = 2.0 / ysize;
  float xmax = 1 - 2.0 / xsize;
  float ymax = image->getaspect() - 2.0 / ysize;

  /* kluge to work around problems at the image borders */

  int inside_points = 0;
  for (i = 0; i < st->samples; i++) {
    x = st->pts[i].x;
    y = st->pts[i].y;
    if (x > xmin && x < xmax && y > ymin && y < ymax) {
      inside_points = 1;
      break;
    }
  }

  if (inside_points == 0)
    return (0);

//  num = 2 * nsamples / 3;

  num = nsamples;

  for (i = 0; i < num; i++) {

    /* pick a random sample near the streamline in the lowpass image */

    do {

      float pick = drand48();

      if (pick < 0.3333) {
        index = (int) floor(drand48() * st->samples);
        rad = radius;
      } else if (pick < 0.6666) {
        index = 0;
        rad = 2 * radius;
      } else {
        index = st->samples - 1;
        rad = 2 * radius;
      }

      SamplePoint *sample = &st->pts[index];
      x = sample->x + rad * (2 * drand48() - 1);
      y = sample->y + rad * (2 * drand48() - 1);

    } while (x < xmin || x > xmax || y < ymin || y > ymax);

    /* debug drawing of samples */

    if (win) {
      win->set_color_index(255);
      win->line(x, y, x, y);
    }

    sum += Square (target - image->get_value(x, y));
  }

#if 0
  num = 2 * nsamples / 3;

  for (i = 0; i < num; i++) {

    /* pick one end of streamline or the other */

    if (i < num / 2)
      index = st->samples - 1;
    else
      index = 0;

    /* pick a random sample near the streamline in the lowpass image */

    SamplePoint *sample = &st->pts[index];

    do {
      x = sample->x + 2 * radius * (2 * drand48() - 1);
      y = sample->y + 2 * radius * (2 * drand48() - 1);
    } while (x < 0 || x > 1 || y < 0 || y > image->getaspect());

    /* debug drawing of samples */

    if (win) {
      win->set_color_index (255);
      win->line (x, y, x, y);
    }

    sum += Square (target - image->get_value(x,y));
  }
#endif

  return (sum);
}


/******************************************************************************
Determine how shortening the streamline may affect the quality.

Entry:
  st       - streamline we want the quality of
  radius   - radius of nearby samples
  nsamples - number of samples
  dist     - another distance factor
  win      - window to plot samples in, for debugging

Exit:
  which_end - shorten which end?
  returns quality
******************************************************************************/

float Lowpass::shorten_quality(
        Streamline *st,
        float radius,
        int nsamples,
        float dist,
        Window2d *win,
        int &which_end
)
{
  int i;
  float rad;
  float delta;
  float x, y;
  float sum1 = 0;
  float sum2 = 0;
  int count1 = 0;
  int count2 = 0;
  VectorField *vf = st->vf;

  /* kluge to work around problems at the image borders */

  float xmin = 2.0 / xsize;
  float ymin = 2.0 / ysize;
  float xmax = 1 - 2.0 / xsize;
  float ymax = image->getaspect() - 2.0 / ysize;

  /* measure the quality as we move into the body of the streamline */
  /* from the tail endpoint */

  x = st->pts[0].x;
  y = st->pts[0].y;
  rad = radius * rad_image->get_value(x, y) / xsize;
  delta = dist * rad / nsamples;

  for (i = 0; i < nsamples; i++) {

    /* don't measure near the borders */
    if (x < xmin || x > xmax || y < ymin || y > ymax)
      continue;

    /* measure the point's quality */
    float value = image->get_value(x, y);
    if (value > target)
      sum1 += Square (target - value);
    count1++;

    /* debug drawing of samples */
    if (win) {
      win->set_color_index(255);
      win->line(x, y, x, y);
    }

    /* move further away from the endpoint */
    x += delta * vf->xval(x, y);
    y += delta * vf->yval(x, y);
  }

  /* now measure from the head */

  x = st->pts[st->samples - 1].x;
  y = st->pts[st->samples - 1].y;
  rad = radius * rad_image->get_value(x, y) / xsize;
  delta = dist * rad / nsamples;

  for (i = 0; i < nsamples; i++) {

    /* don't measure near the borders */
    if (x < xmin || x > xmax || y < ymin || y > ymax)
      continue;

    /* measure the point's quality */
    float value = image->get_value(x, y);
    if (value > target)
      sum2 += Square (target - value);
    count2++;

    /* debug drawing of samples */
    if (win) {
      win->set_color_index(255);
      win->line(x, y, x, y);
    }

    /* move further away from the endpoint */
    x -= delta * vf->xval(x, y);
    y -= delta * vf->yval(x, y);
  }

  /* determine quality */

  float qual = 0;

  if (count1 + count2 != 0)
    qual = (sum1 + sum2) / (count1 + count2);

  /* say which end needs change more */

  if (count1)
    sum1 /= count1;
  if (count2)
    sum2 /= count2;

  if (sum1 > sum2)
    which_end = TAIL;
  else
    which_end = HEAD;

  return (qual);
}


/******************************************************************************
Determine how lengthening the streamline may affect the quality.

Entry:
  st       - streamline we want the quality of
  radius   - radius of nearby samples
  nsamples - number of samples
  dist     - another distance factor
  win      - window to plot samples in, for debugging

Exit:
  which_end - lengthen which end?
  returns quality
******************************************************************************/

float Lowpass::lengthen_quality(
        Streamline *st,
        float radius,
        int nsamples,
        float dist,
        Window2d *win,
        int &which_end
)
{
  int i;
  float rad;
  float delta;
  float x, y;
  float sum1 = 0;
  float sum2 = 0;
  int count1 = 0;
  int count2 = 0;
  VectorField *vf = st->vf;

  /* kluge to work around problems at the image borders */

  float xmin = 2.0 / xsize;
  float ymin = 2.0 / ysize;
  float xmax = 1 - 2.0 / xsize;
  float ymax = image->getaspect() - 2.0 / ysize;

  /* measure the quality as we move away from the streamline's tail */

  x = st->pts[0].x;
  y = st->pts[0].y;
  rad = radius * rad_image->get_value(x, y) / xsize;
  delta = dist * rad / nsamples;

  for (i = 0; i < nsamples; i++) {

    /* don't go too near the borders */
    if (x < xmin || x > xmax || y < ymin || y > ymax)
      break;

    /* measure the point's quality */
    float value = image->get_value(x, y);
    if (value < target)
      sum1 += Square (target - value);
    count1++;

    /* debug drawing of samples */
    if (win) {
      win->set_color_index(255);
      win->line(x, y, x, y);
    }

    /* move further away from the endpoint */
    x -= delta * vf->xval(x, y);
    y -= delta * vf->yval(x, y);
  }

  /* now measure from the head */

  x = st->pts[st->samples - 1].x;
  y = st->pts[st->samples - 1].y;
  rad = radius * rad_image->get_value(x, y) / xsize;
  delta = dist * rad / nsamples;

  for (i = 0; i < nsamples; i++) {

    /* don't go too near the borders */
    if (x < xmin || x > xmax || y < ymin || y > ymax)
      break;

    /* measure the point's quality */
    float value = image->get_value(x, y);
    if (value < target)
      sum2 += Square (target - value);
    count2++;

    /* debug drawing of samples */
    if (win) {
      win->set_color_index(255);
      win->line(x, y, x, y);
    }

    /* move further away from the endpoint */
    x += delta * vf->xval(x, y);
    y += delta * vf->yval(x, y);
  }

  /* determine quality */

  float qual = 0;

  if (count1 + count2 != 0)
    qual = (sum1 + sum2) / (count1 + count2);

  /* say which end needs change more */

  if (count1)
    sum1 /= count1;
  if (count2)
    sum2 /= count2;

  if (sum1 > sum2)
    which_end = TAIL;
  else
    which_end = HEAD;

  return (qual);
}


/******************************************************************************
Determine how much lateral motion a streamline needs by sampling it along
its length.

Entry:
  st       - streamline we want the quality of
  radius   - radius of nearby random samples
  nsamples - number of samples
  dist     - another distance factor
  win      - window to plot samples in, for debugging

Exit:
  which_side - which side to move towards
  returns quality
******************************************************************************/

float Lowpass::main_body_quality(
        Streamline *st,
        float radius,
        int nsamples,
        float dist,
        Window2d *win,
        int &which_side
)
{
  int i;
  float rad;
  float delta;
  float x, y;
  float sum = 0;
  int count = 0;
  VectorField *vf = st->vf;

  /* kluge to work around problems at the image borders */

  float xmin = 2.0 / xsize;
  float ymin = 2.0 / ysize;
  float xmax = 1 - 2.0 / xsize;
  float ymax = image->getaspect() - 2.0 / ysize;

  /* measure the quality at several random positions along the streamline */

  for (i = 0; i < nsamples; i++) {

    int index = (int) (st->samples * drand48());
    SamplePoint *sample = &st->pts[index];

    x = sample->x;
    y = sample->y;

    /* don't measure near the borders */
    if (x < xmin || x > xmax || y < ymin || y > ymax)
      continue;

    rad = radius * rad_image->get_value(x, y) / xsize;
    delta = 0.3 * rad / nsamples;

    /* find out how to move perpendicular to the vector field */
    float dx = -delta * vf->yval(x, y);
    float dy = delta * vf->xval(x, y);

    /* sample along either side of the streamline */
    float x1 = x + dx;
    float y1 = y + dy;
    float x2 = x - dx;
    float y2 = y - dy;

    /* measure the quality at the two points */
    float value1 = image->get_value(x1, y1);
    float value2 = image->get_value(x2, y2);
#if 0
    float diff = value1 - value2;
    sum += copysign (Square(diff), diff);
#endif
#if 1
    sum += value1 - value2;
#endif
    count++;

    /* debug drawing of samples */
    if (win) {
      win->set_color_index(255);
      win->line(x1, y1, x1, y1);
      win->line(x2, y2, x2, y2);
    }
  }

  /* return quality */

  if (count == 0)
    return (0.0);
  else {

    if (sum > 0)
      which_side = LEFT;
    else
      which_side = RIGHT;

    return (fabs(sum / count));
  }
}


/******************************************************************************
Determine the quality of a steamline by sampling it semi-randomly.

Entry:
  st       - streamline we want the quality of
  radius   - radius of nearby random samples
  nsamples - number of samples
  dist     - another distance factor
  win      - window to plot samples in, for debugging

Exit:
  returns quality
******************************************************************************/

void Lowpass::streamline_quality(
        Streamline *st,
        float radius,
        int nsamples,
        float dist,
        Window2d *win
)
{

  /* a frozen streamline shouldn't want to move at all */
  if (st->frozen) {
    st->quality = 0;
    return;
  }

  int lengthen_end;
  int shorten_end;
  int move_side;
  float lq, sq, mq;

  float x, y;
  st->get_origin(x, y);
  int length_frozen = (vis_get_delta_length(x, y) == 0);

  if (length_frozen) {
    lq = sq = 0;
  } else {
    lq = lengthen_quality(st, radius, nsamples, dist, win, lengthen_end);
    sq = shorten_quality(st, radius, nsamples, dist, win, shorten_end);
  }

  mq = main_body_quality(st, radius, nsamples, dist, win, move_side);
  mq *= 4;

  st->quality = lq + sq + mq;

  /* decide what change to make based on the above qualities */

  unsigned long int change = 0;

  if (lq > sq && lq > mq && !length_frozen) {  /* lengthen */
    change |= LEN_CHANGE;
    change |= LONG_ONE;

    if (lengthen_end == HEAD)
      change |= HEAD_CHANGE;
    else if (lengthen_end == TAIL)
      change |= TAIL_CHANGE;
  } else if (sq > lq && sq > mq && !length_frozen) {  /* shorten */
    change |= LEN_CHANGE;
    change |= SHORT_ONE;

    if (shorten_end == HEAD)
      change |= HEAD_CHANGE;
    else if (shorten_end == TAIL)
      change |= TAIL_CHANGE;
  } else {                          /* move */
    change |= MOVE_CHANGE;

#if 0
    if (drand48() > 0.5) {
      if (move_side == RIGHT)
        change |= RIGHT_CHANGE;
      else if (move_side == LEFT)
        change |= LEFT_CHANGE;
      }
#endif
  }

  st->change = change;
}


/******************************************************************************
Recommend how a streamline should be changed, based on the quality measures.
******************************************************************************/

unsigned long int Lowpass::recommend_change(Streamline *st)
{
  unsigned long int change = st->change;

  /* maybe make a totally random change */

  if (0.2 > drand48()) {
    change = 0;
    change |= MOVE_CHANGE;
    change |= LEN_CHANGE;
    change |= ALL_LEN;
  }

  /* maybe pick randomly which end gets shortened/lengthened */
  if (drand48() < 0.5) {
    change &= ~HEAD_CHANGE;
    change &= ~TAIL_CHANGE;
  }

  if (verbose_flag)
    if ((change & MOVE_CHANGE) && (change & LEN_CHANGE)) {
      printf("R");
      fflush(stdout);
    } else if (change & LONG_ONE) {
      printf("L");
      fflush(stdout);
    } else if (change & SHORT_ONE) {
      printf("S");
      fflush(stdout);
    } else if (change & MOVE_CHANGE) {
      printf("M");
      fflush(stdout);
    }

  return (change);
}

