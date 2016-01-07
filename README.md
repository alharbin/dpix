# Introduction #

This is dpix, a viewer and set of libraries to render NPR line drawings.
dpix is an implementation of algorithms presented in the following papers:

"Self-Similar Texture for Coherent Line Stylization," _Pierre Benard,
Forrester Cole, Aleksey Golovinskiy, Adam Finkelstein, NPAR 2010_

"Fast High-Quality Line Visibility," _Forrester Cole and Adam Finkelstein,
Proceedings of I3D 2009_

"Directing Gaze in 3D Models with Stylized Focus," _Forrester Cole,
Doug DeCarlo, Adam Finkelstein, Kenrick Kin, Keith Morley, and
Anthony Santella, Eurographics Symposium on Rendering 2006_

This distribution contains:

> _dpix:_
> > The main application.


> _libnpr:_
> > A rendering library for NPR lines. This is where most of the action is.


> _libgq:_
> > Qt-based utility library for OpenGL rendering. Handles GLSL,
> > framebuffer objects, vertex buffer objects, etc.


> _libcda:_
> > Simple library for reading COLLADA files. Includes support for a few
> > special tags used by the sketchup exporter.


> _libcda/sketchup\_exporter:_
> > A set of ruby scripts for Google SketchUp. Using these scripts is the
> > best way to prepare models for use with dpix.


> _samples:_
> > A few sample models, scenes, styles, and textures for use with dpix.


> _slam:_
> > Example code and textures for self-similar line artmaps. See slam/README.


Also included is source from the following libraries:

_zlib:_

> A free compression library.
> `http://www.zlib.net/`

_libqglviewer:_
> A Qt-based viewer library.
> `http://www.libqglviewer.com/`

_GLee_
> A light-weight OpenGL extension manager.
> `http://elf-stone.com/glee.php`

_matio_
> Library for processing MATLAB files.
> `http://sourceforge.net/projects/matio/`


# How to use #

**You must have Qt 4.3 or higher installed prior to building dpix.**

You can download the most recent Qt distribution here:
[http://www.qtsoftware.com/downloads](http://www.qtsoftware.com/downloads)

dpix is currently known to work on Windows XP or later, Mac OS 10.5 or later,
and Linux, with an NVIDIA 8800 series or higher GPU. Other configurations
may work as well, but have not been tested.

## Windows: ##
A Visual Studio 2008 solution file is provided. You can generate the project
files by typing `qmake -r` in the root directory. Distributing project files
caused portability issues since the project files depend on the particular
location of Qt.

## Mac and Linux: ##
The usual procedure of `qmake -r` and `make` should work.
On Mac, you may need to use `qmake -r -spec macx-g++` to create Makefiles,
rather than Xcode projects. Building with Xcode should be possible, but has
not been tested.

To compile in release mode, run:
`qmake -r "CONFIG += release"`

## Running ##
Once dpix is built, a binary should appear in `dpix/debug` or
`dpix/release`. Run this binary, and try opening one of the sample scenes
in `samples/scenes` to make sure everything is working properly.

If you encounter problems, email fcole@cs.princeton.edu with a complete
problem report and I will try to help out.

# Acknowledgments #

Please see the COPYING file for license information.

Forrester Cole, Michael Burns, Keith Morley, Adam Finkelstein, and Pierre
Benard wrote the code.