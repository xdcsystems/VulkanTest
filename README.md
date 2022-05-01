Test Vulkan demo
=========================

This is a traditional Hello World style application for the Vulkan API.
It renders a rotate RGB shaded equilateral triangle.

The main difference from the examples available on the net is the creation of a separate rendering thread.
This allows you to draw a window with a rotating object continuously. Whether the size or position of the window changes.

It is derived from the excellent projects of Sascha Willems, Petr Kraus,
Alexander Overvoorde and Khronos for Vulkan-Hpp project.

[https://github.com/SaschaWillems/Vulkan]

[https://github.com/krOoze/Hello_Triangle.git]

[https://github.com/Overv/VulkanTutorial]

[https://github.com/KhronosGroup/Vulkan-Hpp]


Requirements
----------------------------

**OS**: Windows or Linux  
**Language**: C++20  
**Build environment**: (latest) [Vulkan SDK](https://vulkan.lunarg.com/sdk/home) (requires `VULKAN_SDK` variable being set)  
**Build environment[Windows]**: Visual Studio, Cygwin, or MinGW (or IDEs running on top of them)  
**Build environment[Linux]**: CMake compatible compiler and build system and `libxcb-dev` and `libxcb-keysyms-dev`  
**Build environment[MacOS]**: CMake compatible compiler and build system  
**Build Environment[GLFW]**: GLFW 3.2+ (already included as a git submodule), requires `xorg-dev` (or XCB) package on Linux  
**Build Environment[GLM]**: GLM 0.9.9.9+ (already included as a git submodule),  
**Target Environment**: installed (latest) Vulkan capable drivers (to see anything)  
**Target Environment**: GLFW

On Unix-like environment refer to
[SDK docs](https://vulkan.lunarg.com/doc/sdk/latest/linux/getting_started.html)
on how to set `VULKAN_SDK` variable.

Adding `VkSurface` support for other platforms should be straightforward using
the provided ones as a template for it.

Files
----------------------------------

| file | description |
|---|---|
| external/glfw/ | GLFW git submodule |
| external/glm/ | GLM git submodule |
| src/main.cpp | The app entry point, including the `main()` function |
| src/Application.cpp | The app class souce code |
| src/CompilerMessages.h | Allows to make compile-time messages shown in the compiler output |
| src/EnumerateScheme.h | A scheme to unify usage of most Vulkan `vkEnumerate*` and `vkGet*` commands |
| src/ErrorHandling.h | `VkResult` check helpers + `VK_EXT_debug_utils` extension related stuff |
| src/VulkanIntrospection.h | Introspection of Vulkan entities; e.g. convert Vulkan enumerants to strings |
| data/shaders | The vertex shader folder |
| .gitignore | Git filter file ignoring most probable outputs messing up the local repo |
| .gitmodules | Git submodules file describing the dependency on GLFW and GLM |
| CMakeLists.txt | CMake makefile |
| LICENSE | Copyright licence for this project |
| README.md | This file |

Build
----------------------------------------------
First just get everything:

    $ git clone --recurse-submodules https://github.com/xdcsystems/VulkanTest.git

Then, on Windows platphorm:  
    1. Install Vulkan SDK  
    2. run build-all.bat , Visual Studio 2019 solution files will be generate  

In many platforms CMake style build should work just fine:

    $ cmake -G"Your preferred generator"

or even just

    $ cmake .

Then use `make`, or the generated Visual Studio `*.sln`, or whatever it created.

You also might want to add `-DCMAKE_BUILD_TYPE=Debug`.

Run
------------------------

You just run it as you would anything else.

<kbd>Esc</kbd> does terminate the app.  
<kbd>Alt</kbd> + <kbd>Enter</kbd> toggles fullscreen (might not work on some WSIplatforms).  
<kbd>q</kbd> increasing rotate speed to left side.  
<kbd>e</kbd> increasing rotate speed to right side.  
