# 🌋 vulkan-base 🖖

* "Simple" Vulkan application that renders textured 3D model.
* Highlights: setup of rasterization pipeline, timestamp queries, debug labels, imgui integration.
* For basic Vulkan raytracing check this repository: https://github.com/kennyalive/vulkan-raytracing

Prerequisites:
* CMake (tested only with Visual Studio generators, no Linux support at the moment)
* Vulkan SDK  
<sub>Should be installed locally (installation also sets VULKAN_SDK environment variable which is accessed by custom build step). We use shader compiler from Vulkan SDK distribution to compile the shaders.</sub>

Build steps: 

1. `cmake -S . -B build`
2. `cmake --build build` or open generated solution in Visual Studio and build there with F7

In order to enable Vulkan validation layers specify ```--validation-layers``` command line argument.

![vulkan-base](https://user-images.githubusercontent.com/4964024/64047691-c812e280-cb6f-11e9-8f26-76c4ee8860cd.png)

_"About 64KB of source code is a reasonable upper limit for the complexity of single triangle rasterization problems now envisioned."_
