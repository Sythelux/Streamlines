//
//  classes for point repulsion
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

#include "sd_vfield.h"
#include "window.h"

#ifndef _REPEL_CLASS_
#define _REPEL_CLASS_

class CellEntry
{
    SamplePoint *sample;
    CellEntry *next;
public:
    CellEntry(SamplePoint *s)
    {
      sample = s;
      next = NULL;
    }

    friend class RepelTable;
};


class RepelTable
{
    float radius;
    float x_scale, y_scale;
    float dist_max;
    CellEntry ***cells;    /* 2D array of pointers to SamplePoint's */
    int x_wrap;            /* number of cells in x */
    int y_wrap;            /* number of cells in y */
    VectorField *vf;
public:
    RepelTable(VectorField *, float);

    ~RepelTable();

    void delete_cell_entries();

    void add_endpoints(Streamline *, int, int);

    void add_all_points(Streamline *);

    void remove_endpoints(Streamline *);

    Bundle *repel(Bundle *, float, float);

    SamplePoint *find_nearest(float, float);

    void find_new_centers(Streamline *, Streamline *, Streamline *, float,
                          float &, float &, float &, float &);

    friend class Streamline;

    friend class CellEntry;
};


#endif /* _REPEL_CLASS_ */

