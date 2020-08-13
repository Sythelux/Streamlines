/*

Window routines written on top of X11.

Greg Turk, September 1994

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
#include <stdlib.h>
#include <math.h>
#include "window.h"

static void quit_proc();
static void draw_proc();

static Window2d *window_list[20];
static int num_windows = 0;


/******************************************************************************
Set the current color index.

Entry:
  index - index to set the current color to  
******************************************************************************/

void Window2d::set_color_index(int index)
{
  index = (int) ((index / 255.0) * (num_colors - 1));
  color_index = index;
}


/******************************************************************************
Make a color.

Entry:
  index - index of color that we're making
  r,g,b - red, green and blue color values, in [0,1]
******************************************************************************/

void Window2d::makecolor(int index, float r, float g, float b)
{
  XColor col;

  index = (int) ((index / 255.0) * (num_colors - 1));

  if (index < 0 || index >= num_colors) {
    printf ("makecolor index out of bounds: %d\n", index);
    return;
  }

  col.red   = (unsigned short) (65535 * r);
  col.green = (unsigned short) (65535 * g);
  col.blue  = (unsigned short) (65535 * b);
  col.pixel = colors[index].pixel;
  col.flags = DoRed | DoGreen | DoBlue;
  XStoreColor (display, defColormap, &col);
}


/******************************************************************************
Make a color, integer version.

Entry:
  index - index of color that we're making
  r,g,b - red, green and blue color values, in [0,255]
******************************************************************************/

void Window2d::makeicolor(int index, int r, int g, int b)
{
  XColor col;

  index = (int) ((index / 255.0) * (num_colors - 1));

  if (index < 0 || index >= num_colors) {
    printf ("makecolor index out of bounds: %d\n", index);
    return;
  }

  col.red   = (unsigned short) (65535 * (r / 255.0));
  col.green = (unsigned short) (65535 * (g / 255.0));
  col.blue  = (unsigned short) (65535 * (b / 255.0));
  col.pixel = colors[index].pixel;
  col.flags = DoRed | DoGreen | DoBlue;
  XStoreColor (display, defColormap, &col);
}


/******************************************************************************
Draw a thick line in the current color.

Entry:
  x1,y1 - coordinate of one end of line
  x2,y2 - other end of line
  thick - thickness in pixels
******************************************************************************/

void Window2d::thick_iline(int x1, int y1, int x2, int y2, int thick)
{
  int color = color_index;

  gcvalues.line_width = thick;
  gcvalues.foreground = colors[color].pixel;
  XChangeGC (display, gc, GCForeground|GCLineWidth, &gcvalues);
  XDrawLine (display, root, gc, x1, ysize - y1 - 1, x2, ysize - y2 - 1);

  XFlush (display);
}


/******************************************************************************
Draw a line in the current color.

Entry:
  x1,y1 - coordinate of one end of line
  x2,y2 - other end of line
******************************************************************************/

void Window2d::iline(int x1, int y1, int x2, int y2)
{
  int color = color_index;

  gcvalues.foreground = colors[color].pixel;
  XChangeGC (display, gc, GCForeground, &gcvalues);
  XDrawLine (display, root, gc, x1, ysize - y1 - 1, x2, ysize - y2 - 1);

  XFlush (display);
}


/******************************************************************************
Draw a line in the current color.

Entry:
  x1,y1 - coordinate of one end of line
  x2,y2 - other end of line
******************************************************************************/

void Window2d::line(float x1, float y1, float x2, float y2)
{
  int color = color_index;

  int a1 = (int) (x1 * xsize);
  int b1 = (int) ((1 - y1 * vscale) * ysize);
  int a2 = (int) (x2 * xsize);
  int b2 = (int) ((1 - y2 * vscale) * ysize);

  gcvalues.foreground = colors[color].pixel;
  XChangeGC (display, gc, GCForeground, &gcvalues);
  XDrawLine (display, root, gc, a1, b1, a2, b2);

  XFlush (display);
}


/******************************************************************************
Draw a thick line in the current color.

Entry:
  x1,y1 - coordinate of one end of line
  x2,y2 - other end of line
  thick - thickness in pixels
******************************************************************************/

void Window2d::thick_line(float x1, float y1, float x2, float y2, int thick)
{
  int color = color_index;

  int a1 = (int) (x1 * xsize);
  int b1 = (int) ((1 - y1 * vscale) * ysize);
  int a2 = (int) (x2 * xsize);
  int b2 = (int) ((1 - y2 * vscale) * ysize);

  gcvalues.line_width = thick;
  gcvalues.foreground = colors[color].pixel;
  XChangeGC (display, gc, GCForeground|GCLineWidth, &gcvalues);
  XDrawLine (display, root, gc, a1, b1, a2, b2);

  XFlush (display);
}


/******************************************************************************
Begin defining a polygon.
******************************************************************************/

void Window2d::polygon_start()
{
  max_points = 5;
  points = new XPoint[max_points];
  npoints = 0;
}


/******************************************************************************
Add a new vertex to a polygon that is being defined.

Entry:
  x,y - new vertex to add to polygon
******************************************************************************/

void Window2d::polygon_vertex(float x, float y)
{
  /* allocate more points if necessary */

  if (npoints >= max_points) {
    max_points *= 2;
    XPoint *new_points = new XPoint[max_points];
    for (int i = 0; i < npoints; i++) {
      new_points[i].x = points[i].x;
      new_points[i].y = points[i].y;
    }
    delete points;
    points = new_points;
  }

  points[npoints].x = (int) (x * xsize);
  points[npoints].y = (int) ((1 - y) * ysize);
  npoints++;
}


/******************************************************************************
Fill in the currently defined polygon.
******************************************************************************/

void Window2d::polygon_fill()
{
  int color = color_index;

  gcvalues.foreground = colors[color].pixel;
  XChangeGC (display, gc, GCForeground, &gcvalues);
  XFillPolygon (display, root, gc, points, npoints, Complex, CoordModeOrigin);
  XFlush (display);

  delete points;
  npoints = 0;
}


/******************************************************************************
Draw a circle of a given radius.
******************************************************************************/

void Window2d::circle (float x, float y, float radius, float intensity)
{
  int color = (int) (intensity * (num_colors - 1));
  int xx = (int) (x * xsize);
  int yy = (int) ((1 - y) * ysize);
  int r = radius * xsize;

  gcvalues.foreground = colors[color].pixel;
  XChangeGC (display, gc, GCForeground, &gcvalues);
  XDrawArc (display, root, gc, xx-r, yy-r, r+r, r+r, 0, 360 * 64);
}


/******************************************************************************
Create a gray color ramp.
******************************************************************************/

void Window2d::gray_ramp()
{
  /* create grey ramp */

  for (int i = 0; i < num_colors; i++) {
    float t = 65535 * i / (num_colors - 1.0);
    colors[i].red   = (unsigned short) t;
    colors[i].green = (unsigned short) t;
    colors[i].blue  = (unsigned short) t;
    colors[i].pixel = pixels[i];
    colors[i].flags = DoRed | DoGreen | DoBlue;
  }

  XStoreColors (display, defColormap, colors, num_colors);
}


/******************************************************************************
Set the size of pixels.
******************************************************************************/

void Window2d::set_pixel_size(int size)
{
  psize = size;
}


/******************************************************************************
Write a pixel to the screen.
******************************************************************************/

void Window2d::writepixel (int x, int y, int color)
{
  color = (int) ((color / 255.0) * (num_colors - 1));

  gcvalues.foreground = colors[color].pixel;
  XChangeGC (display, gc, GCForeground, &gcvalues);
  XFillRectangle (display, root, gc, x * psize, y * psize, psize, psize);
}


/******************************************************************************
Set up the window.
******************************************************************************/

void Window2d::setup_window()
{
  int status;
  unsigned long plane_masks[8];
  XSetWindowAttributes attrib;
  static char default_font[] = "8x13";

  psize = 1;

  /* Open a connection to the display. */
  if ((display = XOpenDisplay (NULL)) == NULL) {
    printf ("Couldn't open display:\n");
    printf ("Make sure X11 is running and DISPLAY is set correctly\n");
    exit (-1);
  }
  screen = XDefaultScreen (display);
  num_colors = 30;

/*
XSetErrorHandler (my_error_handler);
*/

  /* set up foreground, background colors (to Xdefaults, if any) */
  if (XDisplayCells (display, screen) > 2)	/* this is a color display */
    defColormap = XDefaultColormap (display, screen);

  foregrnd = XWhitePixel (display, screen);
  backgrnd = XBlackPixel (display, screen);

  /* set up default depth, visual params */
  defDepth = XDefaultDepth (display, screen);
  defVisual = XDefaultVisual (display, screen);

  /* set up fonts */

  RegFont   = XLoadQueryFont (display, "8x13");
  SmallFont = XLoadQueryFont (display, "6x10");
  BigFont   = XLoadQueryFont (display, "9x15");

  if (RegFont == NULL || SmallFont == NULL || BigFont == NULL) {
    printf ("Couldn't find all fonts.\n");
  }

  /* create root cursor (left arrow) */
  root_cursor = XCreateFontCursor (display, 68);

  /* create root window */
  attrib.background_pixel = backgrnd;
  attrib.border_pixel = foregrnd;
  attrib.override_redirect = False;
  /*
  attrib.override_redirect = True;
  */
  attrib.event_mask = ButtonPressMask | ButtonReleaseMask | ExposureMask |
		      StructureNotifyMask;
  attrib.cursor = root_cursor;
  root = XCreateWindow (display, RootWindow(display, screen),
	      xorg, yorg,
	      xsize, ysize, 1, defDepth, InputOutput,
	      defVisual, CWBackPixel|CWBorderPixel|CWOverrideRedirect|
	      CWEventMask|CWCursor, &attrib);
  XChangeProperty (display, root, XA_WM_NAME, XA_STRING, 8, PropModeReplace,
      (unsigned char *) "Icon", 5);

  /* set up some graphics contexts */

  gcvalues.foreground = foregrnd;
  gcvalues.background = backgrnd;
  gcvalues.line_width = 1;
  gcvalues.font = RegFont->fid;
  gc_reg_font = XCreateGC (display, root,
		  GCForeground|GCBackground|GCLineWidth|GCFont,
		  &gcvalues);
  gcvalues.font = SmallFont->fid;
  gc_small_font = XCreateGC (display, root,
		  GCForeground|GCBackground|GCLineWidth|GCFont,
		  &gcvalues);
  gcvalues.font = BigFont->fid;
  gc_big_font = XCreateGC (display, root,
		  GCForeground|GCBackground|GCLineWidth|GCFont,
		  &gcvalues);

  /* set up drawing parameters */

  status = XAllocColorCells (display, defColormap,
	   0, plane_masks, 0, pixels, num_colors);

  if (status == 0) {
    printf ("bad status from XAllocColorCells\n");
    return;
  }

  gray_ramp();

#if 0
  /* create grey ramp */

  for (int i = 0; i < num_colors; i++) {
    t = 65535 * i / (num_colors - 1.0);
    colors[i].red   = (unsigned short) t;
    colors[i].green = (unsigned short) t;
    colors[i].blue  = (unsigned short) t;
    colors[i].pixel = pixels[i];
    colors[i].flags = DoRed | DoGreen | DoBlue;
  }

  XStoreColors (display, defColormap, colors, num_colors);
#endif

  gcvalues.foreground = foregrnd;
  gcvalues.background = backgrnd;
  gc = XCreateGC (display, root, GCForeground|GCBackground, &gcvalues);

  /* set all callbacks to NULL */

  press1 = NULL;
  press2 = NULL;
  press3 = NULL;
  release1 = NULL;
  release2 = NULL;
  release3 = NULL;
  escape_callback = NULL;
  redraw_callback = NULL;

  /* save a pointer to this window */
  window_list[num_windows++] = this;
}


/******************************************************************************
Draw an image in the window.

Entry:
  width      - width of image to draw
  height     - height of image
  image_data - pixel colors in [0,255]
******************************************************************************/

void Window2d::draw_image(int width, int height, unsigned char *image_data)
{
  XImage *image;
  unsigned char *data;

  data = new unsigned char [width * height];

  /* map [0,255] into range [0,num_colors] */

  for (int j = 0; j < height; j++) {
    int j2 = height - j - 1;  /* used to flip image vertically */
    for (int i = 0; i < width; i++) {
      float t = image_data[i+j*width] / 256.0;
      int col = (int) pixels [(int) (t * num_colors)];
      data[i+j2*width] = col;
    }
  }

  image = XCreateImage (display, defVisual, defDepth, ZPixmap, 0,
                        (char *) data, width, height, 8, 0);

  XPutImage (display, root, gc, image, 0, 0, 0, 0, width, height);
  XFlush (display);

  delete data;
}


/******************************************************************************
Draw an image in the window that is offset from the origin.

Entry:
  width      - width of image to draw
  height     - height of image
  image_data - pixel colors in [0,255]
  xoff,yoff  - offset coordinates
******************************************************************************/

void Window2d::draw_offset_image(
  int width,
  int height,
  unsigned char *image_data,
  int xoff,
  int yoff
)
{
  XImage *image;
  unsigned char *data;

  data = new unsigned char [width * height];

  /* map [0,255] into range [0,num_colors] */

  for (int j = 0; j < height; j++) {
    int j2 = height - j - 1;  /* used to flip image vertically */
    for (int i = 0; i < width; i++) {
      float t = image_data[i+j*width] / 256.0;
      int col = (int) pixels [(int) (t * num_colors)];
      data[i+j2*width] = col;
    }
  }

  image = XCreateImage (display, defVisual, defDepth, ZPixmap, 0,
                        (char *) data, width, height, 8, 0);

  XPutImage (display, root, gc, image, 0, 0, xoff, yoff, width, height);
  XFlush (display);

  delete data;
}


/******************************************************************************
Tell if the left button is down.
******************************************************************************/

int Window2d::left_button()
{
  int result;
  Window win_root;
  Window child;
  int root_x,root_y;
  int win_x,win_y;
  unsigned int keys_buttons;

  result = XQueryPointer (display, root, &win_root, &child, &root_x, &root_y,
			  &win_x, &win_y, &keys_buttons);

  if (keys_buttons & Button1Mask)
    return (1);
  else
    return (0);
}


/******************************************************************************
Tell if the middle button is down.
******************************************************************************/

int Window2d::middle_button()
{
  int result;
  Window win_root;
  Window child;
  int root_x,root_y;
  int win_x,win_y;
  unsigned int keys_buttons;

  result = XQueryPointer (display, root, &win_root, &child, &root_x, &root_y,
			  &win_x, &win_y, &keys_buttons);

  if (keys_buttons & Button2Mask)
    return (1);
  else
    return (0);
}


/******************************************************************************
Tell if the right button is down.
******************************************************************************/

int Window2d::right_button()
{
  int result;
  Window win_root;
  Window child;
  int root_x,root_y;
  int win_x,win_y;
  unsigned int keys_buttons;

  result = XQueryPointer (display, root, &win_root, &child, &root_x, &root_y,
			  &win_x, &win_y, &keys_buttons);

  if (keys_buttons & Button3Mask)
    return (1);
  else
    return (0);
}


/******************************************************************************
Return cursor position within a window.

Exit:
  x,y - position of cursor
******************************************************************************/

void Window2d::cursor_pos(float *x, float *y)
{
  int result;
  Window win_root;
  Window child;
  int root_x,root_y;
  int win_x,win_y;
  unsigned int keys_buttons;

  result = XQueryPointer (display, root, &win_root, &child, &root_x, &root_y,
			  &win_x, &win_y, &keys_buttons);

  *x = win_x / xsize;
  *y = 1 - win_y / ysize;
}


/******************************************************************************
Return cursor position within a window.

Exit:
  x,y - position of cursor
******************************************************************************/

void Window2d::cursor_ipos(int *x, int *y)
{
  int result;
  Window win_root;
  Window child;
  int root_x,root_y;
  unsigned int keys_buttons;

  result = XQueryPointer (display, root, &win_root, &child, &root_x, &root_y,
			  x, y, &keys_buttons);

  *y = ysize - *y - 1;
}


/******************************************************************************
Event handler for drawing area.
******************************************************************************/

void Window2d::do_callbacks (char *data, XEvent *event)
{
  XButtonEvent *button;

  button = (XButtonEvent *) event;
  int x = button->x;
  int y = this->ysize - button->y - 1;
  
  switch(event->type) {

    case ButtonPress:
      switch (button->button) {
	case Button1:
	  if (press1)
	    (*press1)(this, x, y);
	  break;
	case Button2:
	  if (press2)
	    (*press2)(this, x, y);
	  break;
	case Button3:
	  if (press3)
	    (*press3)(this, x, y);
	  break;
      }
      break;

    case ButtonRelease:
      switch (button->button) {
	case Button1:
	  if (release1)
	    (*release1)(this, x, y);
	  break;
	case Button2:
	  if (release2)
	    (*release2)(this, x, y);
	  break;
	case Button3:
	  if (release3)
	    (*release3)(this, x, y);
	  break;
      }
      break;

    case Expose:
      if (redraw_callback)
	(*redraw_callback)(this);
      break;

    default:
      break;
  }
}


/******************************************************************************
Check to see if events need processing.
******************************************************************************/

void check_events()
{
  int found = 0;
  XEvent event;
  XAnyEvent *e;

  for (int i = 0; i < num_windows; i++) {

    Window2d *win = window_list[i];

    while (XPending (win->display)) {

      XNextEvent (win->display, &event);
      e = (XAnyEvent *) &event;

      if (e->window == win->root) {
	win->do_callbacks (NULL, &event);
	found = 1;
      }
      else
	printf ("check_events(): Oops, unknown event.\n");
    }
  }
}

#if 0

/******************************************************************************
Specify the text to appear on the main window's icon.
******************************************************************************/

void window_name(char *name)
{
  XChangeProperty (display, root, XA_WM_NAME, XA_STRING, 8, PropModeReplace,
      (unsigned char *) name, strlen (name));
}


/******************************************************************************
Specify upper left corner of window.
******************************************************************************/

void window_corner(int x, int y)
{
  xorg = x;
  yorg = y;
}


/******************************************************************************
My own error handler.
******************************************************************************/

void my_error_handler(Display *display, XErrorEvent *event)
{
  float x;

  printf ("You got an error.\n");
  x = 0;
  x = 1 / x;
}


/******************************************************************************
Draw a rectangle of a given color.
******************************************************************************/

void draw_rectangle (int x1, int y1, int x2, int y2, int color)
{
  int temp;
  int dx,dy;

  color = (int) ((color / 255.0) * (num_colors - 1));

  if (x1 > x2) {
    temp = x1;
    x1 = x2;
    x2 = temp;
  }

  if (y1 > y2) {
    temp = y1;
    y1 = y2;
    y2 = temp;
  }

  dx = x2 - x1 + 1;
  dy = y2 - y1 + 1;

  gcvalues.foreground = colors[color].pixel;
  XChangeGC (display, gc, GCForeground, &gcvalues);
  XFillRectangle (display, root, gc, x1 * psize, y1 * psize,
		  dx * psize, dy * psize);
}


/******************************************************************************
Draw a circle of a given radius.
******************************************************************************/

void draw_circle (int x, int y, int r, int color)
{
  color = (int) ((color / 255.0) * (num_colors - 1));

  gcvalues.foreground = colors[color].pixel;
  XChangeGC (display, gc, GCForeground, &gcvalues);
  XDrawArc (display, root, gc, x-r, y-r, r+r, r+r, 0, 360 * 64);
}


/******************************************************************************
Flush the pixel buffers.
******************************************************************************/

void flushbuffers()
{
  XFlush (display);
}


/******************************************************************************
Draw a string of text.
******************************************************************************/

void draw_text(char *text, int x, int y)
{
  XDrawString (display, root, gc_reg_font, x, y, text, strlen (text));
}


/******************************************************************************
Set up callback functions for button presses.
******************************************************************************/

void setup_buttons(p1,p2,p3,r1,r2,r3)
  int (*p1)();
  int (*p2)();
  int (*p3)();
  int (*r1)();
  int (*r2)();
  int (*r3)();
{
  press1 = p1;
  press2 = p2;
  press3 = p3;
  release1 = r1;
  release2 = r2;
  release3 = r3;
}


/******************************************************************************
Exit the program.
******************************************************************************/

static void quit_proc()
{
  exit(0);
}

#endif

