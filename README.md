# 🌋 vulkan-base 🖖

* "Simple" Vulkan application that renders textured 3D model.
* Highlights: setup of rasterization pipeline, timestamp queries, debug labels, imgui integration.
* For basic Vulkan raytracing check this repository: https://github.com/kennyalive/vulkan-raytracing

Prerequisites:
* Microsoft Visual Studio  
<sub> VS 2017 solution is provided. Fow never versions of VS you might need to confirm toolset upgrade request when opening the solution for the first time. The project was tested in VS 2017/VS 2019/VS 2022.</sub>
* Vulkan SDK  
<sub>Should be installed locally (installation also sets VULKAN_SDK environment variable which is accessed by custom build step). We use shader compiler from Vulkan SDK distribution to compile the shaders.</sub>

Build steps: 

1. Open solution in Visual Studio IDE.
2. Press F7 (Build Solution). That's all. All dependencies are included.

In order to enable Vulkan validation layers specify ```--validation-layers``` command line argument.

![vulkan-base](https://user-images.githubusercontent.com/4964024/64047691-c812e280-cb6f-11e9-8f26-76c4ee8860cd.png)

_"About 64KB of source code is a reasonable upper limit for the complexity of single triangle rasterization problems now envisioned."_
