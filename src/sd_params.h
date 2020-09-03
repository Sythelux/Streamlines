//  Visualization Parameter Routines

/*
Copyright (c) 1996 The University of North Carolina.  All rights reserved.   
  
Permission to use, copy, modify and distribute this software and its   
documentation for any purpose is hereby granted without fee, provided   
that the above copyright notice and this permission notice appear in   
all copies of this software and that you do not sell the software.   
  
The software is provided "as is" and without warranty of any kind,   
express, implied or otherwise, including without limitation, any   
warranty of merchantability or fitness for a particular purpose.   
*/

void vis_initialize();

/* these all vary together */

int vis_get_lowpass_xsize();

int vis_get_lowpass_ysize();

void vis_set_separation(float);

float vis_set_variable_separation(FloatImage *);

float vis_get_separation(float, float);

void vis_get_separation_extrema(float &, float &);

void vis_set_minimum_blur(float);

float vis_get_blur_radius(float, float);

void vis_set_delta_move(float);

float vis_get_delta_move(float, float);

void vis_set_delta_length(float);

float vis_get_delta_length(float, float);

void vis_set_join_factor(float);

float vis_get_join_distance(float, float);

float vis_get_max_join_distance();

/* this should be separate */

void vis_set_birth_length(float);

float vis_get_birth_length(float, float);

void vis_vary_birth_length(int);

/* this is independent of separation */

void vis_set_draw_width(float);

void vis_set_draw_width(FloatImage *, float, float);

float vis_get_draw_width(float, float);

int vis_draw_width_varies();

/* this is independent of separation */

void vis_set_intensity(float);

void vis_set_intensity(FloatImage *, float, float);

float vis_get_intensity(float, float);

int vis_draw_intensity_varies();

/* this is independent of separation */

void vis_set_arrow_length(float);

void vis_set_arrow_length(FloatImage *, float, float);

float vis_get_arrow_length(float, float);

float vis_max_arrow_length();

int vis_arrow_length_varies();

