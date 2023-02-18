# 🌋 vulkan-base 🖖

* A simple Vulkan application that renders a textured 3D model.
* Highlights: setup of rasterization pipeline, timestamp queries, debug labels, imgui integration.
* For basic Vulkan ray tracing check this repository: https://github.com/kennyalive/vulkan-raytracing

Prerequisites:
* CMake
* Vulkan SDK  (VULKAN_SDK environment variable should be set)

Build steps: 

1. `cmake -S . -B build`
2. `cmake --build build`

Supported platforms: Windows, Linux.

In order to enable Vulkan validation layers specify ```--validation-layers``` command line argument.

![vulkan-base](https://user-images.githubusercontent.com/4964024/64047691-c812e280-cb6f-11e9-8f26-76c4ee8860cd.png)
