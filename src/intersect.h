//
//  intersection class
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


#include "vfield.h"
#include "streamline.h"

#ifndef _INTERSECTION_CLASS_
#define _INTERSECTION_CLASS_

#define DONT_INTERSECT    0
#define DO_INTERSECT      1
#define COLLINEAR         2

class Intersection
{

private:
    Streamline *st;       /* streamline */
    SamplePoint *sample;   /* sample point on streamline */
    float x, y;             /* actual position of intersection */
    float arclen;          /* arclength along streamline */
public:
    friend class IntersectionChain;

};


class IntersectionChain
{
    Intersection *points;
    int max_points;
    int num_points;
public:
    IntersectionChain()
    {
      max_points = 10;
      num_points = 0;
      points = new Intersection[max_points];
    }

    ~IntersectionChain()
    {
      delete points;
    }

    void addIntersection();

    void bundle_with_bundle(Bundle *, Bundle *);

    void streamline_with_bundle(Streamline *, Bundle *);

    void streamline_with_streamline(Streamline *, Streamline *);

    void segment_with_streamline(float, float, float, float,
                                 Streamline *, Streamline *);

    void add_point(float, float, Streamline *);

    void from_streamline(Streamline *);

    void draw(Window2d *);

    int q_num_points()
    { return num_points; }

    friend class Intersection;
};

int segment_with_segment(float, float, float, float,
                         float, float, float, float,
                         float *, float *);


#endif /* _INTERSECTION_CLASS_ */

