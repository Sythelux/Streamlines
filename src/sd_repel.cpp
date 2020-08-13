/*

Point repulsion for good streamline placement.

Greg Turk, May 1995

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
#include "window.h"
#include "floatimage.h"
#include "sd_vfield.h"
#include "sd_streamline.h"
#include "sd_repel.h"
#include "stdraw.h"


/******************************************************************************
Create a table for point repulsion.

Entry:
  vfield - vector field
  r      - largest radius of repulsion
******************************************************************************/

RepelTable::RepelTable(VectorField *vfield, float r)
{
  vf = vfield;

  radius = r;
  if (radius > 0.125)  /* don't want too few cells */
    radius = 0.125;

  x_scale = 0.5 / radius;
  y_scale = x_scale * vfield->getaspect();

  x_wrap = (int) (x_scale + 0.99999);
  y_wrap = (int) (y_scale + 0.99999);

  y_scale = x_scale;

  dist_max = radius * radius;

  /* create a table of cells, which is 2D array of pointers to SamplePoint's */

  cells = new CellEntry **[x_wrap];
  for (int i = 0; i < x_wrap; i++) {
    cells[i] = new CellEntry *[y_wrap];
    for (int j = 0; j < y_wrap; j++)
      cells[i][j] = NULL;
  }
}


/******************************************************************************
Delete the linked list of samples in the cell table.
******************************************************************************/

void RepelTable::delete_cell_entries()
{
  for (int i = 0; i < x_wrap; i++)
    for (int j = 0; j < y_wrap; j++) {
      CellEntry *cell = cells[i][j];
      while (cell != NULL) {
        CellEntry *next = cell->next;
        delete cell;
        cell = next;
      }
      cells[i][j] = NULL;
    }
}


/******************************************************************************
Destroy a table for point repulsion.
******************************************************************************/

RepelTable::~RepelTable()
{
  delete_cell_entries();

  for (int i = 0; i < x_wrap; i++)
    delete cells[i];

  delete cells;
}


/******************************************************************************
Do one step of repulsion.

Exit:
  returns new bundle of streamlines
******************************************************************************/

Bundle* RepelTable::repel(Bundle *bundle, float delta, float rmove)
{
  for (int i = 0; i < x_wrap; i++)
    for (int j = 0; j < y_wrap; j++)
      cells[i][j] = NULL;

  Bundle *new_bundle = new Bundle();

  /* examine all streamlines */

  for (i = 0; i < bundle->num_lines; i++) {

    Streamline *st = bundle->get_line(i);

    /* place all streamline's points in the table */
    for (int j = 0; j < st->samples; j++) {
      SamplePoint *sample = &st->pts[j];
      CellEntry *cell = new CellEntry (sample);
      int a = (int) (x_scale * sample->x);
      int b = (int) (y_scale * sample->y);
      if (a < 0 || a >= x_wrap || b < 0 || b >= y_wrap)
        continue;
      sample->st = st;
      cell->next = cells[a][b];
      cells[a][b] = cell;
    }
  }

  /* see how each streamline is pushed around */

  for (i = 0; i < bundle->num_lines; i++) {

    Streamline *st = bundle->get_line(i);
    st->xsum = 0;
    st->ysum = 0;

    /* look at each sample on the streamline */

    for (int j = 0; j < st->samples; j++) {
      SamplePoint *sample = &st->pts[j];
      float x = sample->x;
      float y = sample->y;
      int aa = (int) (x_scale * x);
      int bb = (int) (y_scale * y);
      int amin = x_wrap + aa - 1;
      int amax = x_wrap + aa + 1;
      int bmin = y_wrap + bb - 1;
      int bmax = y_wrap + bb + 1;

      /* see what the nearby points are */

      CellEntry *cell;
      SamplePoint *s;

      for (int a = amin; a <= amax; a++)
        for (int b = bmin; b <= bmax; b++)
          for (cell=cells[a%x_wrap][b%y_wrap]; cell != NULL; cell=cell->next) {
            s = cell->sample;
            if (s->st == st)
              continue;
            float dx = x - s->x;
            float dy = y - s->y;
            float dist = dx*dx + dy*dy;
            if (dist > dist_max)
              continue;
            dist = sqrt(dist);
            float weight = (radius - dist) / dist;
            st->xsum += dx * weight;
            st->ysum += dy * weight;
          }

    }
  }

  /* move each streamline */

  for (i = 0; i < bundle->num_lines; i++) {
    Streamline *st = bundle->get_line(i);
    float x,y;
    st->get_origin(x,y);
    float len = st->get_length();

    /* change position based on repulsion */
    x += st->xsum * rmove;
    y += st->ysum * rmove;

    /* clamp to edge of window */
    if (x < 0) x = 0;
    if (x > 1) x = 1;
    if (y < 0) y = 0;
    if (y > vf->getaspect()) y = vf->getaspect();

    Streamline *new_st = new Streamline (vf, x, y, len, delta);
    new_bundle->add_line (new_st);
  }


#if 0
  for (i = 0; i < bundle->num_lines; i++) {

    Streamline *st = bundle->get_line(i);

    float x,y;
    st->get_origin(x,y);
    float len = st->get_length();

    float tiny = 0.01;
    x += tiny * (drand48() - 0.5);
    y += tiny * (drand48() - 0.5);

    Streamline *new_st = new Streamline (vf, x, y, len, delta);
    new_bundle->add_line (new_st);
  }
#endif

  return (new_bundle);
}


/******************************************************************************
Add the endpoints of a streamline to a table.

Entry:
  st - streamline whose endpoints we should add
  head - add heads?
  tail - add tails?
******************************************************************************/

void RepelTable::add_endpoints(Streamline *st, int head, int tail)
{
  int a,b;
  SamplePoint *sample;
  CellEntry *cell;

  /* first point */

  if (tail) {

    sample = &st->pts[0];
    cell = new CellEntry (sample);

    a = (int) (x_scale * sample->x);
    b = (int) (y_scale * sample->y);
    if (a >= 0 && a < x_wrap && b >= 0 && b < y_wrap) {
      sample->st = st;
      sample->which_end = TAIL;
      cell->next = cells[a][b];
      cells[a][b] = cell;
    }

  }

  /* last point */

  if (head) {

    sample = &st->pts[st->samples-1];
    cell = new CellEntry (sample);

    a = (int) (x_scale * sample->x);
    b = (int) (y_scale * sample->y);
    if (a >= 0 && a < x_wrap && b >= 0 && b < y_wrap) {
      sample->st = st;
      sample->which_end = HEAD;
      cell->next = cells[a][b];
      cells[a][b] = cell;
    }

  }
}


/******************************************************************************
Add all of the points of a streamline to a table.

Entry:
  st - streamline whose endpoints we should add
******************************************************************************/

void RepelTable::add_all_points(Streamline *st)
{
  /* place each of the sample points into the table */

  for (int i = 0; i < st->samples; i++) {

    SamplePoint *sample = &st->pts[i];
    CellEntry *cell = new CellEntry (sample);

    int a = (int) (x_scale * sample->x);
    int b = (int) (y_scale * sample->y);
    if (a >= 0 && a < x_wrap && b >= 0 && b < y_wrap) {
      sample->st = st;

      /* label the sample as head, tail or other */
      if (i == 0)
        sample->which_end = TAIL;
      else if (i == st->samples-1)
        sample->which_end = HEAD;
      else
        sample->which_end = -1;

      cell->next = cells[a][b];
      cells[a][b] = cell;
    }
  }
}


/******************************************************************************
Remove the endpoints of a streamline from a table.

Entry:
  st - streamline whose endpoints we should remove
******************************************************************************/

void RepelTable::remove_endpoints(Streamline *st)
{
}


/******************************************************************************
Find the nearest sample point in a table to a given position.

Entry:
  x,y - given position

Exit:
  returns pointer to the nearest sample, or NULL if none found
******************************************************************************/

SamplePoint *RepelTable::find_nearest(float x, float y)
{
  float minimum = 1e20;  /* minimum distance so far */
  SamplePoint *closest = NULL;

  int aa = (int) (x_scale * x);
  int bb = (int) (y_scale * y);
  int amin = x_wrap + aa - 1;
  int amax = x_wrap + aa + 1;
  int bmin = y_wrap + bb - 1;
  int bmax = y_wrap + bb + 1;

  SamplePoint *s;
  CellEntry *cell;

  for (int a = amin; a <= amax; a++)
    for (int b = bmin; b <= bmax; b++)
      for (cell = cells[a%x_wrap][b%y_wrap]; cell != NULL; cell = cell->next) {

        s = cell->sample;

        float dx = x - s->x;
        float dy = y - s->y;
        float dist = dx*dx + dy*dy;

        /* closer than previous samples? */

        if (dist < minimum) {
          minimum = dist;
          closest = s;
        }
      }

  /* return the closest sample */
  return (closest);
}

#if 0

/******************************************************************************
Find where the centers of the two old streamlines should move to in order
to be exactly where they can join.  This is for animation.

Entry:
  st,st2 - streamlines being joined
  new_st - streamline that is created when st and st2 are joined
  delta  - step size for integrating along streamlines

Exit:
  x1,y1 - new center of st1
  x2,y2 - new center of st2
******************************************************************************/

void RepelTable::find_new_centers(
  Streamline *st1,
  Streamline *st2,
  Streamline *new_st,
  float delta,
  float& x1,
  float& y1,
  float& x2,
  float& y2
)
{
  int i;
  int steps;
  float x,y;
  float len;
  float ilen;
  float d;

  /* start at one end of the new streamline and follow along */
  /* half of st1's length */

  x = new_st->pts[0].x;
  y = new_st->pts[0].y;

  len = 0.5 * st1->get_length();
  steps = (int) (len / delta);
  d = len / steps;

  for (i = 0; i < steps; i++)
    ilen = vf->integrate (x, y, d, 0, x, y);

  x1 = x;
  y1 = y;

  /* start at the other end of the new streamline */

  x = new_st->pts[new_st->samples-1].x;
  y = new_st->pts[new_st->samples-1].y;

  len = 0.5 * st2->get_length();
  steps = (int) (len / delta);
  d = len / steps;

  for (i = 0; i < steps; i++)
    ilen = vf->integrate (x, y, -d, 0, x, y);

  x2 = x;
  y2 = y;
}

#endif

/******************************************************************************
Find where the centers of the two old streamlines should move to in order
to be exactly where they can join.  This is for animation.

Entry:
  st,st2 - streamlines being joined
  new_st - streamline that is created when st and st2 are joined
  delta  - step size for integrating along streamlines

Exit:
  x1,y1 - new center of st1
  x2,y2 - new center of st2
******************************************************************************/

void RepelTable::find_new_centers(
  Streamline *st1,
  Streamline *st2,
  Streamline *new_st,
  float delta,
  float& x1,
  float& y1,
  float& x2,
  float& y2
)
{
  int i;
  int steps;
  float x,y;
  float len;
  float ilen;
  float d;

  float big_len = new_st->get_length();

  /* start at one end of the new streamline and follow along */
  /* half of st1's length */

  x = new_st->xorig;
  y = new_st->yorig;

  len = 0.5 * st1->get_length();
  len = 0.5 * big_len - len;
  steps = (int) fabs (len / delta);
  d = len / steps;

  for (i = 0; i < steps; i++)
    ilen = vf->integrate (x, y, -d, 0, x, y);

  x1 = x;
  y1 = y;

  /* start at the other end of the new streamline */

  x = new_st->xorig;
  y = new_st->yorig;

  len = 0.5 * st2->get_length();
  len = 0.5 * big_len - len;
  steps = (int) fabs (len / delta);
  d = len / steps;

  for (i = 0; i < steps; i++)
    ilen = vf->integrate (x, y, d, 0, x, y);

  x2 = x;
  y2 = y;
}

