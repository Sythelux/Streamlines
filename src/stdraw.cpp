/*

Vector field visualization.

Version 0.5

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


#include <cstdio>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <strings.h>
#include <cmath>
#include <cstring>
#include "cli.h"
#include "window.h"
#include "floatimage.h"
#include "sd_picture.h"
#include "sd_vfield.h"
#include "sd_streamline.h"
#include "sd_repel.h"
#include "sd_params.h"
#include "stdraw.h"

/* external declarations and forward pointers to routines */

void interpreter();

VectorField *vf = nullptr;         /* the vector field we're visualizing */
FloatImage *float_reg = nullptr;   /* scalar field "register" */

Bundle *bundle;                 /* bundle of streamlines */

Window2d *win;                  /* windows to draw into */

int xsize = 512;                /* window sizes */
int ysize = 512;

int keep_reading_events;        /* for interrupting window event loop */

/* draw things? */
static int graphics_flag = 1;

/* vary the intensity of fancy arrows? */
int vary_arrow_intensity = 0;

/* how much to step while integrating through the vector field */
float delta_step = 0.005;



/* global arrow style */
#define  ARROW_NONE      1
#define  ARROW_FANCY     2
#define  ARROW_HEX       3
#define  ARROW_HEADS     4
#define  ARROW_HEXHEADS  5
static int arrow_style = ARROW_NONE;

/* filled or open arrow? */
#define  ARROW_FILLED  1
#define  ARROW_OPEN    2
static int arrow_type = ARROW_FILLED;

/* arrow size */
static float arrow_length = 0.01;
static float arrow_width = 0.0066;

/* fancy arrow length and separation */
static float fancy_length = 0.08;
static float fancy_separation = 0.01;

/* how many arrows across for hex pattern? */
static float hex_count = 5;


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
        default:
          break;
      }
  }

  /* read in a vector field */

  if (argc > 0) {

    vf = new VectorField(*argv);
    ysize = xsize * vf->getaspect();

    /* save the magnitude in a separate place */
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
  }

  /* initialize visualization parameters */

  vis_initialize();

  /* initialize the bundle */

  bundle = new Bundle();

  /* initialize the scalar field */

  float_reg = new FloatImage(2, 2);

  /* call command interpreter */

  interpreter();
  return 0;
}


/******************************************************************************
Draw an arrow, either open or filled.

Entry:
  pic       - picture to draw to
  x,y       - position to draw arrow at
  width     - arrow width
  length    - arrow length
  thickness - width of line to draw with
******************************************************************************/

void draw_arrow(
        Picture *pic,
        float x,
        float y,
        float width,
        float length,
        float thickness
)
{
  float len;
  float dx, dy;
  float dt = delta_step;   /* delta length for stepping along lines */

  /* find the base of the arrowhead */

  int steps = (int) floor(length / dt);
  dt = length / steps;

  float xx = x;
  float yy = y;

  for (int i = 0; i < steps; i++)
    vf->integrate(xx, yy, -dt, 1, xx, yy);

  dx = x - xx;
  dy = y - yy;
  len = sqrt(dx * dx + dy * dy);
  if (len != 0.0) {
    dx /= len;
    dy /= len;
  }

  float x1 = xx + dy * width;
  float y1 = yy - dx * width;
  float x2 = xx - dy * width;
  float y2 = yy + dx * width;

  if (arrow_type == ARROW_OPEN) {
    pic->thick_line(x, y, x1, y1, thickness);
    pic->thick_line(x, y, x2, y2, thickness);
  } else {
    pic->polygon_start();
    pic->polygon_vertex(x, y);
    pic->polygon_vertex(x1, y1);
    pic->polygon_vertex(x2, y2);
    pic->polygon_fill();
  }
}


/******************************************************************************
Snap arrowheads to streamlines.  Arrowheads approximate a hex grid.

Entry:
  steps  - number of grid steps across and down
  width  - width of arrows
  length - length of arrows
  pic    - picture to write to
  head   - snap only to heads of streamlines?
******************************************************************************/

void snap_arrows_to_streamlines(
        float steps,
        float width,
        float length,
        Picture *pic,
        int head
)
{
  float x, y;
  float s;

  /* set up drawing window stuff */

  pic->set_intensity(1.0);

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
      float jitter = 0.0;
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

      float thickness = 1;
      draw_arrow(pic, nx, ny, s * width, s * length, thickness);
    }
  }

  pic->flush();
}


/******************************************************************************
Draw the streamlines as dashed lines of a given length.  Either draw into
a window or write to a file.

Entry:
  pic          - window or file to draw to
  bundle       - bundle of streamlines to draw with
  len          - length of dashes
  separation   - separation length between dashes
  arrow_length - length of arrowhead
  arrow_width  - width of arrowhead
******************************************************************************/

void draw_dashes(
        Picture *pic,
        Bundle *bundle,
        float len,
        float separation,
        float arrow_length,
        float arrow_width
)
{
  pic->set_intensity(1.0);

  for (int i = 0; i < bundle->num_lines; i++) {

    Streamline *st = bundle->get_line(i);

    if (vis_arrow_length_varies())
      st->variable_draw_dashed(pic, separation, arrow_length, arrow_width);
    else
      st->draw_dashed(pic, len, separation, arrow_length, arrow_width);
  }
}


/******************************************************************************
Draw arrows at the head of every streamline.
******************************************************************************/

void draw_arrows_at_heads(Picture *pic)
{
  float x, y;
  float width;
  float val;

  for (int i = 0; i < bundle->num_lines; i++) {

    Streamline *st = bundle->get_line(i);
    st->get_head(x, y);

    val = vis_get_intensity(x, y) * st->get_intensity();;
    pic->set_intensity(val);

    width = vis_get_draw_width(x, y);

    draw_arrow(pic, x, y, arrow_width, arrow_length, width);
  }
}


/******************************************************************************
Make a picture of all the streamlines, either in a window or write to a file.

Entry:
  filename - name of Postscript file, or NULL if it should be drawn in window
******************************************************************************/

void draw_streamlines(char *filename)
{
  Picture *pic;

  if (filename)
    pic = new Picture(vf->getaspect(), filename);
  else {
    pic = new Picture(win);
    win->clear();
  }

  if (arrow_style == ARROW_FANCY) {
    draw_dashes(pic, bundle, fancy_length, fancy_separation,
                arrow_length, arrow_width);
  } else {

    /* draw the streamlines */

    bundle->draw(pic);

    /* maybe draw some arrows */

    if (arrow_style == ARROW_HEX)
      snap_arrows_to_streamlines(hex_count, arrow_width, arrow_length, pic, 0);
    else if (arrow_style == ARROW_HEXHEADS)
      snap_arrows_to_streamlines(hex_count, arrow_width, arrow_length, pic, 1);
    else if (arrow_style == ARROW_HEADS)
      draw_arrows_at_heads(pic);
  }

  delete pic;
}


/******************************************************************************
Interpret commands.
******************************************************************************/

void interpreter()
{
  char str[80];
  char filename[80];

  START_CLI ("stdraw", "cli")

    COMMAND ("vload file.vec") {
      get_parameter(filename);
      vf = new VectorField(filename);
      float_reg = vf->get_magnitude();
      vf->normalize();
    } COMMAND ("draw_picture") {
      draw_streamlines(NULL);
    } COMMAND ("save_picture file.ps") {
      get_parameter(filename);
      draw_streamlines(filename);
    } COMMAND ("arrows (none | fancy | heads | hex | hexheads)") {
      get_parameter(str);
      if (strcmp(str, "none") == 0)
        arrow_style = ARROW_NONE;
      else if (strcmp(str, "fancy") == 0)
        arrow_style = ARROW_FANCY;
      else if (strcmp(str, "heads") == 0)
        arrow_style = ARROW_HEADS;
      else if (strcmp(str, "hex") == 0)
        arrow_style = ARROW_HEX;
      else if (strcmp(str, "hexheads") == 0)
        arrow_style = ARROW_HEXHEADS;
      else
        printf("'%s' is not a valid arrow style.\n", str);
    } COMMAND ("type_arrow (open | filled)") {
      get_parameter(str);
      if (strcmp(str, "open") == 0)
        arrow_type = ARROW_OPEN;
      else if (strcmp(str, "filled") == 0)
        arrow_type = ARROW_FILLED;
      else
        printf("'%s' is not a valid arrow type.\n", str);
    } COMMAND ("size_arrow  length  width") {
      get_real(&arrow_length);
      get_real(&arrow_width);
    } COMMAND ("hex_count  num_arrows_across") {
      get_real(&hex_count);
    } COMMAND ("fancy_arrow  length (length2) separation") {
      float r1, r2, r3;
      get_real(&r1);
      get_real(&r2);
      get_real(&r3);
      if (r3 == 0) {
        fancy_length = r1;
        fancy_separation = r2;
        vis_set_arrow_length(r1);
      } else {
        fancy_length = r1;
        fancy_separation = r3;
        vis_set_arrow_length(float_reg, r1, r2);
      }
    } COMMAND ("width_line  min  max") {
      float min, max;
      get_real(&min);
      get_real(&max);
      if (max == 0)
        vis_set_draw_width(min);
      else
        vis_set_draw_width(float_reg, min, max);
    } COMMAND ("intensity  min  max") {
      float min, max;
      get_real(&min);
      get_real(&max);
      if (max == 0)
        vis_set_intensity(min);
      else
        vis_set_intensity(float_reg, min, max);
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
    }

#if 0

      COMMAND ("thickness  line_width") {
        float w;
        get_real (&w);
        set_line_thickness (w);
      }

      COMMAND ("reduce_length  start  end") {
        int start,end;
        get_integer (&start);
        get_integer (&end);
        set_reduction (start, end);
      }

      COMMAND ("intensity  value") {
        float val;
        get_real (&val);
        set_intensity (val);
      }

      COMMAND ("fmap  min max") {
        float min,max;
        get_real (&min);
        get_real (&max);
        float_reg->remap (min, max);
      }

      COMMAND ("idashes  off/on (vary dash intensity?)") {
        vary_arrow_intensity = get_boolean();
      }

      COMMAND ("snap_arrows  num_across width length") {
        int num;
        float width,length;
        get_integer (&num);
        get_real (&width);
        get_real (&length);
        Picture *pic = new Picture(win);
        snap_arrows_to_streamlines (num, width, length, pic, 0);
      }

      COMMAND ("hsnap_arrows  num_across width length") {
        int num;
        float width,length;
        get_integer (&num);
        get_real (&width);
        get_real (&length);
        Picture *pic = new Picture(win);
        snap_arrows_to_streamlines (num, width, length, pic, 1);
      }

      COMMAND ("psnap_arrows  num_across width length file") {
        int num;
        float width,length;
        get_integer (&num);
        get_real (&width);
        get_real (&length);
        get_parameter (filename);
        Picture *pic = new Picture(vf->getaspect(), filename);
        snap_arrows_to_streamlines (num, width, length, pic, 0);
        delete pic;
      }

      COMMAND ("hpsnap_arrows  num_across width length file") {
        int num;
        float width,length;
        get_integer (&num);
        get_real (&width);
        get_real (&length);
        get_parameter (filename);
        Picture *pic = new Picture(vf->getaspect(), filename);
        snap_arrows_to_streamlines (num, width, length, pic, 1);
        delete pic;
      }

#endif

    COMMAND ("quit") {
      printf("Bye-bye.\n");
      exit(0);
    } END_CLI ("Pardon?", "Bye-bye.")
}

