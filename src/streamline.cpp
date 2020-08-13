/*

Streamlines.

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
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "../libs/window.h"
#include "../libs/floatimage.h"
#include "vfield.h"
#include "streamline.h"
#include "lowpass.h"
#include "../libs/clip_line.h"
#include "visparams.h"
#include "stplace.h"

/* current default properties of streamlines */

static int start_reduction_default = 0;
static int end_reduction_default = 0;
static int label_default = 0;
static int arrow_type_default = NO_ARROW;
static float arrow_length_default = 0;
static float arrow_width_default = 0;
static int arrow_steps_default = 0;
static float intensity_default = 1;
static float taper_head_default = 0;
static float taper_tail_default = 0;
static float thickness_default = 1;


static float grow_factor = 0;
void set_grow_factor(float factor)
{
  grow_factor = factor;
}


/******************************************************************************
Copy a streamline.
******************************************************************************/

Streamline* Streamline::copy() {

  set_taper (taper_head, taper_tail);
  Streamline *st = new Streamline (vf, xorig, yorig, length1, length2, delta);

  st->frozen = frozen;
  st->label = label;
  st->start_reduction = start_reduction;
  st->end_reduction = end_reduction;
  st->arrow_type = arrow_type;
  st->arrow_length = arrow_length;
  st->arrow_width = arrow_width;
  st->arrow_steps = arrow_steps;
  st->intensity = intensity;
  st->taper_head = taper_head;
  st->taper_tail = taper_tail;

  return (st);
}


/******************************************************************************
Create a streamline.

Entry:
  field     - vector field in which streamline lives
  xx,yy     - starting point for streamline
  len1,len2 - lengths of streamline on either side of origin
  dlen      - step value along the streamline
******************************************************************************/

void Streamline::streamline_creator(
  VectorField *field,
  float xx,
  float yy,
  float len1,
  float len2,
  float dlen
)
{
  int i;
  float x,y;

  /* integrate to midpoint */

#if 1
  float dl = 0.5 * (len1 - len2);
  int num = abs ((int) (dl / dlen)) + 1;
  float dt = dl / num;
  x = xx;
  y = yy;
  for (i = 0; i < num; i++) {
    x += dt * field->xval(x,y);
    y += dt * field->yval(x,y);
  }
  xx = x;
  yy = y;
  float new_len = 0.5 * (len1 + len2);
  len1 = len2 = new_len;
#endif

  /* origin must be within [0,1] x [0,aspect] */

  if (xx < 0 || xx > 1 || yy < 0 || yy > field->getaspect()) {
//    printf ("streamline origin: %f %f\n", xx, yy);
    float x = xx;
    float y = yy;
    clamp_to_screen (x, y, field->getaspect());
    xx = x;
    yy = y;
  }

  /* set values */

  vf = field;
  xorig = xx;
  yorig = yy;
  length1 = len1;
  length2 = len2;
  delta = dlen;
  frozen = 0;      /* a "frozen" streamline is one that isn't to be moved */
  label = label_default;
  start_reduction = start_reduction_default;
  end_reduction = end_reduction_default;
  arrow_type = arrow_type_default;
  arrow_length = arrow_length_default;
  arrow_width = arrow_width_default;
  arrow_steps = arrow_steps_default;
  intensity = intensity_default;
  taper_head = taper_head_default;
  taper_tail = taper_tail_default;
  tail_clipped = 0;
  head_clipped = 0;

  /* values at pixels */
  num_values = 0;
  max_values = 10;
  values = new PixelValue[max_values];

  /* determine spacing of sample points */

  int samples1 = (int) (length1 / delta);
  int samples2 = (int) (length2 / delta);
  delta = (length1+length2) / (samples1+samples2);

  samples = samples1 + samples2 + 1;
  pts = new SamplePoint[samples];

  /* calculate sample points along streamline */

  x = xorig;
  y = yorig;

  xs(samples2) = x;
  ys(samples2) = y;

#define MIN_STEP 0.2

  /* step forward */

  int count1 = 0;
  for (i = 0; i < samples1; i++) {

    float slen = vf->integrate (x, y, delta, 0, x, y);
    if (slen < MIN_STEP)
      break;

    xs(samples2 + i + 1) = x;
    ys(samples2 + i + 1) = y;
    count1++;

    /* clip to screen */
    if (x < 0 || x > 1 || y < 0 || y > vf->getaspect()) {
      set_clip_window (0.0, 1.0, 0.0, vf->getaspect());
      float x0 = xs(samples2 + i);
      float y0 = ys(samples2 + i);
// printf ("clip #1 before: %f %f %f %f\n", x0, y0, x, y);
      clip_line (x0, y0, x, y);
// printf ("clip #1  after: %f %f %f %f\n", x0, y0, x, y);
// printf ("\n");
      xs(samples2 + i + 1) = x;
      ys(samples2 + i + 1) = y;
      head_clipped = 1;
      break;  /* we're off the screen, so don't extend streamline further */
    }
  }

  /* step backwards */

  x = xorig;
  y = yorig;

  int count2 = 0;
  for (i = samples2-1; i >= 0; i--) {

    float slen = vf->integrate (x, y, -delta, 0, x, y);
    if (slen < MIN_STEP)
      break;

    xs(i) = x;
    ys(i) = y;
    count2++;

    /* clip to screen */
    if (x < 0 || x > 1 || y < 0 || y > vf->getaspect()) {
      set_clip_window (0.0, 1.0, 0.0, vf->getaspect());
      float x0 = xs(i+1);
      float y0 = ys(i+1);
// printf ("clip #2 before: %f %f %f %f\n", x0, y0, x, y);
      clip_line (x0, y0, x, y);
// printf ("clip #2  after: %f %f %f %f\n", x0, y0, x, y);
// printf ("\n");
      xs(i) = x;
      ys(i) = y;
      tail_clipped = 1;
      break;  /* we're off the screen, so don't extend streamline further */
    }
  }

  /* compute number of samples, taking into account the portions of */
  /* the streamline that may have been clipped */

  samples = count1 + count2 + 1;

  /* may have to shift the sample points */

  int diff2 = samples2 - count2;
  if (diff2 > 0)
    for (i = 0; i < samples; i++) {
      xs(i) = xs(i + diff2);
      ys(i) = ys(i + diff2);
    }

  int smp = samples - 1;
  if (xs(0) < 0 || xs(0) > 1 || ys(0) < 0 || ys(0) > vf->getaspect() ||
    xs(smp) < 0 || xs(smp) > 1 || ys(smp) < 0 || ys(smp) > vf->getaspect()) {
    printf ("origin: %f %f\n", xx, yy);
    printf ("ends:   %f %f %f %f\n", xs(0), ys(0), xs(smp), ys(smp));
    printf ("\n");
  }

  /* compute intensity tapering */

  for (i = 0; i < samples; i++) {
    float t = i / (samples - 1.0);
    if (t < taper_tail) {
      t = t / taper_tail;
      pts[i].intensity = t;
    }
    else if (1 - t < taper_head) {
      t = (1 - t) / taper_head;
      pts[i].intensity = t;
    }
    else
      pts[i].intensity = 1.0;
  }
}


/******************************************************************************
Create a streamline.

Entry:
  field - vector field in which streamline lives
  xx,yy - starting point for streamline
  len   - length of streamline
  dlen  - step value along the streamline
******************************************************************************/

Streamline::Streamline(
  VectorField *field,
  float xx,
  float yy,
  float len,
  float dlen
)
{
  streamline_creator (field, xx, yy, len * 0.5, len * 0.5, dlen);
}


/******************************************************************************
Create a streamline.

Entry:
  field     - vector field in which streamline lives
  xx,yy     - starting point for streamline
  len1,len2 - lengths of streamline on either side of origin
  dlen      - step value along the streamline
******************************************************************************/

Streamline::Streamline(
  VectorField *field,
  float xx,
  float yy,
  float len1,
  float len2,
  float dlen
)
{
  streamline_creator (field, xx, yy, len1, len2, dlen);
}


/******************************************************************************
Set default value for a streamline's label.
******************************************************************************/

void set_label(int label)
{
  label_default = label;
}


/******************************************************************************
Set default value for a streamline's arrow type (if any).

Entry:
  type   - NO_ARROW, OPEN_ARROW, SOLID_ARROW, etc.
  length - how long an arrowhead
  width  - how wide an arrowhead
  steps  - how many steps along the streamline to take while drawing arrow
******************************************************************************/

void set_arrow(int type, float length, float width, int steps)
{
  arrow_type_default = type;
  arrow_length_default = length;
  arrow_width_default = width;
  arrow_steps_default = steps;
}


/******************************************************************************
Set default values for reducing the length of a streamline when drawn.
******************************************************************************/

void set_reduction(int start, int end)
{
  start_reduction_default = start;
  end_reduction_default = end;
}


/******************************************************************************
Set default value for streamline intensity.
******************************************************************************/

void set_intensity(float val)
{
  intensity_default = val;
}


/******************************************************************************
Set values for tapering of streamline intensity at ends.
******************************************************************************/

void set_taper(float head, float tail)
{
  taper_head_default = head;
  taper_tail_default = tail;
}


/******************************************************************************
Set value for line thickness.
******************************************************************************/

void set_line_thickness(float thick)
{
  thickness_default = thick;
}


/******************************************************************************
Determine (tapered) intensity based on length.
******************************************************************************/

float Streamline::len_to_intensity(float len)
{
  if (len <= tail_zero || len >= head_zero)
    return (0.0);
  else if (len < taper_tail)
    return ((len - tail_zero) / (taper_tail - tail_zero));
  else if (len > taper_head)
    return ((len - taper_head) / (head_zero - taper_head));
  else
    return (1.0);
}

#if 1

/******************************************************************************
Draw a streamline with intensity tapering.
******************************************************************************/

void Streamline::draw_for_taper(Window2d *win)
{
  int i;

  /* draw line segments */

  float x_old = xs(0);
  float y_old = ys(0);

  for (i = 1; i < samples; i++) {

    float x = xs(i);
    float y = ys(i);

//    float val = len_to_intensity (pts[i].len_pos);
    float val = 1.0;
    win->set_color_index ((int) (255 * val));
    win->line (x, y, x_old, y_old);

    x_old = x;
    y_old = y;
  }
}

#endif

#if 0

/******************************************************************************
Draw a streamline with intensity tapering.
******************************************************************************/

void Streamline::draw_tapered(Window2d *win)
{
  int i;

  /* draw line segments */

  float x_old = xs(start_reduction);
  float y_old = ys(start_reduction);

  for (i = start_reduction + 1; i < samples - end_reduction; i++) {

    float x = xs(i);
    float y = ys(i);

    win->set_color_index ((int) (255 * pts[i].intensity));
    win->line (x, y, x_old, y_old);

    x_old = x;
    y_old = y;
  }
}

#endif

/******************************************************************************
Erase a streamline.
******************************************************************************/

void Streamline::erase(Window2d *win)
{
  win->set_color_index (0);
  draw_helper(win, 0);
}


/******************************************************************************
Draw a streamline in a given window.
******************************************************************************/

void Streamline::draw(Window2d *win)
{
  draw_helper(win, 1);
}


/******************************************************************************
Draw a streamline in a given window.

Entry:
  win               - window to draw into
  color_change_flag - can we change the color?
******************************************************************************/

void Streamline::draw_helper(Window2d *win, int color_change_flag)
{
  int i;

  /* draw line segments */

  float x_old = xs(start_reduction);
  float y_old = ys(start_reduction);

  for (i = start_reduction + 1; i < samples - end_reduction; i++) {
    float x = xs(i);
    float y = ys(i);
    if (color_change_flag)
      win->set_color_index ((int) (255 * intensity * pts[i].intensity));
    win->thick_line (x, y, x_old, y_old, (int) thickness_default);
    x_old = x;
    y_old = y;
  }

// win->thick_line (xorig - 0.01, yorig, xorig + 0.01, yorig, 3);
// win->thick_line (xorig, yorig - 0.01, xorig, yorig + 0.01, 3);

  /* maybe draw an open arrow */

  if (color_change_flag)
    win->set_color_index ((int) (255 * intensity));

  float x = xs(samples - end_reduction - 1);
  float y = ys(samples - end_reduction - 1);

  if (arrow_type == OPEN_ARROW)
    draw_arrow(win, x, y, arrow_width, arrow_length, (int)thickness_default, 1);
  else
    draw_arrow(win, x, y, arrow_width, arrow_length, (int)thickness_default, 0);

  win->flush();
}


#if 0

/* old version, just outline */

/******************************************************************************
Draw a fancy arrow.

Entry:
  win         - window to draw into
  xorig,yorig - position to begin arrow
  arrow_len   - arrow length
  head_length - arrowhead length
  head_width  - arrowhead width
  lip_ratio   - how big a lip to make between arrowhead and the arrow's body
******************************************************************************/

void Streamline::draw_fancy_arrow(
  Window2d *win,
  float xorig,
  float yorig,
  float arrow_len,
  float head_length,
  float head_width,
  float lip_ratio
)
{
  float x,y;
  float dx,dy;
  float xx,yy;
  float len;
  float dlen;
  int steps;

  float dt = 1.0 / win->xsize;   /* delta length for stepping along lines */

  /* find the base of the arrowhead */

  steps = (int) floor (head_length / dt);
  dlen = head_length / steps;

  x = xorig;
  y = yorig;

  for (int i = 0; i < steps; i++) {
    dx = vf->xval(x,y);
    dy = vf->yval(x,y);
    len = sqrt (dx*dx + dy*dy);
    if (len != 0.0) {
      dx /= len;
      dy /= len;
    }
    x += dx * dlen;
    y += dy * dlen;
  }

  float x1 = x + dy * head_width;
  float y1 = y - dx * head_width;
  float x2 = x - dy * head_width;
  float y2 = y + dx * head_width;

  float x3 = x + dy * lip_ratio * head_width;
  float y3 = y - dx * lip_ratio * head_width;
  float x4 = x - dy * lip_ratio * head_width;
  float y4 = y + dx * lip_ratio * head_width;

  win->line (xorig, yorig, x1, y1);
  win->line (xorig, yorig, x2, y2);
  win->line (x3, y3, x1, y1);
  win->line (x4, y4, x2, y2);

  /* now step along the arrow's body */

  steps = (int) floor ((arrow_len - head_length) / dt);
  dlen = (arrow_len - head_length) / steps;

  for (i = 0; i < steps; i++) {

    dx = vf->xval(x,y);
    dy = vf->yval(x,y);
    len = sqrt (dx*dx + dy*dy);
    if (len != 0.0) {
      dx /= len;
      dy /= len;
    }
    x += dx * dlen;
    y += dy * dlen;

    float t = (steps - i + 1) / (float) steps;
    float width = t * head_width * lip_ratio;
    x1 = x + dy * width;
    y1 = y - dx * width;
    x2 = x - dy * width;
    y2 = y + dx * width;

    win->line (x1, y1, x3, y3);
    win->line (x2, y2, x4, y4);

    x3 = x1;
    y3 = y1;
    x4 = x2;
    y4 = y2;
  }
}

#endif


/******************************************************************************
Draw a fancy arrow.

Entry:
  win         - window to draw into
  xorig,yorig - position to begin arrow
  arrow_len   - arrow length
  head_length - arrowhead length
  head_width  - arrowhead width
  lip_ratio   - how big a lip to make between arrowhead and the arrow's body
******************************************************************************/

void Streamline::draw_fancy_arrow(
  Window2d *win,
  float xorig,
  float yorig,
  float arrow_len,
  float head_length,
  float head_width,
  float lip_ratio
)
{
  float x,y;
  float dx,dy;
  float len;
  float dlen;
  int steps;

  float dt = 1.0 / win->xsize;   /* delta length for stepping along lines */

  /* find the base of the arrowhead */

  steps = (int) floor (head_length / dt);
  dlen = head_length / steps;

  x = xorig;
  y = yorig;

  for (int i = 0; i < steps; i++) {
    dx = vf->xval(x,y);
    dy = vf->yval(x,y);
    len = sqrt (dx*dx + dy*dy);
    if (len != 0.0) {
      dx /= len;
      dy /= len;
    }
    x += dx * dlen;
    y += dy * dlen;
  }

  float x1 = x + dy * head_width;
  float y1 = y - dx * head_width;
  float x2 = x - dy * head_width;
  float y2 = y + dx * head_width;

  float x3 = x + dy * lip_ratio * head_width;
  float y3 = y - dx * lip_ratio * head_width;
  float x4 = x - dy * lip_ratio * head_width;
  float y4 = y + dx * lip_ratio * head_width;

  /* now step along the arrow's body */

  steps = (int) floor ((arrow_len - head_length) / dt);
  dlen = (arrow_len - head_length) / steps;

  float *xverts1 = new float[steps];
  float *yverts1 = new float[steps];
  float *xverts2 = new float[steps];
  float *yverts2 = new float[steps];

  for (int i = 0; i < steps; i++) {

    dx = vf->xval(x,y);
    dy = vf->yval(x,y);
    len = sqrt (dx*dx + dy*dy);
    if (len != 0.0) {
      dx /= len;
      dy /= len;
    }
    x += dx * dlen;
    y += dy * dlen;

    float t = (steps - i + 1) / (float) steps;
    float width = t * head_width * lip_ratio;

    xverts1[i] = x + dy * width;
    yverts1[i] = y - dx * width;
    xverts2[i] = x - dy * width;
    yverts2[i] = y + dx * width;
  }

  if (vary_arrow_intensity) {
    float val = vis_get_arrow_length(x,y) / vis_max_arrow_length();
    val = 0.9 * val + 0.1;
    win->set_color_index ((int) (val * 255));
  }

  win->polygon_start();

  win->polygon_vertex (xorig, yorig);
  win->polygon_vertex (x1, y1);
  win->polygon_vertex (x3, y3);

  for (int i = 0; i < steps; i++)
    win->polygon_vertex (xverts1[i], yverts1[i]);

  for (int i = steps - 1; i >= 0; i--)
    win->polygon_vertex (xverts2[i], yverts2[i]);

  win->polygon_vertex (x4, y4);
  win->polygon_vertex (x2, y2);
  win->polygon_vertex (xorig, yorig);

  win->polygon_fill();

  delete xverts1;
  delete yverts1;
  delete xverts2;
  delete yverts2;
}


/******************************************************************************
Draw a fancy arrow to a postscript file.

Entry:
  dt          - step size
  file_out    - file to write to
  xorig,yorig - position to begin arrow
  arrow_len   - arrow length
  head_length - arrowhead length
  head_width  - arrowhead width
  lip_ratio   - how big a lip to make between arrowhead and the arrow's body
******************************************************************************/

void Streamline::draw_fancy_arrow_ps(
  float dt,
  ofstream *file_out,
  float xorig,
  float yorig,
  float arrow_len,
  float head_length,
  float head_width,
  float lip_ratio
)
{
  float x,y;
  float dx,dy;
  float len;
  float dlen;
  int steps;

  /* find the base of the arrowhead */

  steps = (int) floor (head_length / dt);
  dlen = head_length / steps;

  x = xorig;
  y = yorig;

  for (int i = 0; i < steps; i++) {
    dx = vf->xval(x,y);
    dy = vf->yval(x,y);
    len = sqrt (dx*dx + dy*dy);
    if (len != 0.0) {
      dx /= len;
      dy /= len;
    }
    x += dx * dlen;
    y += dy * dlen;
  }

  float x1 = x + dy * head_width;
  float y1 = y - dx * head_width;
  float x2 = x - dy * head_width;
  float y2 = y + dx * head_width;

  float x3 = x + dy * lip_ratio * head_width;
  float y3 = y - dx * lip_ratio * head_width;
  float x4 = x - dy * lip_ratio * head_width;
  float y4 = y + dx * lip_ratio * head_width;

  /* now step along the arrow's body */

  steps = (int) floor ((arrow_len - head_length) / dt);
  dlen = (arrow_len - head_length) / steps;

  float *xverts1 = new float[steps];
  float *yverts1 = new float[steps];
  float *xverts2 = new float[steps];
  float *yverts2 = new float[steps];

  for (int i = 0; i < steps; i++) {

    dx = vf->xval(x,y);
    dy = vf->yval(x,y);
    len = sqrt (dx*dx + dy*dy);
    if (len != 0.0) {
      dx /= len;
      dy /= len;
    }
    x += dx * dlen;
    y += dy * dlen;

    float t = (steps - i + 1) / (float) steps;
    float width = t * head_width * lip_ratio;

    xverts1[i] = x + dy * width;
    yverts1[i] = y - dx * width;
    xverts2[i] = x - dy * width;
    yverts2[i] = y + dx * width;
  }

  if (vary_arrow_intensity) {
    float val = vis_get_arrow_length(x,y) / vis_max_arrow_length();
    val = 0.9 * val + 0.1;
    val = 1 - val;
    *file_out << val << " setgray" << endl;
  }

  *file_out << "newpath " << xorig << " " << yorig << " moveto" << endl;
  *file_out << x1 << " " << y1 << " lineto" << endl;
  *file_out << x3 << " " << y3 << " lineto" << endl;

  for (int i = 0; i < steps; i++)
    *file_out << xverts1[i] << " " << yverts1[i] << " lineto" << endl;

  for (int i = steps - 1; i >= 0; i--)
    *file_out << xverts2[i] << " " << yverts2[i] << " lineto" << endl;

  *file_out << x4 << " " << y4 << " lineto" << endl;
  *file_out << x2 << " " << y2 << " lineto" << endl;
  *file_out << xorig << " " << yorig << " lineto" << endl;
  *file_out << " closepath fill" << endl;

  delete xverts1;
  delete yverts1;
  delete xverts2;
  delete yverts2;
}


/******************************************************************************
Draw the body of an arrow or a dash.

Entry:
  win - window to draw into
  x,y - position to begin arrow
  len - arrow length
******************************************************************************/

void Streamline::draw_dash(
  Window2d *win,
  float x,
  float y,
  float len
)
{
  float dt = 1.0 / win->xsize;   /* delta length for stepping along lines */
  int steps = (int) floor (len / dt);
  float dlen = len / steps;

  for (int i = 0; i < steps; i++) {

    float dx = vf->xval(x,y);
    float dy = vf->yval(x,y);
    float length = sqrt (dx*dx + dy*dy);
    if (length != 0.0) {
      dx /= length;
      dy /= length;
    }

    float xnew = x + dx * dlen;
    float ynew = y + dy * dlen;

    win->line (x, y, xnew, ynew);

    x = xnew;
    y = ynew;
  }
}


/******************************************************************************
Draw an arrowhead (just the point) in a given window.

Entry:
  win          - window to draw into
  x,y          - position to begin arrow
  arrow_length - arrowhead length
  arrow_width  - arrowhead width
******************************************************************************/

void Streamline::draw_arrowhead(
  Window2d *win,
  float x,
  float y,
  float arrow_length,
  float arrow_width
)
{
  float xx,yy;

  /* get flow direction */

  float dx = vf->xval(x,y);
  float dy = vf->yval(x,y);
  float len = sqrt (dx*dx + dy*dy);
  dx /= len;
  dy /= len;

  /* now get flow direction halfway down the arrowhead's length */

  xx = x + 0.5 * dx * arrow_length;
  yy = y + 0.5 * dy * arrow_length;
  dx = vf->xval(xx,yy);
  dy = vf->yval(xx,yy);
  len = sqrt (dx*dx + dy*dy);
  dx /= len;
  dy /= len;

  /* other end of box containing arrowhead */

  xx = x + dx * arrow_length;
  yy = y + dy * arrow_length;

  /* tails of arrowhead */

  float x1 = xx - 0.5 * dy * arrow_width;
  float y1 = yy + 0.5 * dx * arrow_width;
  float x2 = xx + 0.5 * dy * arrow_width;
  float y2 = yy - 0.5 * dx * arrow_width;

  win->line(x, y, x1, y1);
  win->line(x, y, x2, y2);
}

static float len_sum;
static float *lens;


/******************************************************************************
Travel along a streamline for a given distance and return the final position.

Entry:
  dist - distance to travel

Exit:
  xout,yout - the final position
  returns 1 if we went off the streamline, 0 if not
******************************************************************************/

int Streamline::search_on_streamline(float dist, float& xout, float& yout)
{
  int pos = 0;

  if (dist < 0 || dist > len_sum)
    return (1);

  while (lens[pos] < dist && pos < samples - 1)
    pos++;

  if (pos >= samples-1)
    return (1);

  float fract = (dist - lens[pos-1]) / (lens[pos] - lens[pos-1]);
  xout = pts[pos-1].x + fract * (pts[pos].x - pts[pos-1].x);
  yout = pts[pos-1].y + fract * (pts[pos].y - pts[pos-1].y);

  return (0);
}


/******************************************************************************
Return a goodness measure based on where we start making dashes.  We want
there to be very little of the streamline's length that is not used.
******************************************************************************/

float Streamline::dash_helper(
  float dist,
  float separation,
  Window2d *win
)
{
  float x,y;
  int bad1,bad2;
  float x1,y1,x2,y2;
  float place;
  float center;
  float len;
  float place_start;
  float place_end;
  float sep,center_sep;
  float delta = 0.5 / win->xsize;

  /* find the midpoint of the streamline */

  float streamline_center = dist;
  search_on_streamline (dist, x, y);

  /* find both ends of the center arrow */

  float center_len = 0.5 * vis_get_arrow_length (x, y);
  center_sep = separation * vis_get_arrow_length (x,y) / vis_max_arrow_length();
  bad1 = search_on_streamline (streamline_center + center_len, x1, y1);
  bad2 = search_on_streamline (streamline_center - center_len, x2, y2);

  if (bad1 || bad2) {
    place_start = len_sum * 0.5;
    place_end   = len_sum * 0.5;
    goto here;
  }

//   draw_fancy_arrow (win, x2, y2, center_len * 2, al, aw, 0.5);

  place_start = streamline_center - center_len;
  place_end   = streamline_center + center_len;

  /* move forward along the streamline, drawing arrows */

  place = streamline_center + center_len + center_sep;

  while (place < len_sum) {

    search_on_streamline (place, x, y);
    len = 0.5 * vis_get_arrow_length (x, y);
    sep = separation * vis_get_arrow_length (x, y) / vis_max_arrow_length();
    center = place + delta;

    while (center - len < place) {
      center += delta;
      if (center > len_sum) {
        goto there;
      }
      search_on_streamline (center, x, y);
      len = 0.5 * vis_get_arrow_length (x, y);
      sep = separation * vis_get_arrow_length (x, y) / vis_max_arrow_length();
    }

    bad1 = search_on_streamline (center + len, x1, y1);
    bad2 = search_on_streamline (center - len, x2, y2);

    if (bad1 || bad2)
      goto there;

//    draw_fancy_arrow (win, x2, y2, len * 2, al, aw, 0.5);
    place_end = center + len;

    place = center + len + sep;
  }
  
  there:

  /* move backward along the streamline, drawing arrows */

  place = streamline_center - center_len - center_sep;

  while (place > 0) {

    search_on_streamline (place, x, y);
    len = 0.5 * vis_get_arrow_length (x, y);
    sep = separation * vis_get_arrow_length (x, y) / vis_max_arrow_length();
    center = place - delta;

    while (center + len > place) {
      center -= delta;
      if (center < 0) {
        goto here;
      }
      search_on_streamline (center, x, y);
      len = 0.5 * vis_get_arrow_length (x, y);
      sep = separation * vis_get_arrow_length (x, y) / vis_max_arrow_length();
    }

    bad1 = search_on_streamline (center + len, x1, y1);
    bad2 = search_on_streamline (center - len, x2, y2);

    if (bad1 || bad2)
      goto here;

//    draw_fancy_arrow (win, x2, y2, len * 2, al, aw, 0.5);
    place_start = center - len;

    place = center - len - sep;
  }

  here:

  float d1 = place_start;
  float d2 = len_sum - place_end;

  /* return distance that is unused (doesn't have arrows on it) */

  return (d1 + d2);
}


/******************************************************************************
Draw a dashed streamline in a given window or a postscript file.

Entry:
  win          - window to draw into
  file_out     - postscript file to write to
  separation   - separation distance between dashes
  arrow_length - arrowhead length
  arrow_width  - arrowhead width
******************************************************************************/

void Streamline::variable_draw_dashed(
  Window2d *win,
  ofstream *file_out,
  float separation,
  float arrow_length,
  float arrow_width
)
{
  int i;
  float x,y;
  float xold,yold;
  float x1,y1,x2,y2;
  float place;
  float center;
  int bad1,bad2;
  float streamline_center;
  float center_len;
  float len;
  float al,aw;
  float delta;
  float sep,center_sep;

  lens = new float[samples];
  lens[0] = 0;

  /* compute the lengths of all the tiny segments between the point samples */

  len_sum = 0;
  for (i = 1; i < samples; i++) {
    float dx = pts[i].x - pts[i-1].x;
    float dy = pts[i].y - pts[i-1].y;
    float len = sqrt (dx * dx + dy * dy);
    len_sum += len;
    lens[i] = len_sum;
  }

#if 1

/* draw underlying streamline */

  if (file_out == NULL) {
    win->gray_ramp();
    win->set_color_index (100);
    xold = pts[0].x;
    yold = pts[0].y;

    for (i = 1; i < samples; i++) {
      x = pts[i].x;
      y = pts[i].y;
      win->line (x, y, xold, yold);
      xold = x;
      yold = y;
    }
  }

#endif

  if (file_out == NULL)
    win->set_color_index (255);

  /* search for a good center */

  float good_center = len_sum * 0.5;
  search_on_streamline (good_center, x, y);
  center_len = 0.5 * vis_get_arrow_length (x, y);

  float best_center = good_center;
  float best_measure = len_sum;

  delta = 1.0 / win->xsize;
  int steps = center_len / delta;

  for (i = 0; i < steps; i++) {
    float val = dash_helper (good_center, separation, win);
    if (val < best_measure) {
      best_measure = val;
      best_center = good_center;
    }
    good_center += delta;
  }

  /* find the midpoint of the streamline */

  streamline_center = best_center;
  search_on_streamline (best_center, x, y);

  /* find both ends of the center arrow */

  center_len = 0.5 * vis_get_arrow_length (x, y);
  center_sep = separation * vis_get_arrow_length (x,y) / vis_max_arrow_length();
  bad1 = search_on_streamline (streamline_center + center_len, x1, y1);
  bad2 = search_on_streamline (streamline_center - center_len, x2, y2);

  if (bad1 || bad2)
    goto here;

  al = center_len * arrow_length;
  aw = center_len * arrow_width;
  if (file_out) {
    float dt = 1.0 / win->xsize;
    draw_fancy_arrow_ps (dt, file_out, x2, y2, center_len * 2, al, aw, 0.5);
  }
  else
    draw_fancy_arrow (win, x2, y2, center_len * 2, al, aw, 0.5);

  /* move forward along the streamline, drawing arrows */

  place = streamline_center + center_len + center_sep;

  delta = 0.5 / win->xsize;

  while (place < len_sum) {

    search_on_streamline (place, x, y);
    len = 0.5 * vis_get_arrow_length (x, y);
    sep = separation * vis_get_arrow_length (x, y) / vis_max_arrow_length();
    center = place + delta;

    while (center - len < place) {
      center += delta;
      if (center > len_sum) {
        goto there;
      }
      search_on_streamline (center, x, y);
      len = 0.5 * vis_get_arrow_length (x, y);
      sep = separation * vis_get_arrow_length (x, y) / vis_max_arrow_length();
    }

    bad1 = search_on_streamline (center + len, x1, y1);
    bad2 = search_on_streamline (center - len, x2, y2);

    if (bad1 || bad2)
      goto there;

    al = len * arrow_length;
    aw = len * arrow_width;
    if (file_out) {
      float dt = 1.0 / win->xsize;
      draw_fancy_arrow_ps (dt, file_out, x2, y2, len * 2, al, aw, 0.5);
    }
    else
      draw_fancy_arrow (win, x2, y2, len * 2, al, aw, 0.5);

    place = center + len + sep;
  }
  
  there:

  /* move backward along the streamline, drawing arrows */

  place = streamline_center - center_len - center_sep;

  while (place > 0) {

    search_on_streamline (place, x, y);
    len = 0.5 * vis_get_arrow_length (x, y);
    sep = separation * vis_get_arrow_length (x, y) / vis_max_arrow_length();
    center = place - delta;

    while (center + len > place) {
      center -= delta;
      if (center < 0) {
        goto here;
      }
      search_on_streamline (center, x, y);
      len = 0.5 * vis_get_arrow_length (x, y);
      sep = separation * vis_get_arrow_length (x, y) / vis_max_arrow_length();
    }

    bad1 = search_on_streamline (center + len, x1, y1);
    bad2 = search_on_streamline (center - len, x2, y2);

    if (bad1 || bad2)
      goto here;

    al = len * arrow_length;
    aw = len * arrow_width;
    if (file_out) {
      float dt = 1.0 / win->xsize;
      draw_fancy_arrow_ps (dt, file_out, x2, y2, len * 2, al, aw, 0.5);
    }
    else
      draw_fancy_arrow (win, x2, y2, len * 2, al, aw, 0.5);

    place = center - len - sep;
  }

  here:

  delete lens;

  win->flush();
}


/******************************************************************************
Draw a dashed streamline in a given window or a postscript file.

Entry:
  win          - window to draw into
  file_out     - postscript file to write to
  dash_length  - length of dashes
  separation   - separation distance between dashes
  arrow_length - arrowhead length
  arrow_width  - arrowhead width
******************************************************************************/

void Streamline::draw_dashed(
  Window2d *win,
  ofstream *file_out,
  float dash_length,
  float separation,
  float arrow_length,
  float arrow_width
)
{
  int i;
  float len_sum = 0;
  float *lens;

  lens = new float[samples];
  lens[0] = 0;

  /* compute the lengths of all the tiny segments between the point samples */

  for (i = 1; i < samples; i++) {
    float dx = pts[i].x - pts[i-1].x;
    float dy = pts[i].y - pts[i-1].y;
    float len = sqrt (dx * dx + dy * dy);
    len_sum += len;
    lens[i] = len_sum;
  }

  /* calculate how many dashes we should have and */
  /* adjust the dash length accordingly */

  int num_dashes = (int) floor (0.5 + len_sum / dash_length);
  if (num_dashes < 2)
    num_dashes = 1;
  dash_length = len_sum / num_dashes;

  /* now make the dashes */

  int pos = 0;
  float fract;

  for (i = 0; i < num_dashes; i++) {

    float start = i * dash_length + 0.5 * separation;
    float end   = (i+1) * dash_length - 0.5 * separation;

    while (lens[pos] < start)
      pos++;

    fract = (start - lens[pos-1]) / (lens[pos] - lens[pos-1]);
    float x1 = pts[pos-1].x + fract * (pts[pos].x - pts[pos-1].x);
    float y1 = pts[pos-1].y + fract * (pts[pos].y - pts[pos-1].y);

    while (lens[pos] < end)
      pos++;

    fract = (end - lens[pos-1]) / (lens[pos] - lens[pos-1]);
//    float x2 = pts[pos-1].x + fract * (pts[pos].x - pts[pos-1].x);
//    float y2 = pts[pos-1].y + fract * (pts[pos].y - pts[pos-1].y);

//    win->line (x1, y1, x2, y2);

#if 1
    if (file_out) {
      float dt = 1.0 / win->xsize;
      draw_fancy_arrow_ps (dt, file_out, x1, y1, dash_length - separation,
                           arrow_length, arrow_width, 0.5);
    }
    else
      draw_fancy_arrow (win, x1, y1, dash_length - separation,
                        arrow_length, arrow_width, 0.5);
#endif

#if 0
    draw_dash (win, x1, y1, dash_length - separation);

    if (arrow_length != 0 && arrow_width != 0)
      draw_arrowhead (win, x1, y1, arrow_length, arrow_width);
#endif

  }

  delete lens;
}


/******************************************************************************
Remember how this streamline affects the value at a particular pixel of a
low-pass filtered version of the streamline.

Entry:
  i,j   - location of pixel in low-pass filtered image
  value - value that changes the pixel
******************************************************************************/

void Streamline::add_value(int i, int j, float value)
{
  if (num_values >= max_values - 1) {
    PixelValue *temp = new PixelValue[max_values * 2];
    for (int i = 0; i < max_values; i++) {
      temp[i].i = values[i].i;
      temp[i].j = values[i].j;
      temp[i].value = values[i].value;
    }
    delete values;
    values = temp;
    max_values *= 2;
  }

  values[num_values].i = i;
  values[num_values].j = j;
  values[num_values].value = value;
  num_values++;
}


/******************************************************************************
Get a value for a pixel that was stored by add_value().

Entry:
  num - index of the value record

Exit:
  i,j   - location of pixel in low-pass filtered image
  value - value that changes the pixel
******************************************************************************/

void Streamline::get_value(int num, int &i, int &j, float &value)
{
  i = values[num].i;
  j = values[num].j;
  value = values[num].value;
}


/******************************************************************************
Clear out the list of values for this streamline.
******************************************************************************/

void Streamline::clear_values()
{
  num_values = 0;
}


/******************************************************************************
Write out a streamline to a postscript file.
******************************************************************************/

void Streamline::write_postscript(ofstream *file_out)
{
  *file_out << (1 - intensity) << " setgray" << endl;
  *file_out << "init_width" << endl;

  for (int i = start_reduction; i < samples - end_reduction - 1; i++) {

    float width = 1;
    int set_width = 0;

    if (vis_draw_width_varies()) {
      width = vis_get_draw_width (xs(i), ys(i));
      set_width = 1;
    }

    if (taper_head != 0.0 || taper_tail != 0.0) {
      width *= pts[i].intensity;
      set_width = 1;
    }

    if (set_width)
      *file_out << width << " sw" << endl;

    *file_out << xs(i)   << " " << ys(i)   << " "
              << xs(i+1) << " " << ys(i+1) << " ln" << endl;
  }

  float x = xs(samples - end_reduction - 1);
  float y = ys(samples - end_reduction - 1);

  if (arrow_type == OPEN_ARROW)
    postscript_draw_arrow (file_out, x, y, arrow_width, arrow_length,
                           (int) thickness_default, 1);
  else
    postscript_draw_arrow (file_out, x, y, arrow_width, arrow_length,
                           (int) thickness_default, 0);

#if 0

  if (arrow_type == OPEN_ARROW) {

    float dt = arrow_length / arrow_steps;
    float x = xs(samples - end_reduction - 1);
    float y = ys(samples - end_reduction - 1);

    /* left side of arrow */
    float x1_old = x;
    float y1_old = y;

    /* right side of arrow */
    float x2_old = x;
    float y2_old = y;

    for (i = 0; i < arrow_steps; i++) {
      float t = arrow_width * i / arrow_steps;
      float dx = vf->xval(x,y);
      float dy = vf->yval(x,y);
      x -= dt * dx;
      y -= dt * dy;
      float x1 = x - t * dy;
      float y1 = y + t * dx;
      float x2 = x + t * dy;
      float y2 = y - t * dx;
      *file_out << x1 << " " << y1 << " "
                << x1_old << " " << y1_old << " ln" << endl;
      *file_out << x2 << " " << y2 << " "
                << x2_old << " " << y2_old << " ln" << endl;
      x1_old = x1;
      y1_old = y1;
      x2_old = x2;
      y2_old = y2;
    }

  }

#endif

}


/******************************************************************************
Return a copy of a bundle.  This routine actually makes copies of all the
streamlines.
******************************************************************************/

Bundle* Bundle::copy()
{
  Bundle *bundle = new Bundle();

  /* make copies of all the streamlines and place them into the new bundle */
  for (int i = 0; i < num_lines; i++) {
    Streamline *st = lines[i]->copy();
    bundle->add_line (st);
  }

  return (bundle);
}


/******************************************************************************
Perform one or more sorting passes on the streamlines based on the quality.

Entry:
  passes - number of sorting passes
******************************************************************************/

void Bundle::sort_passes(int passes)
{
  int i;
  int change;
  Streamline *temp_line;

#define line_swap(a,b)  { temp_line = lines[a]; \
                          lines[a] = lines[b];  \
                          lines[b] = temp_line; \
                          change = 1; }

  /* perform a number of bubble sort passes */

  for (int k = 0; k < passes; k++) {

    /* bubble up */

    change = 0;
    for (i = 0; i < num_lines-1; i++)
      if (lines[i]->quality < lines[i+1]->quality)
        line_swap (i, i+1);

    if (!change)
      break;

    /* bubble down */

    change = 0;
    for (i = num_lines-2; i >= 0; i--)
      if (lines[i]->quality < lines[i+1]->quality)
        line_swap (i, i+1);

    if (!change)
      break;
  }

  /* re-index the streamlines */
  for (i = 0; i < num_lines; i++)
    lines[i]->list_index = i;
}


/******************************************************************************
Compare the quality of two streamlines (for sorting using qsort).
******************************************************************************/

int compare_quality(const void *p1, const void *p2)
{
  Streamline **s1 = (Streamline **) p1;
  Streamline **s2 = (Streamline **) p2;

//  printf ("s1 s2: %f %f\n", (*s1)->get_quality(), (*s2)->get_quality());

  if ((*s1)->get_quality() < (*s2)->get_quality())
    return (1);
  else if ((*s1)->get_quality() > (*s2)->get_quality())
    return (-1);
  else
    return (0);
}


/******************************************************************************
Sort the streamlines in the bundle by quality.
******************************************************************************/

void Bundle::quality_sort()
{
  int i;

#if 0

  printf ("before\n");
  for (i = 0; i < num_lines; i++)
    printf ("%f\n", lines[i]->quality);

#endif

  /* sort the streamlines based on quality */
  qsort (lines, num_lines, sizeof (Streamline *), compare_quality);

  /* re-index the streamlines */
  for (i = 0; i < num_lines; i++)
    lines[i]->list_index = i;

#if 0

  printf ("after\n");
  for (i = 0; i < num_lines; i++)
    printf ("%f\n", lines[i]->quality);

  sort_passes (10);

  printf ("after sort passes\n");
  for (i = 0; i < num_lines; i++)
    printf ("%f\n", lines[i]->quality);

#endif
}


/******************************************************************************
Add a streamline to a bundle.

Entry:
  st - streamline to add
******************************************************************************/

void Bundle::add_line(Streamline *st)
{
  /* make sure there is enough room in the list */

  if (num_lines >= max_lines - 1) {
    max_lines *= 2;
    Streamline **temp = new Streamline *[max_lines];
    for (int i = 0; i < num_lines; i++)
      temp[i] = lines[i];
    lines = temp;
  }

  /* add new streamline to list */

  lines[num_lines] = st;
  st->list_index = num_lines;
  num_lines++;
}


/******************************************************************************
Remove a streamline from a bundle.

Entry:
  st - streamline to delete
******************************************************************************/

void Bundle::delete_line(Streamline *st)
{
  int index = st->list_index;
  num_lines--;
  lines[index] = lines[num_lines];
  lines[index]->list_index = index;
}


/******************************************************************************
Render a filtered version of a bundle of streamlines to an image.

Entry:
  xsize    - width of image
  ysize    - height
  radius - filter radius
******************************************************************************/

FloatImage *Bundle::filtered_render(int xsize, int ysize, float radius)
{
  Lowpass *low = new Lowpass (xsize, ysize, radius, 0.6);

  for (int i = 0; i < num_lines; i++) {
    Streamline *st = lines[i];
    for (int j = 0; j < st->samples - 1; j++) {
      float taper_scale = 0.5 * (st->pts[j].intensity + st->pts[j+1].intensity);
      low->filter_segment (st->xs(j), st->ys(j), st->xs(j+1), st->ys(j+1),
                           taper_scale);
    }
  }

  FloatImage *image = low->get_image_copy();
  delete low;

  return (image);
}


/******************************************************************************
Save to a file a filtered version of a bundle of streamlines.

Entry:
  filename - file to write image to
  xsize    - width of image
  ysize    - height
******************************************************************************/

void Bundle::write_pgm(char *filename, int xsize, int ysize)
{
  Lowpass *low = new Lowpass (xsize, ysize, 2.0, 0.6);

  for (int i = 0; i < num_lines; i++) {
    Streamline *st = lines[i];
    for (int j = 0; j < st->samples - 1; j++) {
      float taper_scale = 0.5 * (st->pts[j].intensity + st->pts[j+1].intensity);
      low->filter_segment (st->xs(j), st->ys(j), st->xs(j+1), st->ys(j+1),
                           taper_scale);
    }
  }

  FloatImage *image = low->get_image_ptr();
  image->write_pgm (filename, 0.0, 1.1);

  delete low;
}


/******************************************************************************
Write out an ascii file containing all streamlines.
******************************************************************************/

void Bundle::write_ascii(char *filename, int taper_info)
{
  ofstream file_out (filename);

  file_out << "! this file contains " << num_lines << " streamlines" << endl;
  file_out << endl;
  file_out << "delta_step " << delta_step << endl;
  file_out << endl;

  for (int i = 0; i < num_lines; i++) {
    Streamline *st = lines[i];
    file_out << "st " <<
      st->xorig << " " <<
      st->yorig << " " <<
      st->length1 << " " <<
      st->length2 << " ";
    if (taper_info)
      file_out << st->taper_tail << " " << st->taper_head;
    file_out << endl;
  }

  file_out << endl;
}


/******************************************************************************
Write the start of a postscript file.
******************************************************************************/

void write_postscript_start(ofstream *file_out)
{
  *file_out << "%!" << endl;
  *file_out << "%" << endl;
  *file_out << "% Streamlines" << endl;
  *file_out << "%" << endl;

  *file_out << "/size 6 def" << endl;
  *file_out << "72 72 3 mul translate" << endl;
  *file_out << "72 size mul dup scale" << endl;
  *file_out << endl;

  *file_out << "/ln {" << endl;
  *file_out << "newpath" << endl;
  *file_out << "moveto lineto" << endl;
  *file_out << "stroke" << endl;
  *file_out << "} def" << endl;
  *file_out << endl;

  *file_out << "1 72 div size div setlinewidth" << endl;
  *file_out << "/init_width { 1 72 div size div setlinewidth } def" << endl;
  *file_out << "/sw { 72 div size div setlinewidth } def" << endl;
}


/******************************************************************************
Write the end of a postscript file.
******************************************************************************/

void write_postscript_end(ofstream *file_out)
{
  *file_out << endl;
  *file_out << "0 setgray" << endl;
  *file_out << "4 72 div size div setlinewidth" << endl;
  *file_out << "0 0 moveto" << endl;
  *file_out << "1 0 lineto" << endl;
  *file_out << "1 1 lineto" << endl;
  *file_out << "0 1 lineto" << endl;
  *file_out << "closepath stroke" << endl;
  *file_out << endl;

  *file_out << endl;
  *file_out << "showpage" << endl;
  *file_out << endl;
}


/******************************************************************************
Write out a postscript image of the streamlines in a bundle.
******************************************************************************/

void Bundle::write_postscript(ofstream *file_out)
{
  for (int i = 0; i < num_lines; i++)
    lines[i]->write_postscript (file_out);
}


/******************************************************************************
Clamp position to the screen region.

Entry:
  x,y  - position to clamp
  ymax - maximum y value

Entry:
  x,y - clamped position
******************************************************************************/

void clamp_to_screen (float &x, float &y, float ymax)
{
  const float tiny = 0.000001;

  if (x < tiny)
    x = tiny;

  if (x > 1 - tiny)
    x = 1 - tiny;

  if (y < tiny)
    y = tiny;

  if (y > ymax - tiny)
    y = ymax - tiny;
}

