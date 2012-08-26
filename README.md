# GLhck

Nothing interesting here yet...
Move on.

## Screenshots

![Screenshot](http://cloudef.eu/armpit/glhck-new-utf8-text.png)
![Screenshot2](http://cloudef.eu/armpit/glhck-openctm.png)

## Building

    git submodule update --init --recursive  # - initialize and fetch submodules
    mkdir target && cd target                # - create build target directory
    cmake -DCMAKE_INSTALL_PREFIX=build ..    # - run CMake, set install directory
    make                                     # - compile

## Running example(s)

    cd target                                # - cd to your target directory
    ./example/display                        # - for example

## Installing

    cd target                                # - cd to your target directory
    make install                             # - install

## Projects using GLhck
*  [glhck-scene][] - QMLON GLhck scene viewer
*  [spacerocks][] - 2D Asteroids clone

## Thanks to
*  [GLEW][] - OpenGL Extension Wrangler
*  [GLFW][] - OpenGL Window/Context manager
*  [Font-Stash][] - Dynamic font glyph cache
*  [ACTC][] - Triangle consolidator
*  [Kazmath][] - C Math library
*  [STB-stuff][] - Used as freetype replacement

[glhck-scene]: https://github.com/bzar/glhck-scene
[spacerocks]: https://github.com/bzar/spacerocks

[GLEW]: http://glew.sourceforge.net/
[GLFW]: http://www.glfw.org/
[ACTC]: http://www.plunk.org/~grantham/public/actc/
[Kazmath]: https://github.com/Kazade/kazmath
[Font-Stash]: http://digestingduck.blogspot.com/2009/08/font-stash.html
[STB-stuff]: http://nothings.org
