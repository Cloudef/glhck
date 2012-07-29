# GLhck

Nothing interesting here yet...
Move on.

## Screenshot

![Screenshot](http://cloudef.eu/armpit/glhck-new-utf8-text.png)

## Building

    git submodule init                    # - initialize and fetch
    git submodule update                  #   submodules
    mkdir target && cd target             # - create build target directory
    cmake -DCMAKE_INSTALL_PREFIX=build .. # - run CMake, set install directory
    make                                  # - compile

## Running test(s)

    cd target                             # - cd to your target directory
    ./test/display                        # - for example

## Installing

    cd target                             # - cd to your target directory
    make install                          # - install


## Thanks to
*  [GLEW][] - OpenGL Extension Wrangler
*  [GLFW][] - OpenGL Window/Context manager
*  [Font-Stash][] - Dynamic font glyph cache
*  [ACTC][] - Triangle consolidator
*  [Kazmath][] - C Math library
*  [STB-stuff][] - Used as freetype replacement

[GLEW]: http://glew.sourceforge.net/
[GLFW]: http://www.glfw.org/
[ACTC]: http://www.plunk.org/~grantham/public/actc/
[Kazmath]: https://github.com/Kazade/kazmath
[Font-Stash]: http://digestingduck.blogspot.com/2009/08/font-stash.html
[STB-stuff]: http://nothings.org
