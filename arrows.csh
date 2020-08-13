#!/bin/csh -f
#
#  This script takes a black & white image of arrows and mattes it on top
#  of a color image so that it appears as though the arrows float over
#  the given color image.  The arrows have drop-shadows.
#
#  This script makes use of a number of image utilities that are part
#  of the standard software on Silicon Graphics machines.
#
#  Entry:
#    arrows_image.rgb - the black and white image of arrows
#    your_color_image.rgb - color image that arrows should go on top of
#
#  Exit:
#    all_arrows.rgb - final image
#
#  David Banks and Greg Turk, May 1996
#

# select a map and arrows

set arrows = arrows_image.rgb
set map = your_color_image.rgb

# make shadows of arrows, by making a shifted version
# of arrows with bite taken out

iroll $arrows arrow_roll.rgb 5 -5
echo iroll
invert $arrows arrow_inv.rgb
echo invert
izoom arrow_roll.rgb arrow_roll_blur.rgb 1 1 -w 3
echo izoom
mult arrow_inv.rgb arrow_roll_blur.rgb arrow_shadow.rgb
echo mult
rm -f arrow_roll.rgb arrow_roll_blur.rgb arrow_inv.rgb

# make place of neither arrow nor arrow shadow

add $arrows arrow_shadow.rgb a1.rgb
echo add
invert a1.rgb not_arrows.rgb
echo invert
rm -f a1.rgb

# make dark version of map

cscale $map map_dark.rgb 127 127 127
echo cscale

# make light version of map

invert $map m1.rgb
echo invert
# cscale m1.rgb m2.rgb 127 127 127
cscale m1.rgb m2.rgb 180 180 180
echo cscale
invert m2.rgb map_light.rgb
echo invert
rm -f m1.rgb m2.rgb

# select light part inside arrows
mult map_light.rgb $arrows map_arrows.rgb
echo mult

# select dark part inside bites (arrow shadows)
mult map_dark.rgb arrow_shadow.rgb map_shadow.rgb
echo mult

# select normal part
mult $map not_arrows.rgb map_not_arrows.rgb
echo mult

rm -f map_light.rgb map_dark.rgb arrow_shadow.rgb not_arrows.rgb

# add all parts together
add map_arrows.rgb map_shadow.rgb t1.rgb
echo add
add t1.rgb map_not_arrows.rgb all_arrows.rgb
echo add
rm -f t1.rgb map_arrows.rgb map_shadow.rgb map_not_arrows.rgb

echo done

