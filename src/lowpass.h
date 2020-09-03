//
//  Low-pass image, for determining if a computed image is near "optimal"
//  in gray-scale value if it is low-pass filtered
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


#include <math.h>
#include "../libs/floatimage.h"
#include "streamline.h"

#ifndef _LOWPASS_CLASS_
#define _LOWPASS_CLASS_

class Lowpass
{
    FloatImage *image;     /* low-pass version of image */
    float target;          /* target gray-scale pixel value */
    float sum;             /* current sum of all pixel values */
    float dev;             /* deviation from target */
    FloatImage *rad_image; /* spatially varying radius */
public:
    Bundle *bundle;      /* bundle of streamlines in the image */
    int xsize, ysize;

    Lowpass(int, int, float, float);

    ~Lowpass()
    {
      delete image;
      delete rad_image;
      delete bundle;
    }

    float current_quality()
    { return (sum); }

    float new_quality(Streamline *st);

    float get_radius(float x, float y)
    {
      return (rad_image->get_value(x, y));
    }

    float better_filter_segment(int, int, float, float, float, float,
                                float, float, float);

    void add_line(Streamline *st);

    void delete_line(Streamline *st);

    void delete_streamlines();

    float recalculate_quality();

    void draw(Window2d *win)
    { image->draw(win); }

    void draw_lines(Window2d *win);

    void filter_segment(float, float, float, float, float);

    void set_radius(float, float);

    FloatImage *get_image_ptr()
    { return (image); }

    FloatImage *get_image_copy()
    {
      FloatImage *img = image->copy();
      return (img);
    }

    int birth_test(float, float, float);

    unsigned long int recommend_change(Streamline *);

    void streamline_quality(Streamline *, float, int, float, Window2d *);

    float random_samples_quality(Streamline *, float, int, float, Window2d *);

    float lengthen_quality(Streamline *, float, int, float, Window2d *, int &);

    float shorten_quality(Streamline *, float, int, float, Window2d *, int &);

    float main_body_quality(Streamline *, float, int, float, Window2d *, int &);

    friend class Streamline;
};

#endif /* _LOWPASS_CLASS_ */

