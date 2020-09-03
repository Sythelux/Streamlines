/*

Streamline placement by image-guided optimization.

Version 0.5

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
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <math.h>
#include "../libs/cli.h"
#include "../libs/window.h"
#include "../libs/floatimage.h"
#include "vfield.h"
#include "streamline.h"
#include "lowpass.h"
#include "repel.h"
#include "stplace.h"
#include "intersect.h"
#include "dissolve.h"
#include "visparams.h"

/* external declarations and forward pointers to routines */

extern void check_events();

void interpreter();

void new_interpreter();

VectorField *vf = NULL;         /* the vector field we're visualizing */
FloatImage *float_reg = NULL;   /* scalar field "register" */

Bundle *bundle;                 /* bundle of streamlines */
Bundle *bundle2;

Window2d *win;                  /* windows to draw into */
Window2d *win2;

int xsize = 512;                /* window sizes */
int ysize = 512;

int keep_reading_events;        /* for interrupting window event loop */

/* verbose mode? */
int verbose_flag = 0;

/* draw things? */
static int graphics_flag = 1;

/* write info to a file for later animation? */
int animation_flag = 0;
ofstream *anim_file;
int anim_index = 0;

/* graph the quality function? */
static int graph_the_quality = 1;

/* target value for lowpass filtered version of streamlines */
static float target_lowpass = 1.0;

/* radius of lowpass filter */
static float radius_lowpass = 2.0;

/* size of lowpass filtered image */
static int lowpass_size = 10;

/* length of streamlines */
// static float streamline_length = 0.1;

/* how much length can change */
// static float delta_length = 0.0;

/* how many times to move each streamline */
static int move_times = 1;

/* too short to live */
static float death_length = 0.0;

/* density threshold below which we birth a new streamline */
static float birth_thresh = 0.0;

/* amount more to blur to test for birth */
static float birth_blur = 1.0;

/* how often to birth (generation) */
static int generation = -1;

/* random move distance for relaxation */
static float rmove = 0.01;

/* how many streamlines to randomly place */
static int random_place_count = 300;

/* the lowpass filtered version of the streamlines */
static Lowpass *low;

/* how much to jitter the grid values */
static float jitter = 0.0;

/* how close the endpoints of two streamlines have to be before we join them */
/* (factor of the filter radius) */
// static float join_factor = 0.0;

/* fraction of the radius, for repelling and such */
static float epsilon = 0.1;

/* probabilities of selecting various streamline changes */

float odds_move = 0;
float odds_len = 0;
float odds_both = 1;

float odds_all_len = 1.0;
float odds_long_both = 0;
float odds_short_both = 0;
float odds_long_one = 0;
float odds_short_one = 0;

/* number of samples when estimating streamline quality */
static int sample_number = 10;

/* how far away are samples from the streamline? */
static float sample_radius = 1.0;

/* how many radii out do we travel away from the endpoints? */
static float sample_endpoint_distance = 3.0;

/* show the samples? */
static int show_samples = 0;

/* use quality estimates to guide streamline changes? */
static int quality_guide = 0;

/* tapered maximum */
static float taper_max = 0;

/* tapered maximum */
static float taper_delta = 0;

/* global arrow type */
static int arrow_type = SOLID_ARROW;

/* vary the intensity of fancy arrows? */
int vary_arrow_intensity = 0;

/* how much to step while integrating through the vector field */
float delta_step = 0.005;


/******************************************************************************
Main routine.
******************************************************************************/

int main(int argc, char *argv[])
{
  char *s;

  while (--argc > 0 && (*++argv)[0] == '-') {
    for (s = argv[0] + 1; *s; s++)
      switch (*s) {
        case 's':
          xsize = atoi(*++argv);
          ysize = atoi(*++argv);
          argc -= 2;
          break;
        case 'g':
          graphics_flag = 1 - graphics_flag;
          break;
        case 'v':
          verbose_flag = 1 - verbose_flag;
          break;
        default:
          break;
      }
  }

  /* read in a vector field */

  if (argc > 0) {
    vf = new VectorField(*argv);
    float_reg = vf->get_magnitude();
    /* normalize the field */
    vf->normalize();
  }

  /* set up graphics stuff */

  if (graphics_flag) {
    win = new Window2d(xsize, ysize);
    win->set_vscale(xsize / (float) ysize);
    win->map();
    win->flush();

    win2 = new Window2d(xsize, ysize);
    win2->set_vscale(xsize / (float) ysize);
    win2->map();
    win2->flush();
  }

  /* initialize visualization parameters */

  vis_initialize();

  /* initialize the bundle */

  bundle = new Bundle();

  /* initialize the scalar field */

  float_reg = new FloatImage(2, 2);

  /* call command interpreter */

  new_interpreter();

//  interpreter();

}


/******************************************************************************
Stop reading events.
******************************************************************************/

void stop_events(Window2d *win, int x, int y)
{
  keep_reading_events = 0;
}


/******************************************************************************
Draw the lowpass filtered image of the current streamline bundle.
******************************************************************************/

void draw_lowimage(Window2d *win, int x, int y)
{
  win2->gray_ramp();
  low->draw(win2);

  if (verbose_flag)
    printf("quality = %f\n", low->current_quality());

  graph_the_quality = 0;
}


/******************************************************************************
Clear the window.
******************************************************************************/

void clear_win(Window2d *win, int x, int y)
{
  win->clear();
  win->flush();
}


/******************************************************************************
Draw a field line at mouse click point.
******************************************************************************/

void line_at_click(Window2d *win, int xx, int yy)
{
  float x = xx / (float) xsize;
  float y = yy / (float) ysize;

  float delta = 1.0 / (float) win->xsize;

  float blen = vis_get_birth_length(x, y);
  Streamline *st = new Streamline(vf, x, y, blen, delta);
  st->draw(win);
}


/******************************************************************************
Let user click to make field lines.
******************************************************************************/

void lines_of_field()
{
  win->makeicolor(20, 255, 255, 255);
  win->set_color_index(20);

  win->left_down(line_at_click);
  win->middle_down(clear_win);
  win->right_down(stop_events);

  keep_reading_events = 1;

  while (keep_reading_events) {
    check_events();
  }
}


/******************************************************************************
Create random streamlines.
******************************************************************************/

void random_lines()
{
  if (graphics_flag) {
    win->clear();
    win->makeicolor(20, 255, 255, 255);
    win->set_color_index(20);
  }

  float len = 0.1;
  float delta = 1.0 / (float) xsize;

  for (int i = 0; i < 200; i++) {
    float x = drand48();
    float y = drand48();
    Streamline *st = new Streamline(vf, x, y, len, delta);
    if (graphics_flag)
      st->draw(win);
  }
}


/******************************************************************************
Test the line filtering.

Entry:
  num - number of lines to draw
******************************************************************************/

void line_test(int num)
{
  win->clear();
  win->makeicolor(BLACK, 0, 0, 0);
  win->makeicolor(RED, 255, 0, 0);
  win->makeicolor(WHITE, 255, 255, 255);
  win->set_color_index(WHITE);

  int xs = 100;
  int ys = 100;

  low = new Lowpass(xs, ys, 2.0, target_lowpass);

  float quality = low->current_quality();
  float delta = 5.0 / (float) xsize;

  float slen = 0.4;

  /* make a ring of streamlines */

  for (int i = 0; i < num; i++) {

    /* create a streamline */
    float theta = 2 * M_PI * i / (float) num;
    float r = 0.25;
    float x = r * cos(theta) + 0.5;
    float y = r * sin(theta) + 0.5;
    Streamline *st = new Streamline(vf, x, y, slen, delta);

    /* measure quality of the low-pass image */

    quality = low->new_quality(st);

    /* add streamline to image */

    add_streamline(st);
    low->add_line(st);
  }

  low->draw(win2);
}


/******************************************************************************
Draw an open arrow in a given window.

Entry:
  file_out  - postscript file to write to
  x,y       - position to draw arrow at
  width     - arrow width
  length    - arrow length
  thickness - width of line to draw with
  open      - open or closed arrow?
******************************************************************************/

void postscript_draw_arrow(
        ofstream *file_out,
        float x,
        float y,
        float width,
        float length,
        int thickness,
        int open
)
{
  float len;
  float dx, dy;
  float dt = 1.0 / win->xsize;   /* delta length for stepping along lines */

  /* find the base of the arrowhead */

  int steps = (int) floor(length / dt);
  dt = length / steps;

  float xx = x;
  float yy = y;

  for (int i = 0; i < steps; i++)
    vf->integrate(xx, yy, -dt, 1, xx, yy);

#if 1
  dx = x - xx;
  dy = y - yy;
  len = sqrt(dx * dx + dy * dy);
  if (len != 0.0) {
    dx /= len;
    dy /= len;
  }
#endif

#if 0
  len = vf->xyval (xx, yy, 1, dx, dy);
#endif

  float x1 = xx + dy * width;
  float y1 = yy - dx * width;
  float x2 = xx - dy * width;
  float y2 = yy + dx * width;

  if (open) {
    *file_out << x << " " << y << " " << x1 << " " << y1
              << " ln" << endl;
    *file_out << x << " " << y << " " << x2 << " " << y2
              << " ln" << endl;
  } else {
    *file_out << "newpath " << x << " " << y << " moveto" << endl;
    *file_out << x1 << " " << y1 << " lineto" << endl;
    *file_out << x2 << " " << y2 << " lineto" << endl;
    *file_out << "closepath fill" << endl;
  }
}


/******************************************************************************
Draw an open arrow in a given window.

Entry:
  win       - window in which to draw
  x,y       - position to draw arrow at
  width     - arrow width
  length    - arrow length
  thickness - width of line to draw with
  open      - open or closed arrow?
******************************************************************************/

void draw_arrow(
        Window2d *win,
        float x,
        float y,
        float width,
        float length,
        int thickness,
        int open
)
{
  float len;
  float dx, dy;
  float dt = 1.0 / win->xsize;   /* delta length for stepping along lines */

  /* find the base of the arrowhead */

  int steps = (int) floor(length / dt);
  dt = length / steps;

  float xx = x;
  float yy = y;

  for (int i = 0; i < steps; i++)
    vf->integrate(xx, yy, -dt, 1, xx, yy);

#if 1
  dx = x - xx;
  dy = y - yy;
  len = sqrt(dx * dx + dy * dy);
  if (len != 0.0) {
    dx /= len;
    dy /= len;
  }
#endif

#if 0
  len = vf->xyval (xx, yy, 1, dx, dy);
#endif

  float x1 = xx + dy * width;
  float y1 = yy - dx * width;
  float x2 = xx - dy * width;
  float y2 = yy + dx * width;

  if (open) {
    win->thick_line(x, y, x1, y1, thickness);
    win->thick_line(x, y, x2, y2, thickness);
  } else {
    win->polygon_start();
    win->polygon_vertex(x, y);
    win->polygon_vertex(x1, y1);
    win->polygon_vertex(x2, y2);
    win->polygon_fill();
  }
}


/******************************************************************************
Snap arrowheads to streamlines.  Arrowheads approximate a hex grid.

Entry:
  steps    - number of grid steps across and down
  width    - width of arrows
  length   - length of arrows
  filename - if non-NULL write to a postscript file instead of draw
  head     - snap only to heads of streamlines?
******************************************************************************/

void snap_arrows_to_streamlines(
        int steps,
        float width,
        float length,
        char *filename,
        int head
)
{
  float x, y;
  float s;

  /* maybe open postscript file */

  ofstream *file_out;

  if (filename) {
    file_out = new ofstream(filename);
    write_postscript_start(file_out);
    bundle->write_postscript(file_out);
  }

  /* set up drawing window stuff */

  win->set_color_index(WHITE);

  /* create a hash table of all streamline samples */

  RepelTable *repel;

  float min, max;
  vis_get_separation_extrema(min, max);
  repel = new RepelTable(vf, max * 2.0);

  for (int k = 0; k < bundle->num_lines; k++) {
    Streamline *st = bundle->get_line(k);
    if (head)
      repel->add_endpoints(st, 1, 0);
    else
      repel->add_all_points(st);
  }

  /* parameters for hex grid */

  float len = steps;
  float dist = 1.0 / len;
  float dist3 = dist * sqrt(3) / 2;

  /* create hex grid */

  for (int i = 0; i < len * 2; i++) {
    for (int j = 0; j < len * vf->getaspect(); j++) {

      /* slight jitter of position */
      float jx = jitter * (drand48() - 0.5);
      float jy = jitter * (drand48() - 0.5);

      /* position of hex point */

      x = dist3 * (0.5 + i + jx);
      if (x >= 1.0)
        break;
      if (i % 2)
        y = dist * (j + 0.25 + jy);
      else
        y = dist * (j + 0.75 + jy);

      /* snap the arrowhead to the nearest streamline */

      SamplePoint *point = repel->find_nearest(x, y);
      if (point == NULL)
        continue;

      float nx = point->x;
      float ny = point->y;
      s = vis_get_separation(nx, ny) / max;

      int thickness = 1;
      int steps = (int) (xsize * s * length);
      int open = (arrow_type == OPEN_ARROW);
      if (filename)
        postscript_draw_arrow(file_out, nx, ny, s * width, s * length,
                              thickness, open);
      else
        draw_arrow(win, nx, ny, s * width, s * length, thickness, open);
    }
  }

  win->flush();

  if (filename)
    write_postscript_end(file_out);
}


/******************************************************************************
Create a hexagonal grid showing vector field.

Entry:
  steps - number of grid steps across and down
******************************************************************************/

void hexagonal_grid(int steps)
{
  float x, y;
  Streamline *st;

  bundle = new Bundle();

  if (graphics_flag) {
    win->clear();
    win->makeicolor(20, 255, 255, 255);
    win->set_color_index(20);
  }

  float delta = delta_step;

  float len = steps;
  float dist = 1.0 / len;
  float dist3 = dist * sqrt(3) / 2;

  for (int i = 0; i < len * 2; i++) {
    for (int j = 0; j < len * vf->getaspect(); j++) {

      float jx = jitter * (drand48() - 0.5);
      float jy = jitter * (drand48() - 0.5);

      x = dist3 * (0.5 + i + jx);
      if (x >= 1.0)
        break;
      if (i % 2)
        y = dist * (j + 0.25 + jy);
      else
        y = dist * (j + 0.75 + jy);

      float blen = vis_get_birth_length(x, y);
      st = new Streamline(vf, x, y, blen, delta);
      bundle->add_line(st);
      if (graphics_flag)
        st->draw(win);
    }
  }

  if (graphics_flag)
    win->flush();
}


/******************************************************************************
Create a square grid showing vector field.

Entry:
  steps - number of grid steps across and down
******************************************************************************/

void square_grid(int steps)
{
  int i, j;
  Streamline *st;

  bundle = new Bundle();

  if (graphics_flag) {
    win->clear();
    win->makeicolor(20, 255, 255, 255);
    win->set_color_index(20);
  }

  float delta = delta_step;

  for (i = 0; i < steps; i++) {
    for (j = 0; j < steps * vf->getaspect(); j++) {
      float jx = jitter * (drand48() - 0.5);
      float jy = jitter * (drand48() - 0.5);
      float x = (i + 0.5 + jx) / steps;
      float y = (j + 0.5 + jy) / steps;
      float blen = vis_get_birth_length(x, y);
      st = new Streamline(vf, x, y, blen, delta);
      bundle->add_line(st);
      if (graphics_flag)
        st->draw(win);
    }
  }

  if (graphics_flag)
    win->flush();
}


/******************************************************************************
Place random lines that keep improving the value of a lowpass image.
******************************************************************************/

void good_random_lines()
{
  if (graphics_flag) {
    win->clear();
    win->makeicolor(BLACK, 0, 0, 0);
    win->makeicolor(RED, 255, 0, 0);
    win->makeicolor(WHITE, 255, 255, 255);
    win->set_color_index(WHITE);
    win->left_down(stop_events);
  }

  int xs = vis_get_lowpass_xsize();
  int ys = vis_get_lowpass_ysize();

  /* create a lowpass image, to be used with a radial cubic filter */
  low = new Lowpass(xs, ys, radius_lowpass, target_lowpass);

  float quality = low->current_quality();
  float delta = delta_step;

  /* randomly place streamlines */

  for (int i = 0; i < random_place_count; i++) {

    /* create a random streamline */
    float x = drand48();
    float y = drand48();
    float blen = vis_get_birth_length(x, y);
    Streamline *st = new Streamline(vf, x, y, blen, delta);

    /* see whether it increases the quality of the low-pass image */

    float new_quality = low->new_quality(st);

    /* if so, add it to image */

    if (quality >= new_quality) {
      low->add_line(st);
      quality = new_quality;
      if (graphics_flag) {
        win->set_color_index(WHITE);
        st->draw(win);
      }
    }

#if 1
    if (i % 100 == 0 && graphics_flag)
      low->draw(win2);
#endif
  }

  if (graphics_flag)
    low->draw(win2);

  /* get the newly created bundle of streamlines */
  bundle = low->bundle->copy();

  /* delete lowpass image */
  delete low;
}


/******************************************************************************
See if we want to birth new streamlines.

Exit:
  returns 1 if there were births, 0 if not
******************************************************************************/

int streamline_birth(Lowpass *low)
{
  int i;
  float quality;
  int had_births = 0;

  if (verbose_flag) {
    printf(".");
    fflush(stdout);
  }

  float delta = delta_step;

  int xs_blur = (int) (birth_blur * vis_get_lowpass_xsize());
  int ys_blur = (int) (birth_blur * vis_get_lowpass_ysize());

  Lowpass *blur = new Lowpass(xs_blur, ys_blur, radius_lowpass, target_lowpass);

  /* create a copy of the current bundle */
  Bundle *temp_bundle = low->bundle->copy();

  /* add all the streamlines to this lowpass image */
  for (i = 0; i < temp_bundle->num_lines; i++) {
    quality = blur->new_quality(temp_bundle->get_line(i));
    blur->add_line(temp_bundle->get_line(i));
  }

  /* find places in this image that are too sparse, and try to */
  /* birth a new streamline at these places */

  Dissolve dissolve(xs_blur, ys_blur);

  int count = 0;
  quality = low->current_quality();

  for (i = 0; i < xs_blur * ys_blur; i++) {

    /* get new pseudo-random position in blur image */
    int a, b;
    dissolve.new_position(a, b);
    float x = (a + 0.5) / xs_blur;
    float y = (b + 0.5) / ys_blur * vf->getaspect();

    if (blur->birth_test(x, y, birth_thresh)) {

      Streamline *birth_st;
      float blen = vis_get_birth_length(x, y);
      birth_st = new Streamline(vf, x, y, blen, delta);

      float new_quality = low->new_quality(birth_st);

      /* add streamline if it improves the quality */

      if (new_quality <= quality) {
        quality = blur->new_quality(birth_st);
        blur->add_line(birth_st);
        quality = low->new_quality(birth_st);
        low->add_line(birth_st);
        had_births = 1;

        add_streamline(birth_st);
        quality = new_quality;

        if (animation_flag) {
          birth_st->anim_index = anim_index++;
          *anim_file << "birth " << birth_st->anim_index << " "
                     << x << " " << y << " " << blen
                     << endl;
        }

        count++;
      } else
        delete birth_st;
    }
  }

  if (count && verbose_flag) {
    printf("(%d)", count);
    fflush(stdout);
  }

  /* free up memory */

  for (i = 0; i < temp_bundle->num_lines; i++)
    delete temp_bundle->get_line(i);
  delete temp_bundle;
  delete blur;

  return (had_births);
}

static Dissolve *dissolve;
static float birth_delta;
static int xs_blur, ys_blur;

/******************************************************************************
Set things up for birthing new streamlines.
******************************************************************************/

void streamline_birth_init(Lowpass *low)
{
  int i;

  birth_delta = delta_step;

  xs_blur = (int) (birth_blur * vis_get_lowpass_xsize());
  ys_blur = (int) (birth_blur * vis_get_lowpass_ysize());

  /* find places in this image that are too sparse, and try to */
  /* birth a new streamline at these places */

  dissolve = new Dissolve(xs_blur, ys_blur);
}


/******************************************************************************
See if we want to birth a streamline.

Exit:
  returns 1 if there was a birth, 0 if not
******************************************************************************/

int streamline_birth_trial(Lowpass *low)
{
  float quality = low->current_quality();

  /* get new pseudo-random position in blur image */
  int a, b;
  dissolve->new_position(a, b);
  float x = (a + 0.5) / xs_blur;
  float y = (b + 0.5) / ys_blur * vf->getaspect();

  if (low->birth_test(x, y, birth_thresh)) {

    Streamline *birth_st;
    float blen = vis_get_birth_length(x, y);
    birth_st = new Streamline(vf, x, y, blen, birth_delta);

    float new_quality = low->new_quality(birth_st);

    /* add streamline if it improves the quality */

    if (new_quality <= quality) {

      quality = low->new_quality(birth_st);
      low->add_line(birth_st);

      add_streamline(birth_st);
      quality = new_quality;

      if (animation_flag) {
        birth_st->anim_index = anim_index++;
        *anim_file << "birth " << birth_st->anim_index << " "
                   << x << " " << y << " " << blen
                   << endl;
      }

      return (1);  /* signal that we got a birth */
    } else
      delete birth_st;
  }

  /* there was no birth if we get here, so signal this */
  return (0);
}


/******************************************************************************
Make a directed change to a streamline.  Accept the change if it improves the
overall quality.

Entry:
  num     - index to streamline that we want to change
  low     - lowpass filtered version of streamlines
  quality - current quality of image
  change  - what kind of change to make to the streamline

Exit:
  quality - new quality, if it has changed
  returns 1 if we had to delete the streamline, 0 otherwise
******************************************************************************/

int make_streamline_move(
        int num,
        Lowpass *low,
        float &quality,
        unsigned long int change
)
{
  /* get the streamline to try changing */

  Streamline *st = low->bundle->get_line(num);

  /* get info about the selected streamline */

  float x, y;
  st->get_origin(x, y);

  float len1_orig, len2_orig;
  st->get_lengths(len1_orig, len2_orig);

  /* if deleting this streamline improves the quality, get rid */
  /* of it and return */

  low->delete_line(st);
  float new_quality = low->current_quality();

  if (new_quality <= quality) {

    if (animation_flag) {
      *anim_file << "change " << st->anim_index << " "
                 << x << " " << y << " 0.0"
                 << endl;
    }

    remove_streamline(st);
    delete st;
    quality = new_quality;
    return (1);
  }

#if 0
  /* maybe change tapering of intensity at ends */

  if ((taper_max > 0) && (drand48() < 0.5)) {

    float head,tail;
    st->get_taper (head, tail);

    if (drand48() < 0.5) {
      head += taper_delta * 2 * (drand48() - 0.5);
      if (head < 0) head = 0;
      if (head > taper_max) head = taper_max;
    }
    else {
      tail += taper_delta * 2 * (drand48() - 0.5);
      if (tail < 0) tail = 0;
      if (tail > taper_max) tail = taper_max;
    }

    if (verbose_flag)
      printf ("head tail: %f %f\n", head, tail);

    set_taper (head, tail);

    change = 0;

  }
#endif

  /* pick a new position */

  if (change & MOVE_CHANGE) {
    x += vis_get_delta_move(x, y) * (drand48() - 0.5);
    y += vis_get_delta_move(x, y) * (drand48() - 0.5);
  }

#if 0
  if (change & MOVE_CHANGE) {
    x += rmove * (drand48() - 0.5);
    y += rmove * (drand48() - 0.5);
  }
#endif

  /* find which end should be changed */

  int which_end;

  if (change & HEAD_CHANGE) {
    which_end = HEAD;
  } else if (change & TAIL_CHANGE) {
    which_end = TAIL;
  } else {
    if (drand48() < 0.5)
      which_end = HEAD;
    else
      which_end = TAIL;
  }

  /* pick new lengths on either end, making sure they are not negative */

  float len1 = len1_orig;
  float len2 = len2_orig;

  float xx, yy;
  float delta_length1, delta_length2;

  st->get_head(xx, yy);
  delta_length1 = vis_get_delta_length(xx, yy);
  st->get_tail(xx, yy);
  delta_length2 = vis_get_delta_length(xx, yy);

  float taper_head, taper_tail;
  st->get_taper(taper_head, taper_tail);

#if 1
  /* maybe change tapering of intensity at ends */

  if ((taper_max > 0) && (drand48() < 0.5)) {

    if (!st->head_clipped && drand48() < 0.5) {

      float pivot = drand48();
      float taper_len = taper_head * (len1_orig + len2_orig);
      float taper_anchor = len1_orig - pivot * taper_len;
      float dt;
      if (taper_len == 0)
        dt = taper_delta * drand48();
      else
        dt = taper_delta * 2 * (drand48() - 0.5);
      taper_len += dt;

      if (taper_len / (len1_orig + len2_orig) > taper_max)
        taper_len = taper_max * (len1_orig + len2_orig);
      else if (taper_len < 0)
        taper_len = 0;

      taper_head = taper_len / (len1_orig + len2_orig);
      len1 = taper_anchor + pivot * taper_len;
      len2 = len2_orig;
    } else if (!st->tail_clipped) {

      float pivot = drand48();
      float taper_len = taper_tail * (len1_orig + len2_orig);
      float taper_anchor = len2_orig - pivot * taper_len;
      float dt;
      if (taper_len == 0)
        dt = taper_delta * drand48();
      else
        dt = taper_delta * 2 * (drand48() - 0.5);
      taper_len += dt;

      if (taper_len / (len1_orig + len2_orig) > taper_max)
        taper_len = taper_max * (len1_orig + len2_orig);
      else if (taper_len < 0)
        taper_len = 0;

      taper_tail = taper_len / (len1_orig + len2_orig);
      len2 = taper_anchor + pivot * taper_len;
      len1 = len1_orig;
    }

//    printf ("head tail: %f %f\n", taper_head, taper_tail);

    change = 0;

  }
#endif

#if 0
  /* maybe change tapering of intensity at ends */

  if ((taper_max > 0) && (drand48() < 0.5)) {

    if (drand48() < 0.5) {

      float taper_len = taper_head * (len1_orig + len2_orig);
      float pivot = len1_orig - 0.5 * taper_len;
      float dt = taper_delta * 2 * (drand48() - 0.5);
      taper_len += dt;

      if (taper_len / (len1_orig + len2_orig) > taper_max)
        taper_len = taper_max * (len1_orig + len2_orig);
      else if (taper_len < 0)
        taper_len = 0;

      taper_head = taper_len / (len1_orig + len2_orig);
      len1 = pivot + 0.5 * taper_len;
      len2 = len2_orig;
    }
    else {

      float taper_len = taper_tail * (len1_orig + len2_orig);
      float pivot = len2_orig - 0.5 * taper_len;
      float dt = taper_delta * 2 * (drand48() - 0.5);
      taper_len += dt;

      if (taper_len / (len1_orig + len2_orig) > taper_max)
        taper_len = taper_max * (len1_orig + len2_orig);
      else if (taper_len < 0)
        taper_len = 0;

      taper_tail = taper_len / (len1_orig + len2_orig);
      len2 = pivot + 0.5 * taper_len;
      len1 = len1_orig;
    }

//    printf ("head tail: %f %f\n", taper_head, taper_tail);

    change = 0;

  }
#endif

  if (change & LONG_BOTH) {
    len1 = len1_orig + delta_length1 * drand48();
    len2 = len2_orig + delta_length2 * drand48();
  }

  if (change & LONG_ONE) {
    if (which_end == HEAD)
      len1 = len1_orig + delta_length1 * drand48();
    else
      len2 = len2_orig + delta_length2 * drand48();
  }

  if (change & SHORT_BOTH) {
    len1 = len1_orig - delta_length1 * drand48();
    if (len1 < 0)
      len1 = delta_length1 * drand48();

    len2 = len2_orig - delta_length2 * drand48();
    if (len2 < 0)
      len2 = delta_length2 * drand48();
  }

  if (change & SHORT_ONE) {
    if (which_end == HEAD) {
      len1 = len1_orig - delta_length1 * drand48();
      if (len1 < 0)
        len1 = delta_length1 * drand48();
    } else {
      len2 = len2_orig - delta_length2 * drand48();
      if (len2 < 0)
        len2 = delta_length2 * drand48();
    }
  }

  if (change & ALL_LEN) {
    len1 = len1_orig + delta_length1 * 2 * (drand48() - 0.5);
    if (len1 < 0)
      len1 = delta_length1 * drand48();

    len2 = len2_orig + delta_length2 * 2 * (drand48() - 0.5);
    if (len2 < 0)
      len2 = delta_length2 * drand48();
  }

  /* clamp position to screen */

#if 0
  float tiny = 0.000001;
  if (x < tiny) x = tiny;
  if (x > 1 - tiny) x = 1 - tiny;
  if (y < tiny) y = tiny;
  if (y > 1 - tiny) y = 1 - tiny;
#endif

  clamp_to_screen(x, y, vf->getaspect());

  float delta = delta_step;
  set_taper(taper_head, taper_tail);
  Streamline *new_st = new Streamline(vf, x, y, len1, len2, delta);
  set_taper(0.0, 0.0);
  new_quality = low->new_quality(new_st);

  /* see if this new one is better than the old one */

  if (new_quality <= quality) {

    if (animation_flag) {
      new_st->anim_index = st->anim_index;
      *anim_file << "change " << st->anim_index << " "
                 << x << " " << y << " " << (len1 + len2)
                 << endl;
    }

    low->add_line(new_st);  /* add new one */
    remove_streamline(st);
    add_streamline(new_st);
    delete st;
    quality = new_quality;
  } else {
    low->add_line(st);      /* add back old one */
    delete new_st;
  }

  return (0);
}


/******************************************************************************
Change around a streamline, and accept the change if it improves the overall
quality.

Entry:
  num     - index to streamline that we want to change
  low     - lowpass filtered version of streamlines
  quality - current quality of image

Exit:
  returns 1 if we had to delete the streamline, 0 otherwise
******************************************************************************/

int move_streamline(int num, Lowpass *low, float &quality)
{
  int result;
  unsigned long int change = 0;

  /* don't try to move the streamline if it is frozen */

  if (low->bundle->get_line(num)->frozen)
    return (0);

  /* select whether to change the length, position, or both */

  float pick = drand48();

  if (pick < odds_move) {
    change |= MOVE_CHANGE;
  } else if (pick < odds_move + odds_len) {
    change |= LEN_CHANGE;
  } else {
    change |= MOVE_CHANGE;
    change |= LEN_CHANGE;
  }

  /* maybe select the kind of length change */

  if (change & LEN_CHANGE) {
    pick = drand48();

    if (pick < odds_short_one)
      change |= SHORT_ONE;
    else if (pick < odds_short_one + odds_long_one)
      change |= LONG_ONE;
    else if (pick < odds_short_one + odds_long_one + odds_short_both)
      change |= SHORT_BOTH;
    else if (pick < odds_short_one + odds_long_one + odds_short_both + odds_long_both)
      change |= LONG_BOTH;
    else
      change |= ALL_LEN;
  }

  result = make_streamline_move(num, low, quality, change);

  return (result);
}

static float graph_x;
static float graph_y;
static float graph_ymax;
static float graph_x_delta;

/******************************************************************************
Initialize things for graphing the quality.

Entry:
  q - initial quality
******************************************************************************/

void init_graph_quality(float q)
{
  graph_x = 0;
  graph_ymax = q * 1.4 * xsize / (float) ysize;
  graph_y = q / graph_ymax;
  graph_x_delta = 1.0 / xsize;
}


/******************************************************************************
Draw new part of the graph of quality.

Entry:
  q   - current quality
  win - window to draw in
******************************************************************************/

void draw_graph_quality(float q, Window2d *win)
{
  float graph_ynew = q / graph_ymax;
  float graph_xnew = graph_x + graph_x_delta;

  if (graph_x > 0) {
    win->set_color_index(WHITE);
    win->line(graph_x, graph_y, graph_xnew, graph_ynew);
  }

  graph_x = graph_xnew;
  if (graph_x >= 1.0)
    graph_x = 0;

  graph_y = graph_ynew;
}


/******************************************************************************
Specify that we joined streamlines by drawing a mark above the graph.

Entry:
  win - window to draw in
******************************************************************************/

void graph_show_join(Window2d *win)
{
  float x = graph_x;
  float y1 = graph_y + graph_x_delta * 4;
  float y2 = graph_y + graph_x_delta * 6;
  win->set_color_index(RED);
  win->line(x, y1, x, y2);
}


/******************************************************************************
Specify that we had births by drawing a mark above the graph.

Entry:
  win - window to draw in
******************************************************************************/

void graph_show_birth(Window2d *win)
{
  float x = graph_x;
  float y1 = graph_y + graph_x_delta * 6;
  float y2 = graph_y + graph_x_delta * 8;
  win->set_color_index(GREEN);
  win->line(x, y1, x, y2);
}


/******************************************************************************
Freeze the current bundle of streamlines.
******************************************************************************/

void freeze_bundle()
{
  for (int i = 0; i < bundle->num_lines; i++)
    bundle->get_line(i)->frozen = 1;
}


/******************************************************************************
Unfreeze the current bundle of streamlines.
******************************************************************************/

void unfreeze_bundle()
{
  for (int i = 0; i < bundle->num_lines; i++)
    bundle->get_line(i)->frozen = 0;
}


/******************************************************************************
Draw the streamlines so as to show their relative qualities.
******************************************************************************/

void draw_streamline_quality(Window2d *win, int x, int y)
{
  int i;
  Bundle *bundle = low->bundle;

  win2->gray_ramp();
  win2->clear();

  /* examine all streamlines to find their quality */

  float *qualities = new float[bundle->num_lines];

  for (i = 0; i < bundle->num_lines; i++) {
    Streamline *st = bundle->get_line(i);
    if (show_samples)
      low->streamline_quality(st, sample_radius, sample_number,
                              sample_endpoint_distance, win2);
    else
      low->streamline_quality(st, sample_radius, sample_number,
                              sample_endpoint_distance, NULL);
    qualities[i] = st->get_quality();
  }

  /* find min and max quality */

  float min = qualities[0];
  float max = qualities[0];
  for (i = 0; i < bundle->num_lines; i++) {
    if (qualities[i] < min)
      min = qualities[i];
    if (qualities[i] > max)
      max = qualities[i];
  }

  if (min == max)
    min = max - 1;

  /* draw the streamlines according to quality */

  for (i = 0; i < bundle->num_lines; i++) {
    Streamline *st = bundle->get_line(i);
    float t = (qualities[i] - min) / (max - min);
    win2->set_color_index((int) (255 * t));
    st->draw_helper(win2, 0);
  }

  win2->flush();
}


/******************************************************************************
Initialize the qualities of the streamlines.
******************************************************************************/

void initialize_streamline_quality()
{
  int i;
  Bundle *bundle = low->bundle;

  /* determine the streamline qualities */
  for (i = 0; i < bundle->num_lines; i++) {
    Streamline *st = bundle->get_line(i);
    low->streamline_quality(st, sample_radius, sample_number,
                            sample_endpoint_distance, NULL);
  }

  /* sort the qualities */
  bundle->quality_sort();

  if (verbose_flag)
    printf("streamlines have been sorted by priority\n");
}


/******************************************************************************
See which streamlines have poor quality and try to improve one of them.
******************************************************************************/

void examine_streamline_quality(float &quality)
{
  int index;
  Bundle *bundle = low->bundle;

  /* pick a streamline that needs improving */

#if 1

  int poor_index = 0;
  poor_index = (int) (0.125 * bundle->num_lines * drand48());

#endif

#if 0

  int poor_index = 0;
  while (poor_index < bundle->num_lines && 0.25 > drand48())
    poor_index++;

  if (poor_index == bundle->num_lines)
    poor_index = (int) (bundle->num_lines * drand48());

#endif

  if (bundle->get_line(poor_index)->frozen == 1)
    return;

  /* find out how it should be changed */

  unsigned long int change = 0;
  change = low->recommend_change(bundle->get_line(poor_index));

  /* try out the change */

  quality = low->current_quality();
  int result = make_streamline_move(poor_index, low, quality, change);

  /* re-evaluate some streamline qualities at random */

  for (int i = 0; i < 5; i++) {
    index = (int) (bundle->num_lines * drand48());
    Streamline *st = bundle->get_line(index);
    low->streamline_quality(st, sample_radius, sample_number,
                            sample_endpoint_distance, NULL);
  }

  /* do some sorting passes */
  bundle->sort_passes(2);
}


/******************************************************************************
Improve the positions of the lines.
******************************************************************************/

void improve_lines(int num)
{
  int i;

  if (graphics_flag) {
    win->clear();
    win->gray_ramp();
    win->set_color_index(WHITE);
    win->left_down(stop_events);
    win->middle_down(draw_lowimage);
    win->right_down(draw_streamline_quality);
  }

  /* maybe start writing an animation file */
  if (animation_flag) {
    anim_file = new ofstream("anim.txt");
    anim_index = 0;
  }

  int xs = vis_get_lowpass_xsize();
  int ys = vis_get_lowpass_ysize();

  if (verbose_flag)
    printf("lowpass size = %d %d\n", xs, ys);

  /* create a lowpass image, to be used with a radial cubic filter */
  low = new Lowpass(xs, ys, radius_lowpass, target_lowpass);

  /* add all the streamlines to this lowpass image */
  for (i = 0; i < bundle->num_lines; i++) {
    float quality = low->new_quality(bundle->get_line(i));
    low->add_line(bundle->get_line(i));
  }

  float quality = low->current_quality();
  float delta = delta_step;

  /* make table to use for joining endpoints */
  RepelTable *repel;

  float join_dist = vis_get_max_join_distance();
  if (join_dist > 0.0) {
    repel = new RepelTable(vf, join_dist);
    float radius = radius_lowpass / vis_get_lowpass_xsize();
    if (verbose_flag) {
      printf("max join distance = %f\n", join_dist);
      printf("radius = %f\n", radius);
    }
  }

  if (graphics_flag) {
    win2->clear();
    win2->makeicolor(WHITE, 255, 255, 255);
    win2->set_color_index(WHITE);

    win->set_color_index(WHITE);
    low->draw_lines(win);
  }

  if (verbose_flag) {
    printf("\n");
    printf("%d streamlines\n", low->bundle->num_lines);
    printf("\n");
  }

  quality = low->current_quality();

  /* estimate the quality of the streamlines */
  initialize_streamline_quality();

  /* get ready for streamline births */
  streamline_birth_init(low);

  /* see if we want some new streamlines to be born */
  if (generation > 0) {
    for (i = 0; i < xs_blur * ys_blur; i++)
      streamline_birth_trial(low);
    quality = low->current_quality();
  }

  if (graph_the_quality) {
    win2->clear();
    win2->makeicolor(RED, 255, 0, 0);
    win2->makeicolor(GREEN, 0, 255, 0);
    win2->makeicolor(WHITE, 255, 255, 255);
    win2->set_color_index(WHITE);
    init_graph_quality(quality);
  }

  /* try to improve the positions of the streamlines */

  keep_reading_events = 1;
  float last_quality = quality;

  for (int k = 0; k < num; k++) {

    /* pick a random streamline (making sure it isn't frozen) */

    int pick;
    do {
      pick = (int) floor(drand48() * low->bundle->num_lines);
    } while (low->bundle->get_line(pick)->frozen == 1);

    /* move it around one or more times */

    if (low->bundle->num_lines > 0)
      for (i = 0; i < move_times; i++)
        if (move_streamline(pick, low, quality))
          break;

    /* check the events and maybe break loop */
    check_events();
    if (!keep_reading_events) {
      if (verbose_flag)
        printf("\nstopped after %d iterations\n", k);

      break;
    }

    /* see if we want some new streamlines to be born */
    if (generation > 0) {
      int had_birth = streamline_birth_trial(low);
      quality = low->current_quality();
      if (had_birth)
        graph_show_birth(win2);
    }

    /* look for endpoints to join */

    if (join_dist > 0.0) {
      int debug_it = 0;
      int did_join = repel->identify_neighbors(low->bundle, win, vf,
                                               low, quality, delta, debug_it);
      if (graph_the_quality && did_join)
        graph_show_join(win2);
    }

    /* look for streamlines that have poor quality, and try to change them */
    if (quality_guide)
      examine_streamline_quality(quality);

    if (graph_the_quality)
      draw_graph_quality(quality, win2);

//    if (last_quality < quality * 0.95)
//      printf ("big jump, iteration %d\n", k);

    last_quality = quality;
  }

  /* get the new bundle of streamlines */
  bundle = low->bundle->copy();

  /* delete the lowpass image */
  delete low;

  /* end the animation, if necessary */
  if (animation_flag)
    delete anim_file;
}


/******************************************************************************
Improve the positions of the lines.
******************************************************************************/

void old_improve_lines(int num)
{
  int i;

  if (graphics_flag) {
    win->clear();
    win->gray_ramp();
    win->set_color_index(WHITE);
    win->left_down(stop_events);
    win->middle_down(draw_lowimage);
    win->right_down(draw_streamline_quality);
  }

  /* maybe start writing an animation file */
  if (animation_flag) {
    anim_file = new ofstream("anim.txt");
    anim_index = 0;
  }

  int xs = vis_get_lowpass_xsize();
  int ys = vis_get_lowpass_ysize();

  if (verbose_flag)
    printf("lowpass size = %d %d\n", xs, ys);

  /* create a lowpass image, to be used with a radial cubic filter */
  low = new Lowpass(xs, ys, radius_lowpass, target_lowpass);

  /* add all the streamlines to this lowpass image */
  for (i = 0; i < bundle->num_lines; i++) {
    float quality = low->new_quality(bundle->get_line(i));
    low->add_line(bundle->get_line(i));
  }

  float quality = low->current_quality();
  float delta = delta_step;

  /* make table to use for joining endpoints */
  RepelTable *repel;

  float join_dist = vis_get_max_join_distance();
  if (join_dist > 0.0) {
    repel = new RepelTable(vf, join_dist);
    float radius = radius_lowpass / vis_get_lowpass_xsize();
    if (verbose_flag) {
      printf("max join distance = %f\n", join_dist);
      printf("radius = %f\n", radius);
    }
  }

#if 0
  if (join_factor > 0.0) {
    float radius = join_factor * radius_lowpass / vis_get_lowpass_xsize();
    repel = new RepelTable(vf, radius);
  }
#endif

  if (graphics_flag) {
    win2->clear();
    win2->makeicolor(WHITE, 255, 255, 255);
    win2->set_color_index(WHITE);

    win->set_color_index(WHITE);
    low->draw_lines(win);
  }

  if (verbose_flag) {
    printf("\n");
    printf("%d streamlines\n", low->bundle->num_lines);
    printf("\n");
  }

  quality = low->current_quality();

  /* estimate the quality of the streamlines */
  initialize_streamline_quality();

  /* see if we want some new streamlines to be born */
  if (generation > 0) {
    (void) streamline_birth(low);
    quality = low->current_quality();
  }

  if (graph_the_quality) {
    win2->clear();
    win2->makeicolor(RED, 255, 0, 0);
    win2->makeicolor(GREEN, 0, 255, 0);
    win2->makeicolor(WHITE, 255, 255, 255);
    win2->set_color_index(WHITE);
    init_graph_quality(quality);
  }

  /* try to improve the positions of the streamlines */

  keep_reading_events = 1;
  float last_quality = quality;

  for (int k = 0; k < num; k++) {

    /* pick a random streamline (making sure it isn't frozen) */

    int pick;
    do {
      pick = (int) floor(drand48() * low->bundle->num_lines);
    } while (low->bundle->get_line(pick)->frozen == 1);

    /* move it around one or more times */

    if (low->bundle->num_lines > 0)
      for (i = 0; i < move_times; i++)
        if (move_streamline(pick, low, quality))
          break;

    /* check the events and maybe break loop */
    check_events();
    if (!keep_reading_events) {
      if (verbose_flag)
        printf("\nstopped after %d iterations\n", k);

      break;
    }

    /* see if we want some new streamlines to be born */
    if (generation > 0 && k > 0 && k % generation == 0) {
      int had_birth = streamline_birth(low);
      quality = low->current_quality();
      if (had_birth)
        graph_show_birth(win2);
    }

    /* look for endpoints to join */

    if (join_dist > 0.0) {
      int debug_it = 0;
      int did_join = repel->identify_neighbors(low->bundle, win, vf,
                                               low, quality, delta, debug_it);
      if (graph_the_quality && did_join)
        graph_show_join(win2);
    }

#if 0
    if (join_factor > 0.0) {
      int debug_it = 0;
      int did_join = repel->identify_neighbors (low->bundle, win, vf,
                                                low, quality, delta, debug_it);
      if (graph_the_quality && did_join)
        graph_show_join (win2);
    }
#endif

    /* look for streamlines that have poor quality, and try to change them */
    if (quality_guide)
      examine_streamline_quality(quality);

    if (graph_the_quality)
      draw_graph_quality(quality, win2);

//    if (last_quality < quality * 0.95)
//      printf ("big jump, iteration %d\n", k);

    last_quality = quality;
  }

  /* get the new bundle of streamlines */
  bundle = low->bundle->copy();

  /* delete the lowpass image */
  delete low;

  /* end the animation, if necessary */
  if (animation_flag)
    delete anim_file;
}


/******************************************************************************
Optimize the intensity along the streamlines (allow their intensity to fall
off near the endpoints).
******************************************************************************/

void taper_optimize()
{
  taper_max = 0.3;
  taper_delta = 0.1;

  rmove = 0;
  vis_set_join_factor(0);

  improve_lines(999999);

  set_taper(0.0, 0.0);
}


/******************************************************************************
Optimize the placement of streamlines.
******************************************************************************/

void optimize_streamlines(float sep_target)
{
  float sep;
  float len;
  int gen;

  /* clear out old sets of streamlines */

  bundle = new Bundle();

  /* set various parameters */

  quality_guide = 1;
  birth_thresh = 0.02;
  birth_blur = 0.7;
  rmove = 0.01;
  taper_max = 0;
  taper_delta = 0;

  vis_set_join_factor(1);
  vis_set_delta_length(1.125);

  target_lowpass = 1.0;

  /* determine correct separation, generations and birth lengths */

  sep = sep_target;
  while (sep < 0.04)
    sep *= 2;

  gen = 100;
  len = 2.5 * sep;

  while (fabs(sep - sep_target) > 0.0001) {

    vis_set_separation(sep);
    vis_set_birth_length(len);
    generation = gen;

    gen *= 2;
    len *= 0.5;
    sep *= 0.5;
  }

  improve_lines(999999);
}


/******************************************************************************
Cascade several calls to improve_lines.
******************************************************************************/

void cascaded_improve(float sep_target)
{
  float sep;
  float len;
  int gen;

  /* clear out old sets of streamlines */

  bundle = new Bundle();

  /* set various parameters */

  quality_guide = 1;
  birth_thresh = 0.02;
  birth_blur = 0.7;
  rmove = 0.01;
  taper_max = 0;
  taper_delta = 0;

  vis_set_join_factor(1);
  vis_set_delta_length(1.125);

  target_lowpass = 1.0;

  /* determine correct separation, generations and birth lengths */

  sep = sep_target;
  while (sep < 0.04)
    sep *= 2;

  gen = 100;
  len = 2.5 * sep;

  while (fabs(sep - sep_target) > 0.0001) {

    vis_set_separation(sep);
    vis_set_birth_length(len);
    generation = gen;

    improve_lines(999999);

    gen *= 2;
    len *= 0.5;
    sep *= 0.5;
  }
}


/******************************************************************************
Draw evenly spaced tufts.
******************************************************************************/

void tufts(float sep, float len)
{
  bundle = new Bundle();

  quality_guide = 1;

  birth_thresh = 0.02;
  birth_blur = 0.7;
  generation = 400;
  rmove = 0.01;
  taper_max = 0;
  taper_delta = 0;

  vis_set_join_factor(0);
  vis_set_delta_length(0.0);

  vis_set_separation(sep);
  vis_set_birth_length(len);

  target_lowpass = 0.6;

  improve_lines(999999);
}


/******************************************************************************
Improve the positions of the lines by checking four directions of motion.
******************************************************************************/

void four_test()
{
  if (graphics_flag) {
    win->clear();
    win->makeicolor(BLACK, 0, 0, 0);
    win->makeicolor(RED, 255, 0, 0);
    win->makeicolor(WHITE, 255, 255, 255);
    win->set_color_index(WHITE);
    win->left_down(stop_events);
  }

  int xs = vis_get_lowpass_xsize();
  int ys = vis_get_lowpass_ysize();

  /* create a lowpass image, to be used with a radial cubic filter */
  low = new Lowpass(xs, ys, radius_lowpass, target_lowpass);

  /* add all the streamlines to this lowpass image */
  for (int i = 0; i < bundle->num_lines; i++)
    low->add_line(bundle->get_line(i));

  float delta = delta_step;

  if (graphics_flag) {
    win2->clear();
    win2->makeicolor(WHITE, 255, 255, 255);
    win2->set_color_index(WHITE);
    low->draw_lines(win);
    low->draw_lines(win2);
  }

  if (verbose_flag) {
    printf("\n");
    printf("%d streamlines\n", low->bundle->num_lines);
    printf("\n");
  }

  /* try to improve the positions of the streamlines */

  keep_reading_events = 1;

  while (1) {

    for (int i = 0; i < low->bundle->num_lines; i++) {

      /* get a streamline to improve */

      Streamline *st = low->bundle->get_line(i);
      float x, y;
      st->get_origin(x, y);
      float length = st->get_length();

      /* create two new streamlines in orthogonal directions */

      float eps = epsilon * radius_lowpass / vis_get_lowpass_xsize();

      Streamline *new_x0 = new Streamline(vf, x - eps, y, length, delta);
      Streamline *new_x1 = new Streamline(vf, x + eps, y, length, delta);
      Streamline *new_y0 = new Streamline(vf, x, y - eps, length, delta);
      Streamline *new_y1 = new Streamline(vf, x, y + eps, length, delta);

      /* see what the correct direction to move is */

      float quality = low->current_quality();

      low->delete_line(st);
      float x_quality = low->new_quality(new_x1) - low->new_quality(new_x0);
      float y_quality = low->new_quality(new_y1) - low->new_quality(new_y0);

      float dmove = rmove * radius_lowpass / vis_get_lowpass_xsize();
      x += dmove * x_quality;
      y += dmove * y_quality;
      Streamline *new_st = new Streamline(vf, x, y, length, delta);
      printf("q qx qy, x y: %f %f %f, %f %f ", quality, x_quality, y_quality, x, y);
      float new_quality = low->new_quality(new_st);

      if (new_quality <= quality) {
        low->add_line(new_st);
        if (graphics_flag) {
          win->set_color_index(BLACK);
          st->draw(win);
          win->set_color_index(WHITE);
          new_st->draw(win);
        }
//      delete st;
        printf("new\n");
      } else {
        low->add_line(st);      /* add back old one */
        delete new_st;
        printf("old\n");
      }

      delete new_x0;
      delete new_x1;
      delete new_y0;
      delete new_y1;

      /* check the events */
      check_events();

      /* maybe break out of loop */
      if (!keep_reading_events)
        goto here;
    }
  }

  here: /* for jumping out of loop */

  printf("done with four-test\n");
}


/******************************************************************************
Improve the positions of the lines point repulsion.
******************************************************************************/

void repel(int num)
{
  int i;

  if (graphics_flag) {
    win->clear();
    win->makeicolor(BLACK, 0, 0, 0);
    win->makeicolor(WHITE, 255, 255, 255);
    win->set_color_index(WHITE);
    win->left_down(stop_events);

    win2->clear();
    win2->makeicolor(WHITE, 255, 255, 255);
    win2->set_color_index(WHITE);
    bundle->draw(win2);
  }

  Bundle *old_bundle = bundle->copy();

  float radius = epsilon * radius_lowpass / vis_get_lowpass_xsize();

  float delta = delta_step;
  RepelTable repel(vf, radius);

  keep_reading_events = 1;
  for (int k = 0; k < num; k++) {

    /* do one step of repulsion */
    bundle = repel.repel(old_bundle, delta, rmove);

    /* draw new bundle */
    if (graphics_flag)
      for (i = 0; i < old_bundle->num_lines; i++) {
        win->set_color_index(BLACK);
        old_bundle->get_line(i)->draw(win);
        win->set_color_index(WHITE);
        bundle->get_line(i)->draw(win);
      }

    /* make old bundle the new one */
    for (i = 0; i < old_bundle->num_lines; i++)
      delete old_bundle->get_line(i);
    delete old_bundle;
    old_bundle = bundle;

    /* check the events */
    check_events();

    /* maybe break loop */
    if (!keep_reading_events) {
      if (verbose_flag) {
        printf("\n");
        printf("stopped after %d iterations\n", k);
      }
      break;
    }
  }
}


/******************************************************************************
Remove a streamline from the current collection for visualization.
******************************************************************************/

void remove_streamline(Streamline *st)
{

  /* maybe erase the old streamline */

  if (graphics_flag) {
//    win->set_color_index (BLACK);
    st->erase(win);
  }
}


/******************************************************************************
Add a streamline to the current collection for visualization.
******************************************************************************/

void add_streamline(Streamline *st)
{
  /* evaluate this streamline's quality */

  low->streamline_quality(st, sample_radius, sample_number,
                          sample_endpoint_distance, NULL);

  /* maybe draw the new streamline */

  if (graphics_flag) {
//    win->set_color_index (WHITE);
//    win->set_color_index ((int) (255 * st->get_intensity()));
    st->draw(win);
  }
}


/******************************************************************************
Draw the streamlines as dashed lines of a given length.  Either draw into
a window or write to a file.

Entry:
  win          - window to draw in
  filename     - file to write to
  bundle       - bundle of streamlines to draw with
  len          - length of dashes
  separation   - separation length between dashes
  arrow_length - length of arrowhead
  arrow_width  - width of arrowhead
******************************************************************************/

void draw_dashes(
        Window2d *win,
        char *filename,
        Bundle *bundle,
        float len,
        float separation,
        float arrow_length,
        float arrow_width
)
{
  int i;

  ofstream *file_out = NULL;

  if (filename)
    file_out = new ofstream(filename);

  if (filename) {
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
    *file_out << "/sw { 72 div size div setlinewidth } def" << endl;
  } else {
    win->clear();
    win->set_color_index(WHITE);
  }

  for (i = 0; i < bundle->num_lines; i++) {
    Streamline *st = bundle->get_line(i);
    if (vis_arrow_length_varies())
      st->variable_draw_dashed(win, file_out, separation,
                               arrow_length, arrow_width);
    else
      st->draw_dashed(win, file_out, len, separation,
                      arrow_length, arrow_width);
  }

  if (filename) {
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

  delete file_out;
}

const int ncharges = 2;
float charge[ncharges] = {0.01, -0.01};
float cx[ncharges] = {0.2, 0.8};
float cy[ncharges] = {0.5, 0.5};

/******************************************************************************
Calculate the magnitude of a dipole, or some variant of it.
******************************************************************************/

void dipole_magnitude()
{
  int i, j, k;
  int size = 64;
  float val;

  if (float_reg)
    delete float_reg;
  float_reg = new FloatImage(size, size);

  for (i = 0; i < size; i++)
    for (j = 0; j < size; j++) {

      float x = i / (float) (size - 1);
      float y = j / (float) (size - 1);
      float sum = 0;

      for (k = 0; k < ncharges; k++) {
        float dx = x - cx[k];
        float dy = y - cy[k];

        float r2 = dx * dx + dy * dy;
        float r = sqrt(r2);

//        val += (1 - r);

        float min = 0.03;
        if (r < min)
          val = 1.0 / min;
        else
          val = 1.0 / r;

        sum += val;
      }

      float_reg->pixel(i, j) = sum;
    }
}


/******************************************************************************
Write a high-resolution version of the lowpass image to a file.
******************************************************************************/

void make_lowpass_image(int size, char *filename)
{
  int i;
  float min = 0;
  float max = 2.0;
  int xs, ys;

  xs = size;
  ys = size;

  float s = size / (float) vis_get_lowpass_xsize();

  vis_set_minimum_blur(radius_lowpass * s);

  /* create a lowpass image, to be used with a radial cubic filter */
  low = new Lowpass(xs, ys, radius_lowpass, target_lowpass);

  /* add all the streamlines to this lowpass image */
  for (i = 0; i < bundle->num_lines; i++) {
    float quality = low->new_quality(bundle->get_line(i));
    low->add_line(bundle->get_line(i));
  }

  FloatImage *img = low->get_image_ptr();
  img->write_pgm(filename, min, max);

  vis_set_minimum_blur(radius_lowpass);
}

#if 0

/******************************************************************************
Improve the lowpass filter measure by doing intensity tapering on the
ends of the streamlines.
******************************************************************************/

void taper_improve()
{
  int i;
  Streamline *st,*new_st;
  Bundle *tbundle = new Bundle();
  float x,y;
  float extend_factor = 3.0;
  extern void set_grow_factor(float);

  float delta = delta_step;

  /* create new bunch of strealines that are longer */

  for (i = 0; i < bundle->num_lines; i++) {

    st = bundle->get_line(i);
    st->get_origin (x, y);

    float len = st->get_length();
    float grow = extend_factor * vis_get_separation (x, y);
    float len1 = len * 0.5 + grow;
    float len2 = len * 0.5 + grow;

    set_grow_factor (grow);
    new_st = new Streamline (vf, x, y, len1, len2, delta);

    new_st->tail_zero = 0.1;
    new_st->taper_tail = 0.2;
    new_st->taper_head = 0.8;
    new_st->head_zero = 0.9;

    tbundle->add_line (new_st);
  }

  /* draw the new lines */

  if (graphics_flag) {
    win->clear();
    win->gray_ramp();
    win->set_color_index (WHITE);
    win->left_down (stop_events);
    win->middle_down (draw_lowimage);
    win->right_down (draw_streamline_quality);
  }

  for (i = 0; i < tbundle->num_lines; i++) {
    st = tbundle->get_line(i);
    st->draw_for_taper (win);
  }

  /* create lowpass filtered version of lines */

  int xs = vis_get_lowpass_xsize();
  int ys = vis_get_lowpass_ysize();

  if (verbose_flag)
    printf ("lowpass size = %d %d\n", xs, ys);

  /* create a lowpass image, to be used with a radial cubic filter */
  low = new Lowpass (xs, ys, radius_lowpass, target_lowpass);

  /* add all the streamlines to this lowpass image */
  for (i = 0; i < tbundle->num_lines; i++) {
    float q = low->new_quality(tbundle->get_line(i));
    low->add_line (tbundle->get_line(i));
  }

  float quality = low->current_quality();

  keep_reading_events = 1;

  for (i = 0; i < 999999; i++) {

    check_events();
    if (!keep_reading_events) {

      if (verbose_flag)
        printf ("\nstopped after %d iterations\n", i);

      break;
    }

  }

}

#endif

/******************************************************************************
Interpret commands.  Many of these commands are undocumented, and users of
this command set should be prepared to read code in order to understand
the various functions.  Use at your own risk!
******************************************************************************/

void interpreter()
{
  int i, j;
  char filename[80];
  int grid_num = 40;
  float grid_dist = 0.3;

  START_CLI ("stplace", "cli")

    COMMAND ("vload filename") {
      get_parameter(filename);
      vf = new VectorField(filename);
      float_reg = vf->get_magnitude();
      vf->normalize();
    } COMMAND ("vsave filename (xsize)") {
      int res;
      get_parameter(filename);
      get_integer(&res);
      if (res == 0)
        res = vf->xsize;
      VectorField *vtemp = new VectorField(res, res);
      for (int i = 0; i < res; i++)
        for (int j = 0; j < res; j++) {
          float s = i / (float) res;
          float t = j / (float) res;
          vtemp->xval(i, j) = vf->xval(s, t);
          vtemp->yval(i, j) = vf->yval(s, t);
        }
      vtemp->write_file(filename);
      delete vtemp;
    } COMMAND ("vflip") {
      for (int i = 0; i < vf->xsize; i++)
        for (int j = 0; j < vf->ysize / 2; j++) {
          int jj = vf->ysize - j - 1;
          float tx = vf->xval(i, j);
          float ty = vf->yval(i, j);
          vf->xval(i, j) = vf->xval(i, jj);
          vf->yval(i, j) = vf->yval(i, jj);
          vf->xval(i, jj) = tx;
          vf->yval(i, jj) = ty;
        }
    } COMMAND ("hflip") {
      for (int i = 0; i < vf->xsize / 2; i++)
        for (int j = 0; j < vf->ysize; j++) {
          int ii = vf->xsize - i - 1;
          float tx = vf->xval(i, j);
          float ty = vf->yval(i, j);
          vf->xval(i, j) = vf->xval(ii, j);
          vf->yval(i, j) = vf->yval(ii, j);
          vf->xval(ii, j) = tx;
          vf->yval(ii, j) = ty;
        }
    } COMMAND ("xyswap") {
      for (int i = 0; i < vf->xsize; i++)
        for (int j = 0; j < vf->ysize; j++) {
          float temp = vf->xval(i, j);
          vf->xval(i, j) = vf->yval(i, j);
          vf->yval(i, j) = temp;
        }
    } COMMAND ("vscale  x y") {
      float xs, ys;
      get_real(&xs);
      get_real(&ys);
      for (int i = 0; i < vf->xsize; i++)
        for (int j = 0; j < vf->ysize; j++) {
          vf->xval(i, j) *= xs;
          vf->yval(i, j) *= ys;
        }
    } COMMAND ("vcut  xorg yorg size") {
      int xs, ys;
      int size;
      get_integer(&xs);
      get_integer(&ys);
      get_integer(&size);
      VectorField *vf2 = new VectorField(size, size);
      for (int i = 0; i < size; i++)
        for (int j = 0; j < size; j++) {
          vf2->xval(i, j) = vf->xval(i + xs, j + ys);
          vf2->yval(i, j) = vf->yval(i + xs, j + ys);
        }
      delete vf;
      vf = vf2;
    } COMMAND ("vrotate  degrees") {
      float theta;
      get_real(&theta);
      theta *= 3.1415926535 / 180.0;
      float cs = cos(theta);
      float sn = sin(theta);
      for (int i = 0; i < vf->xsize; i++)
        for (int j = 0; j < vf->ysize; j++) {
          float x = vf->xval(i, j);
          float y = vf->yval(i, j);
          vf->xval(i, j) = cs * x + sn * y;
          vf->yval(i, j) = -sn * x + cs * y;
        }
    } COMMAND ("vstretch  xmag") {

      float xmag;
      get_real(&xmag);

      int xs = vf->xsize;
      int xsize = (int) (xs * xmag);
      int ysize = xs;

      VectorField *vf2 = new VectorField(xsize, ysize);

      for (int i = 0; i < xsize; i++)
        for (int j = 0; j < ysize; j++) {
          float xx = i / (float) (xsize - 1);
          float yy = j / (float) (ysize - 1);
          yy *= xmag;
          if (yy > 1)
            yy = 0.99;
          vf2->xval(i, j) = vf->xval(xx, yy);
          vf2->yval(i, j) = vf->yval(xx, yy);
        }

      delete vf;
      vf = vf2;
    } COMMAND ("gradient") {
      if (vf)
        delete vf;
      vf = new VectorField(float_reg->getwidth(), float_reg->getheight());
      for (int i = 0; i < float_reg->getwidth(); i++)
        for (int j = 0; j < float_reg->getheight(); j++) {
          float gx, gy;
          float x = i / (float) float_reg->getwidth();
          float y = j / (float) float_reg->getheight();
          float_reg->gradient(x, y, &gx, &gy);
          vf->xval(i, j) = gx;
          vf->yval(i, j) = gy;
        }
      vf->normalize();
    } COMMAND ("streamline xorg yorg len1 len2 (taper_tail taper_head)") {
      float x, y, len1, len2;
      float tail, head;
      float delta = delta_step;
      Streamline *st;
      get_real(&x);
      get_real(&y);
      get_real(&len1);
      get_real(&len2);
      get_real(&tail);
      get_real(&head);
      if (tail != 0.0 || head != 0.0) {
        set_taper(head, tail);
        st = new Streamline(vf, x, y, len1, len2, delta);
      } else
        st = new Streamline(vf, x, y, len1, len2, delta);
      bundle->add_line(st);
    } COMMAND ("snap_arrows  num_across width length") {
      int num;
      float width, length;
      get_integer(&num);
      get_real(&width);
      get_real(&length);
      snap_arrows_to_streamlines(num, width, length, NULL, 0);
    } COMMAND ("hsnap_arrows  num_across width length") {
      int num;
      float width, length;
      get_integer(&num);
      get_real(&width);
      get_real(&length);
      snap_arrows_to_streamlines(num, width, length, NULL, 1);
    } COMMAND ("psnap_arrows  num_across width length file") {
      int num;
      float width, length;
      get_integer(&num);
      get_real(&width);
      get_real(&length);
      get_parameter(filename);
      snap_arrows_to_streamlines(num, width, length, filename, 0);
    } COMMAND ("hpsnap_arrows  num_across width length file") {
      int num;
      float width, length;
      get_integer(&num);
      get_real(&width);
      get_real(&length);
      get_parameter(filename);
      snap_arrows_to_streamlines(num, width, length, filename, 1);
    } COMMAND ("squares num_across") {
      int num;
      get_integer(&num);
      if (num != 0)
        grid_num = num;
      square_grid(grid_num);
    } COMMAND ("hexagons num_across") {
      int num;
      get_integer(&num);
      if (num != 0)
        grid_num = num;
      hexagonal_grid(grid_num);
    } COMMAND ("jitter value") {
      get_real(&jitter);
    } COMMAND ("lines") {
      lines_of_field();
    } COMMAND ("random_lines") {
      random_lines();
    } COMMAND ("good_lines") {
      good_random_lines();
    } COMMAND ("improve_lines  {num}") {
      int num;
      get_integer(&num);
      if (num == 0)
        improve_lines(999999);
      else
        improve_lines(num);
    } COMMAND ("optimize  separation") {
      float sep;
      get_real(&sep);
      optimize_streamlines(sep);
    } COMMAND ("cascade  separation") {
      float sep;
      get_real(&sep);
      cascaded_improve(sep);
    } COMMAND ("tufts  separation  length") {
      float sep, len;
      get_real(&sep);
      get_real(&len);
      tufts(sep, len);
    } COMMAND ("four_test") {
      four_test();
    } COMMAND ("repel") {
      int num;
      get_integer(&num);
      if (num == 0)
        repel(999999);
      else
        repel(num);
    } COMMAND ("better_lines") {
      good_random_lines();
      improve_lines(999999);
    } COMMAND ("target_lowpass  value") {
      get_real(&target_lowpass);
    } COMMAND ("separation  value") {
      float sep;
      get_real(&sep);
      vis_set_separation(sep);
    } COMMAND ("radius  value") {
      get_real(&radius_lowpass);
      vis_set_minimum_blur(radius_lowpass);
    } COMMAND ("length  value") {
      float slen;
      get_real(&slen);
      vis_set_birth_length(slen);
    } COMMAND ("vlength  off/on") {
      int flag;
      flag = get_boolean();
      vis_vary_birth_length(flag);
    } COMMAND ("dlen  value") {
      float dl;
      get_real(&dl);
      vis_set_delta_length(dl);
    } COMMAND ("jtimes  value") {
      get_integer(&move_times);
    } COMMAND ("death_length  value") {
      get_real(&death_length);
    } COMMAND ("birth_thresh  value") {
      get_real(&birth_thresh);
    } COMMAND ("birth_blur  value") {
      get_real(&birth_blur);
    } COMMAND ("generation  value") {
      get_integer(&generation);
    } COMMAND ("join_factor  value") {
      float join;
      get_real(&join);
      vis_set_join_factor(join);
    } COMMAND ("rmove  value") {
      get_real(&rmove);
    } COMMAND ("epsilon  value") {
      get_real(&epsilon);
    } COMMAND ("lsize  value") {
      get_integer(&lowpass_size);
      printf("lsize is an obsolete command!\n");
    } COMMAND ("rplace  value") {
      get_integer(&random_place_count);
    } COMMAND ("sradius  value") {
      get_real(&sample_radius);
    } COMMAND ("end_dist  value") {
      get_real(&sample_endpoint_distance);
    } COMMAND ("nsamples  value") {
      get_integer(&sample_number);
    } COMMAND ("show_samples  off/on") {
      show_samples = get_boolean();
    } COMMAND ("quality_guide  off/on") {
      quality_guide = get_boolean();
    } COMMAND ("freeze") {
      freeze_bundle();
    } COMMAND ("unfreeze") {
      unfreeze_bundle();
    } COMMAND ("arrow (0 = none, 1 = open, 2 = closed) length width") {
      int type;
      float length, width;
      get_integer(&type);
      get_real(&length);
      get_real(&width);
      set_arrow(type, length, width, (int) (length * xsize));
      arrow_type = type;
    } COMMAND ("reduce_length  start  end") {
      int start, end;
      get_integer(&start);
      get_integer(&end);
      set_reduction(start, end);
    } COMMAND ("intensity  value") {
      float val;
      get_real(&val);
      set_intensity(val);
    } COMMAND ("mtaper  max_val") {
      get_real(&taper_max);
    } COMMAND ("dtaper  delta") {
      get_real(&taper_delta);
    } COMMAND ("odds  move length both") {
      get_real(&odds_move);
      get_real(&odds_len);
      get_real(&odds_both);

      /* normalize the odds */
      float sum = odds_move + odds_len + odds_both;
      odds_move /= sum;
      odds_len /= sum;
      odds_both /= sum;

      printf("odds: %f %f %f\n", odds_move, odds_len, odds_both);
    } COMMAND ("lodds  all long_both short_both long_one short_one") {
      get_real(&odds_all_len);
      get_real(&odds_long_both);
      get_real(&odds_short_both);
      get_real(&odds_long_one);
      get_real(&odds_short_one);

      /* normalize the odds */
      float sum = odds_long_both + odds_short_both +
                  odds_long_one + odds_short_one + odds_all_len;
      odds_all_len /= sum;
      odds_long_both /= sum;
      odds_short_both /= sum;
      odds_long_one /= sum;
      odds_short_one /= sum;
    } COMMAND ("pgm filename (img_size)") {
      get_parameter(filename);
      int num;
      get_integer(&num);
      if (num == 0)
        bundle->write_pgm(filename, xsize, ysize);
      else
        bundle->write_pgm(filename, num, num);
    } COMMAND ("write_streamlines filename") {
      get_parameter(filename);
      if (taper_max > 0)
        bundle->write_ascii(filename, 1);
      else
        bundle->write_ascii(filename, 0);
    } COMMAND ("wlowpass   min max filename") {
      float min, max;
      get_real(&min);
      get_real(&max);
      get_parameter(filename);
      FloatImage *img = low->get_image_ptr();
      img->write_pgm(filename, min, max);
    } COMMAND ("lowpass  imgsize  filename") {
      int isize;
      get_integer(&isize);
      get_parameter(filename);
      make_lowpass_image(isize, filename);
    } COMMAND ("postscript filename") {
      get_parameter(filename);
      ofstream file_out(filename);
      write_postscript_start(&file_out);
      bundle->write_postscript(&file_out);
      write_postscript_end(&file_out);
    } COMMAND ("draw_streamlines") {
      win->clear();
      win->makeicolor(BLACK, 0, 0, 0);
      win->makeicolor(WHITE, 255, 255, 255);
      win->set_color_index(WHITE);
      bundle->draw(win);
    } COMMAND ("thickness  line_width") {
      float w;
      get_real(&w);
      set_line_thickness(w);
    } COMMAND ("render_streamlines  radius") {
      float rad;
      get_real(&rad);
      if (rad == 0.0) rad = 2.0;
      FloatImage *tmp = bundle->filtered_render(xsize, ysize, rad);
      win->gray_ramp();
      tmp->draw_clamped(win, 0.0, 1.1);
      delete tmp;
    } COMMAND ("dashes  length separation arrow_length arrow_width") {
      float length;
      float separation;
      float len, width;
      get_real(&length);
      get_real(&separation);
      get_real(&len);
      get_real(&width);
      draw_dashes(win2, NULL, bundle, length, separation, len, width);
    } COMMAND ("pdashes  length separation arrow_length arrow_width filename") {
      float length;
      float separation;
      float len, width;
      get_real(&length);
      get_real(&separation);
      get_real(&len);
      get_real(&width);
      get_parameter(filename);
      draw_dashes(win2, filename, bundle, length, separation, len, width);
    } COMMAND ("idashes  off/on (vary dash intensity?)") {
      vary_arrow_intensity = get_boolean();
    } COMMAND ("vorticity  grid_size") {
      int size;
      get_integer(&size);
      if (size == 0)
        size = 256;
      FloatImage *image = vf->get_vorticity(size, size);
      win2->gray_ramp();
      image->draw(win2);
    } COMMAND ("divergence  grid_size") {
      int size;
      get_integer(&size);
      if (size == 0)
        size = 256;
      FloatImage *image = vf->get_divergence(size, size);
      win2->gray_ramp();
      image->draw(win2);
    } COMMAND ("fload  filename") {
      get_parameter(filename);
      float_reg->read_file(filename);
    } COMMAND ("fsave  filename") {
      get_parameter(filename);
      float_reg->write_file(filename);
    } COMMAND ("floadpgm  filename") {
      get_parameter(filename);
      float_reg->read_pgm(filename);
    } COMMAND ("fsavepgm  filename") {
      get_parameter(filename);
      float_reg->write_pgm(filename, 0.0, 1.1);
    } COMMAND ("fdraw") {
      float_reg->draw(win);
    } COMMAND ("frender (radius)") {
      float rad;
      get_real(&rad);
      if (rad == 0.0)
        rad = 2.0;
      if (float_reg != NULL)
        delete float_reg;
      float_reg = bundle->filtered_render(xsize, ysize, rad);
      win->gray_ramp();
      float_reg->draw_clamped(win, 0.0, 1.1);
    } COMMAND ("fmap  min max") {
      float min, max;
      get_real(&min);
      get_real(&max);
      float_reg->remap(min, max);
    } COMMAND ("fflip") {
      for (i = 0; i < float_reg->getsize(); i++)
        float_reg->pixel(i) = -1 * float_reg->pixel(i);
    } COMMAND ("fbias  val") {
      float val;
      get_real(&val);
      float_reg->bias(val);
    } COMMAND ("fgain  val") {
      float val;
      get_real(&val);
      float_reg->gain(val);
    } COMMAND ("fseparation") {
      if (float_reg)
        vis_set_variable_separation(float_reg);
    } COMMAND ("fwidth  min max") {

      float min, max;
      get_real(&min);
      get_real(&max);

      if (min == max || max == 0.0)
        vis_set_draw_width(min);

      if (float_reg)
        vis_set_draw_width(float_reg, min, max);
      else
        fprintf(stderr, "No values in floating point register\n");
    } COMMAND ("flength  min max") {

      float min, max;
      get_real(&min);
      get_real(&max);

      if (min == max || max == 0.0)
        vis_set_arrow_length(min);

      if (float_reg)
        vis_set_arrow_length(float_reg, min, max);
      else
        fprintf(stderr, "No values in floating point register\n");
    } COMMAND ("fmagnitude filename") {
      get_parameter(filename);
      VectorField *vtemp = new VectorField(filename);
      float_reg = vtemp->get_magnitude();
      delete vtemp;
    } COMMAND ("fconstant") {
      if (float_reg)
        delete float_reg;
      float_reg = new FloatImage(20, 20);
      float_reg->setimage(1.0);
    } COMMAND ("framp") {
      int size = 20;
      FloatImage *img = new FloatImage(size, size);
      for (i = 0; i < size; i++)
        for (j = 0; j < size; j++)
          img->pixel(i, j) = i / (float) (size - 1);
      if (float_reg)
        delete float_reg;
      float_reg = img->copy();
    } COMMAND ("fnoise  size blur_steps") {
      int size;
      int steps;
      get_integer(&size);
      get_integer(&steps);

      if (float_reg)
        delete float_reg;
      float_reg = new FloatImage(size, size);

      for (i = 0; i < size; i++)
        for (j = 0; j < size; j++)
          float_reg->pixel(i, j) = drand48();

      float_reg->blur(steps);
    } COMMAND ("fcombine (vector-field and float_reg)") {
      for (j = 0; j < vf->ysize; j++)
        for (i = 0; i < vf->xsize; i++) {
          float s = i / (float) vf->xsize;
          float t = j / (float) vf->ysize;
          float mag = float_reg->get_value(s, t);
          vf->xval(i, j) = vf->xval(i, j) * mag;
          vf->yval(i, j) = vf->yval(i, j) * mag;
        }
    } COMMAND ("fblur  steps") {
      int steps;
      get_integer(&steps);
      float_reg->blur(steps);
    } COMMAND ("fdipole") {
      dipole_magnitude();
    } COMMAND ("bdraw") {
      win->clear();
      win->makeicolor(BLACK, 0, 0, 0);
      win->makeicolor(WHITE, 255, 255, 255);
      win->set_color_index(WHITE);
      bundle->draw(win);
      bundle2->draw(win);
    } COMMAND ("bcopy") {
      bundle2 = bundle;
    } COMMAND ("bclear") {
      bundle = new Bundle();
    } COMMAND ("intersect") {
      win->makeicolor(BLACK, 0, 0, 0);
      win->makeicolor(RED, 255, 0, 0);
      win->makeicolor(WHITE, 255, 255, 255);

      /* the first few streamlines */
#if 0
      int nlines = 1;

      for (int k = 0; k < nlines; k++) {
        Streamline *st = bundle->get_line(k);
        win->set_color_index (RED);
        st->draw(win);

        IntersectionChain *chain;
        chain = new IntersectionChain();
        chain->streamline_with_bundle (st, bundle2);

        win->set_color_index (WHITE);
        chain->draw(win);
        delete chain;
      }
#endif

      /* bundle with bundle */
#if 1
      IntersectionChain *chain;
      chain = new IntersectionChain();
      chain->bundle_with_bundle(bundle, bundle2);

      win->set_color_index(WHITE);
      chain->draw(win);
      delete chain;
#endif

    } COMMAND ("graph_quality  off/on") {
      graph_the_quality = get_boolean();
    } COMMAND ("ltest num") {
      int num;
      get_integer(&num);
      if (num == 0)
        num = 4;
      line_test(num);
    } COMMAND ("euler") {
      set_integration(EULER);
    } COMMAND ("midpoint") {
      set_integration(MIDPOINT);
    } COMMAND ("runge_kutta") {
      set_integration(RUNGE_KUTTA);
    } COMMAND ("animation  off/on") {
      animation_flag = get_boolean();
    } COMMAND ("graphics  off/on") {
      graphics_flag = get_boolean();
    } COMMAND ("quit") {
      printf("Bye-bye.\n");
      exit(0);
    } END_CLI ("Pardon?", "Bye-bye.")
}


/******************************************************************************
Interpret commands.  These are the documented commands only.
******************************************************************************/

void new_interpreter()
{
  int i, j;
  char filename[80];
  int grid_num = 40;
  float grid_dist = 0.3;

  START_CLI ("stplace", "cli")

    COMMAND ("vload file.vec") {
      get_parameter(filename);
      vf = new VectorField(filename);
      float_reg = vf->get_magnitude();
      vf->normalize();
    } COMMAND ("write_streamlines filename") {
      get_parameter(filename);
      if (taper_max > 0)
        bundle->write_ascii(filename, 1);
      else
        bundle->write_ascii(filename, 0);
    } COMMAND ("draw_streamlines") {
      win->clear();
      win->makeicolor(BLACK, 0, 0, 0);
      win->makeicolor(WHITE, 255, 255, 255);
      win->set_color_index(WHITE);
      bundle->draw(win);
    } COMMAND ("optimize  separation") {
      float sep;
      get_real(&sep);
      optimize_streamlines(sep);
    } COMMAND ("cascade  separation") {
      float sep;
      get_real(&sep);
      cascaded_improve(sep);
    } COMMAND ("tufts  separation  length") {
      float sep, len;
      get_real(&sep);
      get_real(&len);
      tufts(sep, len);
    } COMMAND ("taper") {
      taper_optimize();
    } COMMAND ("squares  num_across  length") {

      int num;
      get_integer(&num);
      if (num != 0)
        grid_num = num;

      float len;
      get_real(&len);
      if (len == 0) len = 0.1;
      vis_set_birth_length(len);

      square_grid(grid_num);
    } COMMAND ("hexagons  num_across  length") {

      int num;
      get_integer(&num);
      if (num != 0)
        grid_num = num;

      float len;
      get_real(&len);
      if (len == 0) len = 0.1;
      vis_set_birth_length(len);

      hexagonal_grid(grid_num);
    } COMMAND ("streamline xorg yorg len1 len2 (taper_tail taper_head)") {
      float x, y, len1, len2;
      float tail, head;
      float delta = delta_step;
      Streamline *st;
      get_real(&x);
      get_real(&y);
      get_real(&len1);
      get_real(&len2);
      get_real(&tail);
      get_real(&head);
      if (tail != 0.0 || head != 0.0) {
        set_taper(head, tail);
        st = new Streamline(vf, x, y, len1, len2, delta);
      } else
        st = new Streamline(vf, x, y, len1, len2, delta);
      bundle->add_line(st);
    } COMMAND ("delta_step  value") {
      get_real(&delta_step);
    } COMMAND ("quit") {
      printf("Bye-bye.\n");
      exit(0);
    } END_CLI ("Pardon?", "Bye-bye.")
}

