//  Global defines and stuff

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


#define BLACK  10
#define RED    20
#define GREEN  30
#define WHITE 255

#define LEN_CHANGE   0x0001
#define MOVE_CHANGE  0x0002
#define SHORT_ONE    0x0004
#define LONG_ONE     0x0008
#define SHORT_BOTH   0x0010
#define LONG_BOTH    0x0020
#define ALL_LEN      0x0040
#define HEAD_CHANGE  0x0080
#define TAIL_CHANGE  0x0100
#define LEFT_CHANGE  0x0200
#define RIGHT_CHANGE 0x0400
#define TAPER_CHANGE 0x0800

void remove_streamline(Streamline *);

void add_streamline(Streamline *);

extern int animation_flag;
extern ofstream *anim_file;
extern int anim_index;
extern int vary_arrow_intensity;
extern float delta_step;

void postscript_draw_arrow(ofstream *, float, float, float, float, int, int);

void draw_arrow(Window2d *, float, float, float, float, int, int);

extern int verbose_flag;

