//
//  streamline class
//

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


#ifndef _STREAMLINE_CLASS_
#define _STREAMLINE_CLASS_

#include <fstream>
#include "vfield.h"
#include "../libs/window.h"
#include "../libs/floatimage.h"

using namespace std;

class Streamline;

class PixelValue
{
public:
    int i, j;
    float value;
    float len_pos;       /* how far along streamline? (for intensity taper) */
};

#define TAIL  2
#define HEAD  3
#define LEFT  4
#define RIGHT 5

class SamplePoint
{
public:
    float x, y;           /* position of sample point */
    SamplePoint *next;   /* next point in linked list */
    Streamline *st;
    unsigned char which_end;
    float intensity;
    float len_pos;       /* how far along streamline? (for intensity taper) */
};

class Streamline
{

    VectorField *vf;      /* vector field we're embedded in */
    float xorig, yorig;    /* "center" of streamline */
    float length1;        /* forward length */
    float length2;        /* backward length */
    float delta;          /* step size factor */
    int samples;          /* number of points representing the streamline */
    SamplePoint *pts;     /* the points along the streamline */
    PixelValue *values;   /* pixel differences that we cause to lowpass image */
    int max_values;       /* memory allocated to values */
    int list_index;       /* where we are in lowpass image bundle */
    float xsum, ysum;      /* sum of repelling forces */
    float quality;        /* est. of quality of streamline (placement goodness) */
    long unsigned int change;  /* recommended change, based on quality */
    int label;            /* what kind of streamline (for drawing purposes) */
    int arrow_type;       /* what kind of arrow, if any */
    float arrow_length;   /* length of arrowhead */
    float arrow_width;    /* width of arrowhead */
    float arrow_steps;    /* how many steps to make to draw arrowhead */
    int start_reduction;  /* reduction of length at start, for drawing */
    int end_reduction;    /* reduction of length at end, fr drawing */
    float intensity;      /* how bright to draw it */

public:

    int frozen;           /* a frozen streamline is one that isn't to be moved */
    int num_values;       /* current number of values */
    int anim_index;       /* index number for animation */

    float taper_head;     /* intensity tapering at head */
    float taper_tail;     /* intensity tapering at tail */
    float head_zero;      /* where tapering at head falls to zero */
    float tail_zero;      /* where tapering at tail falls to zero */
    int tail_clipped;
    int head_clipped;

    Streamline(VectorField *, float, float, float, float);

    Streamline(VectorField *, float, float, float, float, float);

    void streamline_creator(VectorField *, float, float, float, float, float);

    ~Streamline()
    {
      delete values;
      delete pts;
    }

    void draw(Window2d *);

    void draw_for_taper(Window2d *);

    void erase(Window2d *);

    void draw_helper(Window2d *, int);

    void variable_draw_dashed(Window2d *, ofstream *, float, float, float);

    int search_on_streamline(float, float &, float &);

    float dash_helper(float, float, Window2d *);

    void draw_dashed(Window2d *, ofstream *, float, float, float, float);

    void draw_dash(Window2d *, float, float, float);

    void draw_arrowhead(Window2d *, float, float, float, float);

    void draw_fancy_arrow(Window2d *, float, float, float, float, float, float);

    void draw_fancy_arrow_ps(float, ofstream *,
                             float, float, float, float, float, float);

    void add_value(int i, int j, float value);

    void get_value(int num, int &i, int &j, float &value);

    void clear_values();

    float get_quality()
    { return (quality); }

    float len_to_intensity(float);

    void get_origin(float &x, float &y)
    {
      x = xorig;
      y = yorig;
    }

    void get_head(float &x, float &y)
    {
      x = pts[samples - 1].x;
      y = pts[samples - 1].y;
    }

    void get_tail(float &x, float &y)
    {
      x = pts[0].x;
      y = pts[0].y;
    }

    float get_length()
    { return (length1 + length2); }

    float get_intensity()
    { return (intensity); }

    void get_taper(float &head, float &tail)
    {
      head = taper_head;
      tail = taper_tail;
    }

    void get_lengths(float &len1, float &len2)
    {
      len1 = length1;
      len2 = length2;
    }

    Streamline *copy();

    float &xs(int index)
    { return pts[index].x; }

    float &ys(int index)
    { return pts[index].y; }

    int get_samples()
    { return (samples); }

    void write_postscript(ofstream *file_out);

    friend class Bundle;

    friend class Lowpass;

    friend class RepelTable;
};

/* routines that set default streamline parameters */

void set_reduction(int, int);

void set_arrow(int, float, float, int);

void set_label(int);

void set_intensity(float);

void set_taper(float, float);

void set_line_thickness(float);

/* some types for streamlines */

#define  NO_ARROW     0
#define  OPEN_ARROW   1
#define  SOLID_ARROW  2


/* a bundle of streamlines */

class Bundle
{
    Streamline **lines;
    int max_lines;
public:
    int num_lines;

    Bundle()
    {
      max_lines = 500;
      num_lines = 0;
      lines = new Streamline *[max_lines];
    }

    ~Bundle()
    {
      delete lines;
    }

    void add_line(Streamline *);

    void delete_line(Streamline *);

    Bundle *copy();

    Streamline *get_line(int index)
    { return (lines[index]); }

    void write_postscript(ofstream *);

    void write_ascii(char *, int);

    FloatImage *filtered_render(int, int, float);

    void write_pgm(char *, int, int);

    void quality_sort();

    void sort_passes(int);

    void draw(Window2d *win)
    {
      for (int i = 0; i < num_lines; i++)
        lines[i]->draw(win);
    }
};

extern void clamp_to_screen(float &, float &, float);

extern void write_postscript_start(ofstream *);

extern void write_postscript_end(ofstream *);

#endif /* _STREAMLINE_CLASS_ */

