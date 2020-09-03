// Class of windows containing 2-D drawings.
//
// Greg Turk, September 1994

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

#ifndef _WINDOW2D_CLASS_
#define _WINDOW2D_CLASS_

#include <X11/Xlib.h>
#include <X11/Xatom.h>

class Window2d
{

    int xorg, yorg;
    int window_ptr;
    int bg_color;

    int screen;

    unsigned long foregrnd, backgrnd;
    Cursor root_cursor;
    int defDepth;
    Visual *defVisual;
    Colormap defColormap;

    XPoint *points;
    int npoints;
    int max_points;

    float vscale;

    XFontStruct *RegFont;
    XFontStruct *SmallFont;
    XFontStruct *BigFont;

    GC gc_global;
    GC gc_reg_font;
    GC gc_small_font;
    GC gc_big_font;

    int num_colors;
    unsigned long pixels[256];
    int color_index;
    int psize;

    GC gc;
    XGCValues gcvalues;
    XColor colors[256];

    void (*press1)(Window2d *, int, int);

    void (*press2)(Window2d *, int, int);

    void (*press3)(Window2d *, int, int);

    void (*release1)(Window2d *, int, int);

    void (*release2)(Window2d *, int, int);

    void (*release3)(Window2d *, int, int);

    void (*escape_callback)(Window2d *);

    void (*redraw_callback)(Window2d *);

public:

    Display *display;
    Window root;
    int xsize, ysize;

    void setup_window();

    void do_callbacks(char *, XEvent *);

    Window2d()
    {
      xsize = ysize = 300;
      xorg = 30;
      yorg = 200;
      bg_color = 0;
      psize = 1;
      setup_window();
    }

    Window2d(int w, int h)
    {
      xsize = w;
      ysize = h;
      xorg = 30;
      yorg = 200;
      bg_color = 0;
      psize = 1;
      setup_window();
    }

    Window2d(int w, int h, int xorigin, int yorigin)
    {
      xsize = w;
      ysize = h;
      xorg = xorigin;
      yorg = yorigin;
      bg_color = 0;
      psize = 1;
      setup_window();
    }

    Window window_pointer()
    { return root; }

    void escape(void (*func)(Window2d *))
    { escape_callback = func; }

    void redraw(void (*func)(Window2d *))
    { redraw_callback = func; }

    void left_down(void (*func)(Window2d *, int, int))
    { press1 = func; }

    void middle_down(void (*func)(Window2d *, int, int))
    { press2 = func; }

    void right_down(void (*func)(Window2d *, int, int))
    { press3 = func; }

    void left_up(void (*func)(Window2d *, int, int))
    { release1 = func; }

    void middle_up(void (*func)(Window2d *, int, int))
    { release2 = func; }

    void right_up(void (*func)(Window2d *, int, int))
    { release3 = func; }

    void check_events();

    int left_button();

    int middle_button();

    int right_button();

    void cursor_pos(float *x, float *y);

    void cursor_ipos(int *x, int *y);

    void setsize(int w, int h)
    {
      xsize = w;
      ysize = h;
    }

    void getsize(int *w, int *h)
    {
      *w = xsize;
      *h = ysize;
    }

    void set_vscale(float s)
    { vscale = s; }

    void ibackground(int r, int g, int b)
    { bg_color = (b << 16) | (g << 8) | r; }

    void background(float r, float g, float b)
    {
      ibackground((int) (r * 255), (int) (g * 255), (int) (b * 255));
    }

    void gray_ramp();

    void clear()
    {
      XClearWindow(display, root);
      XFlush(display);
    }

    void flush()
    { XFlush(display); }

    void map()
    { XMapWindow(display, root); }

    void prefpos(int x, int y)
    {
      xorg = x;
      yorg = y;
    }

    void setcolor(float, float, float);

    void seticolor(int, int, int);

    void makecolor(int, float, float, float);

    void makeicolor(int, int, int, int);

    void set_color_index(int);

    void line(float, float, float, float);

    void thick_line(float, float, float, float, int);

    void iline(int, int, int, int);

    void thick_iline(int, int, int, int, int);

    void polygon_start();

    void polygon_vertex(float, float);

    void polygon_fill();

    void circle(float, float, float, float);

    void set_pixel_size(int);

    void writepixel(int, int, int);

    void draw_image(int, int, unsigned char *);

    void draw_offset_image(int, int, unsigned char *, int, int);
};

extern void check_events();

#endif

