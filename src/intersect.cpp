/*

Intersections between streamlines.

David Banks, July 1995

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
#include "lowpass.h"
#include "intersect.h"


/******************************************************************************
Locate the intersections between two bundles of streamlines and
collect them into an intersection chain. The chain follows
intersections as they are ordered along streamlines in the first bundle.

Entry:
  bundle1 - the first bundle of streamlines
  bundle2 - the second bundle of streamlines 
******************************************************************************/

void IntersectionChain::bundle_with_bundle(Bundle *bundle1, Bundle *bundle2)
{
  Streamline *line_in_bundle;
  int i;

  for (i = 0; i < bundle1->num_lines; i++) {
    line_in_bundle = bundle1->get_line(i);
    streamline_with_bundle(line_in_bundle, bundle2);
  }
}


/******************************************************************************
Locate the intersections between a streamline and a bundle of streamlines 
and collect them into an intersection chain. 

Entry:
  st     - the streamline
  bundle - the bundle of streamlines
******************************************************************************/
void IntersectionChain::streamline_with_bundle(Streamline *st, Bundle *bundle)
{
  int intersection_exists = 0;
  int i;
  Streamline *line_in_bundle;

  for (i = 0; i < bundle->num_lines; i++) {
    line_in_bundle = bundle->get_line(i);
    streamline_with_streamline(st, line_in_bundle);
  }

}


/******************************************************************************
Locate the intersections between two streamlines and collect them 
into an intersection chain. 

Entry:
  st   - the first streamline
  st2  - the second streamline
******************************************************************************/
void IntersectionChain::streamline_with_streamline(Streamline *st, 
						   Streamline *st2)
{
  int sample;    /* number of samples in a streamline */
  int i;
  float x1, y1, x2, y2;

  sample = st->get_samples();    /* there are always >= 1 samples */
  for (i = 0; i < sample-1; i++) {
      x1 = st->xs(i );
      y1 = st->ys(i );
      x2 = st->xs(i+1);
      y2 = st->ys(i+1); 
// printf ("sample %d %f %f %f %f\n", i, x1, y1, x2, y2);
      segment_with_streamline( x1, y1, x2, y2, st, st2);
      }
}


/******************************************************************************
Locate the intersections between a segment and a streamlines and collect them 
into an intersection chain. 

Entry:
  x1, y1  - one endpoint of the segment 
  x2, y2  - the other endpoint of the segment 
  st, st2 - the streamlines
******************************************************************************/
void IntersectionChain::segment_with_streamline(float x1, float y1, 
					   float x2, float y2,
					   Streamline *st, Streamline *st2)
{
  int sample;	
  int i, intersect_flag;
  float x3, y3, x4, y4;
  float x, y;

// printf ("one segment %f %f %f %f\n", x1, y1, x2, y2);

  sample = st->get_samples();    /* there are always >= 1 samples */
  for (i = 0; i < sample-1; i++) {
      x3 = st2->xs(i );
      y3 = st2->ys(i );
      x4 = st2->xs(i+1);
      y4 = st2->ys(i+1); 
// printf ("second segment: %f %f %f %f\n", x3, y3, x4, y4);
      intersect_flag = segment_with_segment(
			   x1, y1, x2, y2, 
			   x3, y3, x4, y4, 
			   &x, &y);
// printf ("intersect flag = %d\n", intersect_flag);
// printf ("\n");
      if (intersect_flag == DO_INTERSECT) {
// printf ("x y: %f %f\n", x, y);
	add_point(x, y, st);
	}
      }
}


/******************************************************************************
Add an intersection to the intersection chain.

Entry:
  x, y - the intersection point to be added
  st   - the streamline
******************************************************************************/
void IntersectionChain::add_point(float x, float y, Streamline *st)
{
  if (num_points >= max_points - 1) {
    Intersection *temp = new Intersection[max_points * 2];
    for (int i = 0; i < max_points; i++) {
      temp[i].st     = points[i].st;
      temp[i].sample = points[i].sample;
      temp[i].x      = points[i].x;
      temp[i].y      = points[i].y;
      temp[i].arclen = points[i].arclen;
    }
    delete points;
    points = temp;
    max_points *= 2;
  }

  points[num_points].st = st;
  /*
  points[num_points].sample = sample;
  */
  points[num_points].x = x;
  points[num_points].y = y;
  points[num_points].arclen = 0.0;
  num_points++;
}


/******************************************************************************
Draw a streamline in a given window.
******************************************************************************/

void IntersectionChain::draw(Window2d *win)
{
  /* draw line segments */
  int i;

  float x, y, dx = 0.01, dy = 0.01;

  for (i = 0; i < num_points; i++) {
    x = points[i].x;
    y = points[i].y;
    win->line (x-dx, y-dy, x+dx, y-dy);
    win->line (x+dx, y-dy, x+dx, y+dy);
    win->line (x+dx, y+dy, x-dx, y+dy);
    win->line (x-dx, y+dy, x-dx, y-dy);

    win->line (x-dx, y-dy, x+dx, y+dy);
    win->line (x+dx, y-dy, x-dx, y+dy);
  }

  win->flush();
}


/******************************************************************************
Convert the samples in a streamline into an intersection chain.

Entry:
  st   - the streamline to be converted
******************************************************************************/
void IntersectionChain::from_streamline(Streamline *st)
{
  int i, sample;
  float x, y;

  sample = st->get_samples();    /* there are always >= 1 samples */
  for (i = 0; i < sample; i++) {
    x = st->xs(i);
    y = st->ys(i);
    add_point(x, y, st);
    }
}

