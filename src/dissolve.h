// Traverse a rectangle of pixels in a random order.  From Mike Morton's
// contribution to Graphics Gems (original volume).
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


#ifndef _DISSOLVE_CLASS_
#define _DISSOLVE_CLASS_


class Dissolve
{
    int width, height;
    int seq;
    int regwidth;
    int mask;
    int initial_value;
public:
    Dissolve(int w, int h);

    int next_value();

    void set_initial_value(int val)
    {
      if (val != 0)
        initial_value = val;
      else
        initial_value = 1;
    }

    void new_position(int &, int &);
};

#endif /* _DISSOLVE_CLASS_ */

