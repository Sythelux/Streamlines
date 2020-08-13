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
#include "sd_vfield.h"
#include "sd_streamline.h"
#include "sd_params.h"
#include "stdraw.h"
#include "window.h"
#include "floatimage.h"
#include "clip_line.h"

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


/******************************************************************************
Draw a streamline.
******************************************************************************/

void Streamline::draw(Picture *pic)
{
  int i;
  float width;
  float val;

  /* draw line segments */

  float x_old = xs(start_reduction);
  float y_old = ys(start_reduction);

  for (i = start_reduction + 1; i < samples - end_reduction; i++) {

    float x = xs(i);
    float y = ys(i);

    val = vis_get_intensity(x,y);
    pic->set_intensity (val * intensity * pts[i].intensity);
    width = vis_get_draw_width(x,y) * pts[i].intensity;
    pic->thick_line (x, y, x_old, y_old, width);

    x_old = x;
    y_old = y;
  }

#if 0
  /* maybe draw an arrow */

  pic->set_intensity (intensity);

  float x = xs(samples - end_reduction - 1);
  float y = ys(samples - end_reduction - 1);
  width = vis_get_draw_width(x,y) * pts[i].intensity;
  val = vis_get_intensity(x,y);
  pic->set_intensity (val * intensity * pts[samples-end_reduction-1].intensity);

  if (arrow_type == OPEN_ARROW)
    draw_arrow(pic, x, y, arrow_width, arrow_length, width);
  else
    draw_arrow(pic, x, y, arrow_width, arrow_length, width);
#endif

  pic->flush();
}


/******************************************************************************
Draw a fancy arrow.

Entry:
  pic         - picture to draw into
  xorig,yorig - position to begin arrow
  arrow_len   - arrow length
  head_length - arrowhead length
  head_width  - arrowhead width
  lip_ratio   - how big a lip to make between arrowhead and the arrow's body
******************************************************************************/

void Streamline::draw_fancy_arrow(
  Picture *pic,
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

  float dt = delta_step;   /* delta length for stepping along lines */

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
    x -= dx * dlen;
    y -= dy * dlen;
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

  for (i = 0; i < steps; i++) {

    dx = vf->xval(x,y);
    dy = vf->yval(x,y);
    len = sqrt (dx*dx + dy*dy);
    if (len != 0.0) {
      dx /= len;
      dy /= len;
    }
    x -= dx * dlen;
    y -= dy * dlen;

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
    pic->set_intensity (val);
  }

  pic->polygon_start();

  pic->polygon_vertex (xorig, yorig);
  pic->polygon_vertex (x1, y1);
  pic->polygon_vertex (x3, y3);

  for (i = 0; i < steps; i++)
    pic->polygon_vertex (xverts1[i], yverts1[i]);

  for (i = steps - 1; i >= 0; i--)
    pic->polygon_vertex (xverts2[i], yverts2[i]);

  pic->polygon_vertex (x4, y4);
  pic->polygon_vertex (x2, y2);
  pic->polygon_vertex (xorig, yorig);

  pic->polygon_fill();

  delete xverts1;
  delete yverts1;
  delete xverts2;
  delete yverts2;
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
  float separation
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
  float delta = 0.5 * delta_step;

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
Draw a dashed streamline.

Entry:
  pic          - window to draw into
  separation   - separation distance between dashes
  arrow_length - arrowhead length
  arrow_width  - arrowhead width
******************************************************************************/

void Streamline::variable_draw_dashed(
  Picture *pic,
  float separation,
  float arrow_length,
  float arrow_width
)
{
  int i;
  float x,y;
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
  float scale;

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

#if 0

/* draw underlying streamline, for debugging */

  pic->set_intensity (0.7);
  float xold = pts[0].x;
  float yold = pts[0].y;

  for (i = 1; i < samples; i++) {
    x = pts[i].x;
    y = pts[i].y;
    pic->thick_line (x, y, xold, yold, 2.0);
    xold = x;
    yold = y;
  }

#endif

  pic->set_intensity (1.0);

  /* search for a good center */

  float good_center = len_sum * 0.5;
  search_on_streamline (good_center, x, y);
  center_len = 0.5 * vis_get_arrow_length (x, y);

  float best_center = good_center;
  float best_measure = len_sum;

  delta = delta_step;
  int steps = center_len / delta;

  for (i = 0; i < steps; i++) {
    float val = dash_helper (good_center, separation);
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

  scale = vis_get_arrow_length (x, y) / vis_max_arrow_length();
  al = scale * arrow_length;
  aw = scale * arrow_width;

  draw_fancy_arrow (pic, x1, y1, center_len * 2, al, aw, 0.5);

  /* move forward along the streamline, drawing arrows */

  delta = 0.5 * delta_step;

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

    scale = vis_get_arrow_length (x, y) / vis_max_arrow_length();
    al = scale * arrow_length;
    aw = scale * arrow_width;

    draw_fancy_arrow (pic, x1, y1, len * 2, al, aw, 0.5);

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

    scale = vis_get_arrow_length (x, y) / vis_max_arrow_length();
    al = scale * arrow_length;
    aw = scale * arrow_width;

    draw_fancy_arrow (pic, x1, y1, len * 2, al, aw, 0.5);

    place = center - len - sep;
  }

  here:

  delete lens;

  pic->flush();
}


/******************************************************************************
Draw a dashed streamline in a given window or a postscript file.

Entry:
  pic          - picture to draw into
  dash_length  - length of dashes
  separation   - separation distance between dashes
  arrow_length - arrowhead length
  arrow_width  - arrowhead width
******************************************************************************/

void Streamline::draw_dashed(
  Picture *pic,
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
    float x2 = pts[pos-1].x + fract * (pts[pos].x - pts[pos-1].x);
    float y2 = pts[pos-1].y + fract * (pts[pos].y - pts[pos-1].y);

//    draw_fancy_arrow (pic, x1, y1, dash_length - separation,
//                      arrow_length, arrow_width, 0.5);

    draw_fancy_arrow (pic, x2, y2, dash_length - separation,
                      arrow_length, arrow_width, 0.5);

  }

  delete lens;
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
Write out an ascii file containing all streamlines.
******************************************************************************/

void Bundle::write_ascii(char *filename, int taper_info)
{
  ofstream file_out (filename);

  file_out << "! this file contains " << num_lines << " streamlines" << endl;
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

