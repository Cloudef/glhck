# GLhck

Nothing interesting here yet...
Move on.

## Screenshots

![Screenshot](http://cloudef.eu/armpit/glhck-new-utf8-text.png)
![Screenshot2](http://cloudef.eu/armpit/glhck-openctm.png)
![Screenshot3](http://cloudef.eu/armpit/glhck-assimp.png)

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
*  [glhck-scene][] - QMLON GLhck scene arranger
*  [2dshooter][] - 2D Shooter game
*  [spacerocks][] - 2D Asteroids clone
*  [anglhck][] - AngelScript bindings for glhck

## Thanks to
*  [GLEW][] - OpenGL Extension Wrangler
*  [GLFW][] - OpenGL Window/Context manager
*  [Font-Stash][] - Dynamic font glyph cache
*  [ACTC][] - Triangle consolidator
*  [Kazmath][] - C Math library
*  [STB-stuff][] - Used as freetype replacement

[glhck-scene]: https://github.com/bzar/glhck-scene
[2dshooter]: https://github.com/bzar/2dshooter
[spacerocks]: https://github.com/bzar/spacerocks
[anglhck]: https://github.com/bzar/anglhck

[GLEW]: http://glew.sourceforge.net/
[GLFW]: http://www.glfw.org/
[ACTC]: http://www.plunk.org/~grantham/public/actc/
[Kazmath]: https://github.com/Kazade/kazmath
[Font-Stash]: http://digestingduck.blogspot.com/2009/08/font-stash.html
[STB-stuff]: http://nothings.org
