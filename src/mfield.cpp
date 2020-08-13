/*

Create various vector fields.

Version 0.5

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

#define CIRCLE     1
#define SADDLE     2
#define SOURCE     3
#define SINK       4
#define SADDLE2    5
#define ELECTRO    6
#define CONSTANT   7
#define CYLINDER   8
#define NOISE      9

int xsize = 64;
int ysize = 64;
int zsize = 1;
int Rank = 2;

float noise_cycles = 10.0;
float circulation = 0.0;

char *myname;

void usage();
void init_noise();
void noise_gradient(float, float, float&, float&);
void electrostatic(float, float, float&, float&);
void cylinder_flow(float, float, float&, float&);

const int ncharges = 2;
float charge[ncharges] = {0.01, -0.01};
float cx[ncharges] = {0.3, -0.3};
float cy[ncharges] = {0.0, 0.0};

float xmin = -0.5;
float xmax =  0.5;
float ymin = -0.5;
float ymax =  0.5;


/******************************************************************************
Main routine.
******************************************************************************/

void main(int argc, char *argv[])
{
  char          *file;
  float         *VectorField;
  register float x, y;
  register int   i, j;
  int            fd, cc;
  char           str[80];
  char *s;
  int which = CIRCLE;
  float angle;
  int rotate_flag = 0;
  int lic_style = 0;    /* write out LIC file type? */
  float cs,sn;
  float fx,fy;

  myname = argv[0];

  while(--argc > 0 && (*++argv)[0]=='-') {
    for(s = argv[0]+1; *s; s++)
      switch(*s) {
	case 's':
	  xsize = atoi (*++argv);
	  ysize = atoi (*++argv);
	  argc -= 2;
	  break;
        case 'f':
	  which = atoi (*++argv);
          argc -= 1;
          break;
        case 'r':
	  angle = atof (*++argv);
          rotate_flag = 1;
          argc -= 1;
          break;
        case 'l':
          lic_style = 1;
          break;
        case 'n':
	  noise_cycles = atof (*++argv);
          which = NOISE;
          argc -= 1;
          break;
        case 'c':
	  circulation = atof (*++argv);
          which = CYLINDER;
          argc -= 1;
          break;
        case 'm':
	  xmin = atof (*++argv);
	  xmax = atof (*++argv);
	  ymin = atof (*++argv);
	  ymax = atof (*++argv);
          argc -= 4;
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

  if (rotate_flag) {
    float theta = (angle * M_PI) / 180;
    cs = cos(theta);
    sn = sin(theta);
  }

  /* calculate vector field */

  if (which == NOISE)
    init_noise();

  for (j = 0; j < ysize; j++) {
    for (i = 0; i < xsize; i++) {

      /* position in field */

      float t;
      t = (float)i/(float)(xsize-1);
      x = xmin + t * (xmax - xmin);

      t = (float)j/(float)(ysize-1);
      y = ymin + t * (ymax - ymin);

      /* select field value */
      switch (which) {
        case CIRCLE:
          fx = y;
          fy = -x;
          break;
        case SADDLE:
          fx = y;
          fy = x;
          break;
        case SOURCE:
          fx = x;
          fy = y;
          break;
        case SINK:
          fx = -x;
          fy = -y;
          break;
        case SADDLE2:
          fx = x;
          fy = -y;
          break;
        case ELECTRO:
          electrostatic (x, y, fx, fy);
          break;
        case CONSTANT:
          fx = 0.0;
          fy = 1.0;
          break;
        case NOISE:
          noise_gradient (x, y, fx, fy);
          break;
        case CYLINDER:
          cylinder_flow (x, y, fx, fy);
          break;
        default:
          fprintf (stderr, "Bad switch: %d\n", which);
      }

      /* possibly rotate vector */
      if (rotate_flag) {
        float xx = fx;
        float yy = fy;
        fx =  cs * xx + sn * yy;
        fy = -sn * xx + cs * yy;
      }

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
  fprintf (stderr, "         [-f field_type]\n");
  fprintf (stderr, "         [-s xsize ysize]\n");
  fprintf (stderr, "         [-r angle_in_degrees]\n");
  fprintf (stderr, "         [-n noise_cycles]\n");
  fprintf (stderr, "         [-c circulation]\n");
  fprintf (stderr, "         [-l] (LIC file)\n");
  fprintf (stderr, "         [-m xmin xmax ymin ymax]\n");
  fprintf (stderr, "\n");
  fprintf (stderr, "field_type:\n");
  fprintf (stderr, "1 = circle (default)\n");
  fprintf (stderr, "2 = saddle\n");
  fprintf (stderr, "3 = source\n");
  fprintf (stderr, "4 = sink\n");
  fprintf (stderr, "5 = other saddle\n");
  fprintf (stderr, "6 = electrostatic charges\n");
  fprintf (stderr, "7 = constant vector\n");
  fprintf (stderr, "8 = flow around cylinder\n");
}

/* primes for hash */

#define  PR1    17
#define  PR2   101
#define  PR3  1999

/* table of random values and vectors for noise texture */

#define  TABLE_MAX  512
static float rand_vals[TABLE_MAX];


/******************************************************************************
Initialize the noise tables.
******************************************************************************/

void init_noise()
{
  /* table of random values in [0,1] */

  for (int i = 0; i < TABLE_MAX; i++)
    rand_vals[i] = drand48();
}


/******************************************************************************
Compute noise at a given point.

Entry:
  x,y - point at which to get noise value

Exit:
  returns value of noise at this point
******************************************************************************/

float noise(float x, float y)
{
  int hash;
  float n00,n01,n10,n11;

  x *= noise_cycles;
  y *= noise_cycles;

  int i = (int) floor(x);
  int j = (int) floor(y);

  float tx = x - i;
  float ty = y - j;
  tx = (2 * tx - 3) * tx * tx + 1;
  ty = (2 * ty - 3) * ty * ty + 1;

  int i0 = i * PR1;
  int j0 = j * PR2;
  int i1 = (i+1) * PR1;
  int j1 = (j+1) * PR2;

#define hash_value(a,b,result) hash = (a+b) % TABLE_MAX; \
                               if (hash < 0) hash += TABLE_MAX; \
                               result = rand_vals[hash];

  hash_value (i0, j0, n00);
  hash_value (i0, j1, n01);
  hash_value (i1, j0, n10);
  hash_value (i1, j1, n11);

  float n0 = n00 * ty + n01 * (1 - ty);
  float n1 = n10 * ty + n11 * (1 - ty);
  float n  =  n0 * tx +  n1 * (1 - tx);

  return (n);
}


/******************************************************************************
Compute the gradient of noise at a given point.

Entry:
  x,y - point at which to get noise gradient

Exit:
  fx,fy - gradient of noise at this point
******************************************************************************/

void noise_gradient(float x, float y, float& fx, float& fy)
{
  float epsilon = 0.00001;

  float nx0 = noise (x - epsilon, y);
  float nx1 = noise (x + epsilon, y);
  float ny0 = noise (x, y - epsilon);
  float ny1 = noise (x, y + epsilon);

  fx = (nx1 - nx0) / epsilon;
  fy = (ny1 - ny0) / epsilon;
}


/******************************************************************************
Compute electrostatic potential at a point.

Entry:
  x,y - point at which to measure potential

Exit:
  fx,fy - potential at point
******************************************************************************/

void electrostatic(float x, float y, float& fx, float& fy)
{
  float xsum = 0;
  float ysum = 0;

  for (int i = 0; i < ncharges; i++) {
    float dx = x - cx[i];
    float dy = y - cy[i];

    float r2 = dx * dx + dy * dy;
    float r = sqrt (r2);

    xsum += charge[i] / r2 * (dx / r);
    ysum += charge[i] / r2 * (dy / r);
  }

  fx = xsum;
  fy = ysum;
}


/******************************************************************************
Compute flow around a cylinder.

Entry:
  x,y - point at which to measure velocity

Exit:
  vx,vy - velocity at point
******************************************************************************/

void cylinder_flow(float x, float y, float& vx, float& vy)
{
  float U = 0.5;
  float a = 0.25;

  float r = sqrt (x*x + y*y);

  float theta;
  if (x == 0 && y == 0)
    theta = 0;
  else
    theta = atan2 (y, x);

  float circ = circulation / (2 * M_PI * r);
  float u_radius =  U * (1 - (a*a)/(r*r)) * cos(theta);
  float u_theta  = -U * (1 + (a*a)/(r*r)) * sin(theta) - circ;

  vx = u_radius * cos(theta) - u_theta * sin(theta);
  vy = u_radius * sin(theta) + u_theta * cos(theta);
}

