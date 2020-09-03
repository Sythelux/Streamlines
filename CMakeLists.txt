cmake_minimum_required(VERSION 3.16)
project(streamlines)

set(CMAKE_CXX_STANDARD 20)

include_directories(libs)
include_directories(src)

find_package(X11 REQUIRED)
link_libraries(${X11_LIBRARIES})
include_directories(${X11_INCLUDE_DIR})

add_library(lib
        libs/window.cpp
        libs/window.h
        libs/floatimage.cpp
        libs/floatimage.h
        libs/cli.cpp
        libs/cli.h
        libs/clip_line.cpp
        libs/clip_line.h
        )

add_executable(stplace
        libs/window.cpp
        libs/window.h
        libs/floatimage.cpp
        libs/floatimage.h
        libs/cli.cpp
        libs/cli.h
        libs/clip_line.cpp
        libs/clip_line.h
        src/stplace.cpp
        src/stplace.h
        src/vfield.cpp
        src/vfield.h
        src/streamline.cpp
        src/streamline.h
        src/lowpass.cpp
        src/lowpass.h
        src/repel.cpp
        src/repel.h
        src/intersect.cpp
        src/intersect.h
        src/xlines.cpp
        src/dissolve.cpp
        src/dissolve.h
        src/visparams.cpp
        src/visparams.h
        )


add_executable(stdraw
        libs/cli.cpp
        libs/cli.h
        libs/clip_line.cpp
        libs/clip_line.h
        libs/floatimage.cpp
        libs/floatimage.h
        libs/window.cpp
        libs/window.h
        src/sd_params.cpp
        src/sd_params.h
        src/sd_picture.cpp
        src/sd_picture.h
        src/sd_repel.cpp
        src/sd_repel.h
        src/sd_streamline.cpp
        src/sd_streamline.h
        src/sd_vfield.cpp
        src/sd_vfield.h
        src/stdraw.cpp
        src/stdraw.h
        )

add_executable(mfield
        src/mfield.cpp
        )

add_executable(noise
        src/noise.cpp
        )

# for compilation checks
add_executable(streamlines
        libs/cli.cpp
        libs/cli.h
        libs/clip_line.cpp
        libs/clip_line.h
        libs/floatimage.cpp
        libs/floatimage.h
        libs/window.cpp
        libs/window.h
        src/dissolve.cpp
        src/dissolve.h
        src/intersect.cpp
        src/intersect.h
        src/lowpass.cpp
        src/lowpass.h
        src/mfield.cpp
        src/noise.cpp
        src/picture.cpp
        src/picture.h
        src/repel.cpp
        src/repel.h
        src/sd_params.cpp
        src/sd_params.h
        src/sd_picture.cpp
        src/sd_picture.h
        src/sd_repel.cpp
        src/sd_repel.h
        src/sd_streamline.cpp
        src/sd_streamline.h
        src/sd_vfield.cpp
        src/sd_vfield.h
        src/stdraw.cpp
        src/stdraw.h
        src/stplace.cpp
        src/stplace.h
        src/streamline.cpp
        src/streamline.h
        src/vfield.cpp
        src/vfield.h
        src/visparams.cpp
        src/visparams.h
        src/xlines.cpp)
