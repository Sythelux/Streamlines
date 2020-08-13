/*

Create a vector field from noise.

Version 0.5

Greg Turk, August 1995

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


#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

void usage();

int xsize = 20;
int ysize = 20;
int zsize = 1;
int Rank = 2;

static char *myname;

class Vec {
public:
  float x,y;
  Vec(float xx, float yy) {
    x = xx;
    y = yy;
  }
};

Vec ***grid;
Vec ***new_grid;


/******************************************************************************
Main routine.
******************************************************************************/

void main(int argc, char *argv[])
{
  register int   i, j;
  char          *file;
  float         *VectorField;
  int            fd, cc;
  char           str[80];
  char *s;
  int lic_style = 0;    /* write out LIC file type? */
  float fx,fy;
  int normalize = 1;
  int smoothing_steps = 40;

  myname = argv[0];

  while(--argc > 0 && (*++argv)[0]=='-') {
    for(s = argv[0]+1; *s; s++)
      switch(*s) {
	case 's':
	  xsize = atoi (*++argv);
	  ysize = atoi (*++argv);
	  argc -= 2;
	  break;
        case 'l':
          lic_style = 1;
          break;
        case 'n':
          normalize = 1 - normalize;
          break;
        case 'k':
          smoothing_steps = atoi (*++argv);
          argc -= 1;
          break;
	default:
          usage();
          exit (-1);
	  break;
      }
  }

  /* get output filename */

  if (argc > 0)
    file = *argv;
  else {
    usage();
    exit (-1);
  }

  VectorField = (float *)malloc(sizeof(float)*ysize*xsize*2);
  if (VectorField == NULL) {
    fprintf(stderr, "%s: insufficient memory for creating the field\n", myname);
    exit(-1);
  }

  /* allocate grid */

  grid = new Vec **[ysize];
  new_grid = new Vec **[ysize];
  for (j = 0; j < ysize; j++) {
    grid[j] = new Vec *[xsize];
    new_grid[j] = new Vec *[xsize];
  }

  /* place random vector values in grid */

  for (j = 0; j < ysize; j++)
    for (i = 0; i < xsize; i++) {

      float x = 2 * (drand48() - 0.5);
      float y = 2 * (drand48() - 0.5);

      if (normalize) {
        float len = sqrt (x*x + y*y);
        grid[j][i] = new Vec(x / len, y / len);
      }
      else
        grid[j][i] = new Vec(x,y);

      new_grid[j][i] = new Vec(0.0, 0.0);
    }
  
  /* smooth the random vectors */

  for (int num = 0; num < smoothing_steps; num++) {

    for (j = 0; j < ysize; j++)
      for (i = 0; i < xsize; i++) {
        int i0 = (i + xsize - 1) % xsize;
        int i1 = (i + 1) % xsize;
        new_grid[j][i]->x = grid[j][i]->x + grid[j][i0]->x + grid[j][i1]->x;
        new_grid[j][i]->y = grid[j][i]->y + grid[j][i0]->y + grid[j][i1]->y;
      }

    for (i = 0; i < xsize; i++)
      for (j = 0; j < ysize; j++) {
        int j0 = (j + ysize - 1) % ysize;
        int j1 = (j + 1) % ysize;
        grid[j][i]->x = new_grid[j][i]->x + new_grid[j0][i]->x +
                        new_grid[j1][i]->x;
        grid[j][i]->y = new_grid[j][i]->y + new_grid[j0][i]->y +
                        new_grid[j1][i]->y;
      }

    if (normalize)
      for (j = 0; j < ysize; j++)
        for (i = 0; i < xsize; i++) {
            float x = grid[j][i]->x;
            float y = grid[j][i]->y;
            float len = sqrt (x*x + y*y);
            grid[j][i] = new Vec(x / len, y / len);
          }

  }

  /* calculate vector field */

  for (j = 0; j < ysize; j++) {
    for (i = 0; i < xsize; i++) {

      fx = grid[j][i]->x;
      fy = grid[j][i]->y;

      /* save vector value */
      if (lic_style) {
        VectorField[2*((ysize - j - 1)*xsize + i) + 0] = fx;
        VectorField[2*((ysize - j - 1)*xsize + i) + 1] = fy;
      }
      else {
        VectorField[2*(j*xsize + i) + 0] = fx;
        VectorField[2*(j*xsize + i) + 1] = fy;
      }
   }
  }

  fd = open(file, O_CREAT|O_TRUNC|O_WRONLY, 0666);
  if (fd < 0) {
    fprintf(stderr, "%s: unable to open %s: %s\n", myname, file,
            strerror(errno));
    exit(-1);
  }

  sprintf (str, "VF\n%d %d %d\n%d\n", xsize, ysize, zsize, Rank);
  cc = write(fd, str, strlen(str));
  if (cc == -1) {
    fprintf (stderr, "Can't write to file.\n");
    exit (-1);
  }

  cc = write(fd, VectorField, 2*sizeof(float)*xsize*ysize);

  if (cc != 2*sizeof(float)*xsize*ysize) {
    fprintf(stderr, "%s: write returned short: %s\n", myname,
            strerror(errno));
    close(fd);
    exit(-1);
  }

  close(fd);
}


/******************************************************************************
Print out usage information.
******************************************************************************/

void usage()
{
  fprintf (stderr, "%s: { options } output_file\n", myname);
  fprintf (stderr, "        -s xsize ysize (default 20 by 20)\n");
  fprintf (stderr, "        -k smoothing_steps (default 40)\n");
}

