/*

Visualization Parameter Routines

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
#include "../libs/floatimage.h"
#include "vfield.h"
#include "streamline.h"
#include "lowpass.h"
#include "stplace.h"

extern VectorField *vf;

static FloatImage *separation = NULL;    /* separation between lines */
static float sep_min,sep_max;            /* min and max separation */
static float blur_min;                   /* minimum blur radius */
static float separation_to_blur;         /* conversion factor */
static float delta_move;                 /* moving streamline */
static float delta_length;               /* lengthen/shorten streamline */
static float join_factor;                /* max. distance to join lines */
  
static float birth_length;               /* birth length of streamline */
static int vary_birth_length;            /* whether birth length varies */

static FloatImage *dwidth = NULL;        /* variable drawing width */
static float draw_width = 1.0;           /* drawing width */

static FloatImage *arrow_len_img = NULL; /* variable arrow length */
static float arrow_len = 1.0;            /* arrow length */
static float max_arrow_len = 1.0;        /* maximum arrow length */


/******************************************************************************
Initialize various visualization parameters.
******************************************************************************/

void vis_initialize()
{
  separation = new FloatImage (4, 40);
  separation->setimage (0.04);
  separation->get_extrema (sep_min, sep_max);

  separation_to_blur = 6.0 / 5.0;
  blur_min = 2.0;
  birth_length = 0.1;
  vary_birth_length = 0;

  delta_move = 0.5;
  delta_length = 1.125;

  join_factor = 0.0;
}


/******************************************************************************
Return the width in pixels of the lowpass image.
******************************************************************************/

int vis_get_lowpass_xsize()
{
  float size = separation_to_blur * blur_min / sep_min;

  return ((int) floor(size));
}


/******************************************************************************
Return the height in pixels of the lowpass image.
******************************************************************************/

int vis_get_lowpass_ysize()
{
  float size = separation_to_blur * blur_min / sep_min;
  size *= vf->getaspect();

  return ((int) floor(size));
}


/******************************************************************************
Set the separation distance between streamlines to a constant.
******************************************************************************/

void vis_set_separation(float s)
{
  separation->setimage(s);
  separation->get_extrema (sep_min, sep_max);
}


/******************************************************************************
Set the separation distance between streamlines to values specified by
an image.
******************************************************************************/

void vis_set_variable_separation(FloatImage *img)
{
  separation = img->copy();
  separation->get_extrema (sep_min, sep_max);
}


/******************************************************************************
Return the separation distance between streamlines at a given position
in the image.
******************************************************************************/

float vis_get_separation(float x, float y)
{
  float sep = separation->get_value(x,y);
  return (sep);
}


/******************************************************************************
Return the min and max separation distance.
******************************************************************************/

void vis_get_separation_extrema(float &min, float &max)
{
  min = sep_min;
  max = sep_max;
}


/******************************************************************************
Set the minimum blur radius for the lowpass image.
******************************************************************************/

void vis_set_minimum_blur(float r)
{
  blur_min = r;
}


/******************************************************************************
Return the correct blur radius at a particular place in the lowpass image.
******************************************************************************/

float vis_get_blur_radius(float x, float y)
{
  float sep = separation->get_value(x,y);
  return (blur_min * sep / sep_min);
}


/******************************************************************************
Set the minimum length of a streamline that is being born.
******************************************************************************/

void vis_set_birth_length(float len)
{
  birth_length = len;
}


/******************************************************************************
Return the correct birth length of a streamline at a particular place
in the lowpass image.
******************************************************************************/

float vis_get_birth_length(float x, float y)
{
  if (vary_birth_length) {
    float sep = separation->get_value(x,y);
    return (birth_length * sep / sep_min);
  }
  else
    return (birth_length);
}


/******************************************************************************
Set the flag saying whether the birth length varies or not.
******************************************************************************/

void vis_vary_birth_length(int flag)
{
  vary_birth_length = flag;
}


/******************************************************************************
Set the scaling factor for how far to move a streamline.
******************************************************************************/

void vis_set_delta_move(float s)
{
  delta_move = s;
}


/******************************************************************************
Get the scaling factor for how far to move a streamline.
******************************************************************************/

float vis_get_delta_move(float x, float y)
{
  float sep = separation->get_value(x,y);
  return (delta_move * sep);
}


/******************************************************************************
Set the scaling factor for how much to lengthen or shorten a streamline.
******************************************************************************/

void vis_set_delta_length(float s)
{
  delta_length = s;
}


/******************************************************************************
Get the scaling factor for how much to lengthen or shorten a streamline.
******************************************************************************/

float vis_get_delta_length(float x, float y)
{
  float sep = separation->get_value(x,y);
  return (delta_length * sep);
}


/******************************************************************************
Set the scaling factor for distance over which lines can be joined.
******************************************************************************/

void vis_set_join_factor(float s)
{
  join_factor = s;
}


/******************************************************************************
Return the distance over which two lines may be joined.
******************************************************************************/

float vis_get_join_distance(float x, float y)
{
  float sep = separation->get_value(x,y);
  return (sep * join_factor * separation_to_blur);
}


/******************************************************************************
Return the maximum possible distance over which two lines may be joined.
******************************************************************************/

float vis_get_max_join_distance()
{
  return (sep_max * join_factor * separation_to_blur);
}


/******************************************************************************
Set how wide streamlines should be drawn.  Constant value.
******************************************************************************/

void vis_set_draw_width(float w)
{
  if (dwidth) {
    delete dwidth;
    dwidth = NULL;
  }

  draw_width = w;
}


/******************************************************************************
Set how wide streamlines should be drawn.  Variable according to an image.
******************************************************************************/

void vis_set_draw_width(FloatImage *img, float min, float max)
{
  if (dwidth)
    delete dwidth;
  
  dwidth = img->copy();
  dwidth->remap (min, max);
}


/******************************************************************************
Given a position, return the drawing width.
******************************************************************************/

float vis_get_draw_width(float x, float y)
{
  if (dwidth)
    return (dwidth->get_value(x,y));
  else
    return (draw_width);
}


/******************************************************************************
Return 1 if the drawing width is variable, otherwise return 0.
******************************************************************************/

int vis_draw_width_varies()
{
  if (dwidth)
    return 1;
  else
    return 0;
}


/******************************************************************************
Set how long arrows should be drawn.  Constant value.
******************************************************************************/

void vis_set_arrow_length(float len)
{
  if (arrow_len_img) {
    delete arrow_len_img;
    arrow_len_img = NULL;
  }

  arrow_len = len;
  max_arrow_len = len;
}


/******************************************************************************
Set how long arrows should be drawn.  Variable according to an image.
******************************************************************************/

void vis_set_arrow_length(FloatImage *img, float min, float max)
{
  if (arrow_len_img)
    delete arrow_len_img;
  
  arrow_len_img = img->copy();
  arrow_len_img->remap (min, max);
  max_arrow_len = max;
}


/******************************************************************************
Given a position, return the arrow length.
******************************************************************************/

float vis_get_arrow_length(float x, float y)
{
  if (arrow_len_img)
    return (arrow_len_img->get_value(x,y));
  else
    return (arrow_len);
}


/******************************************************************************
Return the maximum arrow length.
******************************************************************************/

float vis_max_arrow_length()
{
  return (max_arrow_len);
}


/******************************************************************************
Return 1 if the arrow length is variable, otherwise return 0.
******************************************************************************/

int vis_arrow_length_varies()
{
  if (arrow_len_img)
    return 1;
  else
    return 0;
}

