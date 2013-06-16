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

## Addons for GLhck
* [nethck][] - Geometry sharing over network
* [anglhck][] - AngelScript bindings
* [consolehck][] - Text console widget

## Projects using GLhck
* [glhck-scene][] - QMLON GLhck scene arranger
* [2dshooter][] - 2D Shooter game
* [spacerocks][] - 2D Asteroids clone

## Thanks to
* [GLEW][] - OpenGL Extension Wrangler
* [GLFW][] - OpenGL Window/Context manager
* [GLSW][] - The OpenGL Shader Wrangler
* [Font-Stash][] - Dynamic font glyph cache
* [ACTC][] - Triangle consolidator
* [Kazmath][] - C Math library
* [STB-stuff][] - Used as freetype replacement
* [UTF-8 Decoder][] - Flexible and Economical UTF-8 Decoder
* [Assimp][] - Open Asset Import Library
* [OpenCTM][] - Open Compressed Triangle Mesh file format
* [Imlib2][] - GLhck's image format loaders are based on imlib2

[nethck]: https://github.com/Cloudef/nethck
[anglhck]: https://github.com/bzar/anglhck
[consolehck]: https://github.com/bzar/consolehck

[glhck-scene]: https://github.com/bzar/glhck-scene
[2dshooter]: https://github.com/bzar/2dshooter
[spacerocks]: https://github.com/bzar/spacerocks

[GLEW]: http://glew.sourceforge.net/
[GLFW]: http://www.glfw.org/
[GLSW]: http://prideout.net/blog/?p=11
[ACTC]: http://www.plunk.org/~grantham/public/actc/
[Kazmath]: https://github.com/Kazade/kazmath
[Font-Stash]: http://digestingduck.blogspot.com/2009/08/font-stash.html
[STB-stuff]: http://nothings.org
[UTF-8 Decoder]: http://bjoern.hoehrmann.de/utf-8/decoder/dfa/
[Assimp]: http://assimp.sourceforge.net/
[OpenCTM]: http://openctm.sourceforge.net/
[Imlib2]: http://docs.enlightenment.org/api/imlib2/html/
