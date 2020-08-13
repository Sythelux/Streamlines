                   Image-Guided Streamline Placement

                           Code Version 0.5

                    Documentation author: Greg Turk

Disclaimer
----------

>>> SUPPORT FOR THIS CODE WILL BE LIMITED OR NON-EXISTENT. <<<

Why?  I am starting as an Assistant Professor at Georgia Tech in September
1996, and I know that this will make me incredibly busy.  Code support will
be near the bottom of the queue for me.  This means that bug fixes, more
detailed documentation and new releases of the code are all unlikely to
happen in the near future.  I truly wish that I could spend more time
smoothing out this code, but it does not look like this will happen in
the near future.

Here is the copyright notice for the code:

  Copyright (c) 1996 The University of North Carolina.  All rights reserved.   
  
  Permission to use, copy, modify and distribute this software and its   
  documentation for any purpose is hereby granted without fee, provided   
  that the above copyright notice and this permission notice appear in   
  all copies of this software and that you do not sell the software.   
    
  The software is provided "as is" and without warranty of any kind,   
  express, implied or otherwise, including without limitation, any   
  warranty of merchantability or fitness for a particular purpose.   

Overview
--------

This text accompanies public-domain code for performing streamline placement
in 2D vector fields.  The techniques used are described in the following
article:

  "Image-Guided Streamline Placement"
  Greg Turk and David Banks
  SIGGRAPH 96 Conference Proceedings
  Computer Graphics Annual Conference Series 1996

Here is the abstract of this paper:

  Accurate control of streamline density is key to producing several
  effective forms of visualization of 2-dimensional vector fields. We
  introduce a technique that uses an energy function to guide the placement
  of streamlines at a specified density. This energy function uses a
  low-pass filtered version of the image to measure the difference between
  the current image and the desired visual density. We reduce the energy
  (and thereby improve the placement of streamlines) by (1) changing the
  positions and lengths of streamlines, (2) joining streamlines that nearly
  abut, and (3) creating new streamlines to fill sufficiently large gaps.
  The entire process is iterated to produce streamlines that are neither too
  crowded nor too sparse. The resulting streamlines manifest a more
  hand-placed appearance than do regularly- or randomly-placed streamlines.
  Arrows can be added to the streamlines to disambiguate flow direction, and
  flow magnitude can be represented by the thickness, density, or intensity
  of the lines. 

This distribution contains source code for four programs.  Here are the
programs and a brief description of each:

  mfield  - make simple test vector field files
  noise   - create vector field noise files
  stplace - use optimization to create streamlines for a given vector field
  stdraw  - draw images of streamlines that were created using "stplace"

In addition to the above code, the directory "data" contains several
sample vector field files.

Installation
------------

This distribution of code includes documentation and three directories.
These directories are:

  libs - contains library routines
  src  - source code for all programs
  data - sample vector fields and streamlines

In addition to the programs, a script called "arrrows.csh" is included
for embossing arrows with dropshadows over another image.  This script
was used to make Figure 7 in our paper (wind velocity over Australia).

All of the programs are written in C++, so you need a C++ compiler to
create the executables.  Some of the programs run with the X11 window
system.  The programs were developed on a Silicon Graphics workstation, but
they are written in fairly vanilla C++ and should work with little or no
modification on other platforms.

To make the programs, type the following in the main directory:

  cd libs
  make
  cd ../src
  make

This should create four executables in the "src" directory:
stplace, stdraw, mfield, vnoise.

See the file "Programs.txt" for documentation of these programs.

<end of document>

