Minecraft Unicode Font Generator - The ultra fast generating minecraft unicode font textures utility
====================================================================================================

Disclaimer
----------
This program comes with no warranty.
You must use this program at your own risk.

Introduction
------------

Minecraft Unicode Font Generator is a utility for generating minecraft unicode font textures. 

Features
--------

Here is a list of features:

* Command-line interface
* Multi-process processing

How to get source code
----------------------

We maintain the source code at Github:
https://github.com/codehz/minecraft-unicode-font-generator

To get the latest source code, run following command::

    $ git clone https://github.com/codehz/minecraft-unicode-font-generator.git

This will create minecraft-unicode-font-generator directory in your current directory and source
files are stored there.

Dependency
----------

======================== ========================================
features                  dependency
======================== ========================================
Font Rendering           FreeType2 https://www.freetype.org/
Command Line Parser      TCLAP http://tclap.sourceforge.net/
PNG File Output          PNG++ http://www.nongnu.org/pngpp/
======================== ========================================

.. note::
  Windows Platform is NOT supported currently.

How to build
------------

Minecraft Unicode Font Generator is primarily written in C++. It is written based on C++11/C++14 standard features. The current source code requires C++14 aware compiler. For well-known compilers, such as g++ and clang, the ``-std=c++11`` or ``-std=c++0x`` flag must be supported.

In order to build Minecraft Unicode Font Generator from the source package, you need following development packages (package name may vary depending on the distribution you use):

* FreeType2 https://www.freetype.org/
* TCLAP http://tclap.sourceforge.net/
* PNG++ http://www.nongnu.org/pngpp/

If you downloaded source code from git repository, you have to install following packages to get build environment:

* premake5
* gnumake
* gcc-c++/clang-c++

And run following command to generate Makefile::

    $ premake5 gmake

You can choose clang++ as compiler by following command instead::

    $ premake5 -cc=clang gmake

After Makefile is generated, run ``make`` to compile the program::

    $ make

