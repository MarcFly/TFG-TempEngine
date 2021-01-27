**SqueLib**
===

Thanks for taking a look at ***SqueLib***, an end of degree project built to help building **C++** multiplatform applications on Windows, Linux and Android easily and you having to write _only_  **C++** and a touch of **CMake** (and whatever else you decide to add)

For that reason it will be fully available for anyone to use and fork, although I will be the sole maintainer of this project as it will be my own testing grounds for anything I want to learn along the dying language of **C++** (maybe someday this will be ported or remade **Rust**).

But why did I really decided on this project, I hate going into IDE's. There are too many steps in configuring the projects and doing things. I prefer to get all things together, build and then just use any available IDE to debug what's happening, which **CMake** is great at. 

The project has been debugged using MSVS and GDB. I haven't got _ndk-gdb_ to work, probably because my toolchain is not in concordance to the ndk's, and I genuinely dislike Android Studio but that may have to be an option if someone can point me in the right direction.

What does SqueLib provide?
===

**Easy C++ Android:**</br>
As long as you are using C++17 that matches LLVM's LibC support, you can build with it and get an android app easily.

**Easy Logging:**</br>
Maybe not that fast and memory efficient, but very practical log management that retrieves where it is called (very practical if you are a mad log-debugger like me).

**Display Management:**</br>
Althought currently not supporting it on _Android_, _SqueLib_ wraps over _GLFW_ to provide an easier experience that will not require a whole rewrite on _Android_.

**OpenGL'esque rendering:**</br>
Crossplatform wrapping of _OpenGL_ (up to latest Core without extensions) and _OpenGLES_ 3.2, some simple types are provided that makes it easier to get things going. I focus more on first declaring and then performing rather than having to setup each time verbosely like plain _OpenGL_

A long term interest is having support for other APIs to learn how they work and how to have a more comforming code, but that will have to wait until this library proves useful by itself.

**Easy file management:**</br>
An easy way to load and write files and assets for the provided platforms.

**Fast Precision Timer:**</br>
Using _chrono_, a simple and efficient timer is provided down to nanosecond precision.

Building With SqueLib
===

You can basically copy the style of any example found, but the main parts will be the following:

 - Add FlyLib as a subdirectory, specifying any Binary Directory:
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ bash
    add_subdirectory(${path_to_FlyLib} ${paht_to_BinaryDir})
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 - Setup the install targets process
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ bash
    # this will be update with the better functionality
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 - Build with cmake adding the target platform
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ bash
    cmake <dir> ( -DToWindows=1 || -DToLinux=1 || -DToAndroid ) -DCMAKE_BUILD_TYPE=(Release||Debug)
    cmake --build <dir>
    # Optional for Android, install into android device...
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
For getting into building with CMake, I strongly recommend **[Jeff Preshing's](https://preshing.com/)** **[introduction](https://preshing.com/20170522/learn-cmakes-scripting-language-in-15-minutes/)**.</br>
There is a lot of info available and the documentation is extensive, but this carreid me heavily starting out. The post on plywood and arch80 engine have been also really interesting!

# Resources
When packaging apps and building, you'd want to have your data properly packaged together. Until ***SqueLib*** supports proper packaging (through cmake), currently the only option is to package by yourself where it is built for _Linux/Windows_.

On _Android_ it is another pain in the ass, currently ***SqueLib*** will copy a folder called _EngineAssets_ and _EngineResources_ to where it's built, then it will package and install accordingly. It is imperative that the EngineResources has a folder called icon with icon.png or it will not have any icon and fail the packaging process as the manifest file specifies. This will be updated following more complete releases as project advances.

The _EngineAssets_ folder is required to package properly the app, which will be where things that do not require permission to interact with will be saved. On the long run you don't want a big package file on a phone, rather a lean app that will download and store properly the required files elsewhere, as some data will need to be modified. Permissions are currently WIP and external storage support will also be coming.

# License
Copyright 2021 Marc Torres Jimenez

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
# Acknowledgements
 - **[Raysan5](https://github.com/raysan5/)** for a short but deep introductory talk on libraries and making code more reusable, and obviously for **[_RayLib_](https://www.raylib.com/)** which is wil provide more than what I can and be probably more robust.
 - **[CNLohr](https://github.com/cnlohr)** for **[_RawDraw_](https://github.com/cntools/rawdraw/tree/27f05afe1747e3dc7a5dd02eaf2b761ef3624762)** and specifically **[_RawDrawAndroid_](https://github.com/cnlohr/rawdrawandroid)**, getting this to work has been much easier and a great learning process for CMake (as I have learn CMake translating your makefiles to CMake).
 - **[Morgan McGuire](https://casual-effects.com/)** for providing **[_Markdeep_](https://casual-effects.com/markdeep)** in an open source way, writing this readme has been a pleasure.

 The full project documentation and bibliography will be available as the project documentation nears its completion.
