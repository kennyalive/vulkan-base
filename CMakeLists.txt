cmake_minimum_required(VERSION 3.15)
project(vulkan-base)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(PROGRAM_SOURCE
    src/demo.cpp
    src/demo.h
    src/lib.cpp
    src/lib.h
    src/main.cpp
    src/vk.cpp
    src/vk.h
    src/vk_utils.cpp
    src/vk_utils.h
)
set(SHADER_SOURCE
    src/shaders/mesh.vert.glsl
    src/shaders/mesh.frag.glsl
)
set(THIRD_PARTY_SOURCE
    # glfw
    third-party/glfw/context.c
    third-party/glfw/egl_context.c
    third-party/glfw/glx_context.c
    third-party/glfw/init.c
    third-party/glfw/input.c
    third-party/glfw/linux_joystick.c
    third-party/glfw/monitor.c
    third-party/glfw/null_init.c
    third-party/glfw/null_joystick.c
    third-party/glfw/null_monitor.c
    third-party/glfw/null_window.c
    third-party/glfw/osmesa_context.c
    third-party/glfw/platform.c
    third-party/glfw/posix_module.c
    third-party/glfw/posix_poll.c
    third-party/glfw/posix_thread.c
    third-party/glfw/posix_time.c
    third-party/glfw/vulkan.c
    third-party/glfw/wgl_context.c
    third-party/glfw/win32_init.c
    third-party/glfw/win32_joystick.c
    third-party/glfw/win32_module.c
    third-party/glfw/win32_monitor.c
    third-party/glfw/win32_thread.c
    third-party/glfw/win32_time.c
    third-party/glfw/win32_window.c
    third-party/glfw/window.c
    third-party/glfw/x11_init.c
    third-party/glfw/x11_monitor.c
    third-party/glfw/x11_window.c
    third-party/glfw/xkb_unicode.c
    # imgui
    third-party/imgui/imconfig.h
    third-party/imgui/imgui.cpp
    third-party/imgui/imgui.h
    third-party/imgui/imgui_demo.cpp
    third-party/imgui/imgui_draw.cpp
    third-party/imgui/imgui_internal.h
    third-party/imgui/imgui_tables.cpp
    third-party/imgui/imgui_widgets.cpp
    third-party/imgui/impl/imgui_impl_glfw.cpp
    third-party/imgui/impl/imgui_impl_glfw.h
    third-party/imgui/impl/imgui_impl_vulkan.cpp
    third-party/imgui/impl/imgui_impl_vulkan.h
    # volk
    third-party/volk/volk.c
    third-party/volk/volk.h
)

source_group("" FILES ${PROGRAM_SOURCE})
source_group(TREE "${CMAKE_SOURCE_DIR}/third-party" PREFIX third-party FILES ${THIRD_PARTY_SOURCE})
source_group(TREE "${CMAKE_SOURCE_DIR}/src/shaders" PREFIX shaders FILES ${SHADER_SOURCE})

add_executable(vulkan-base ${PROGRAM_SOURCE} ${THIRD_PARTY_SOURCE} ${SHADER_SOURCE})
target_compile_features(vulkan-base PRIVATE cxx_std_17)

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    target_compile_definitions(vulkan-base PRIVATE _GLFW_WIN32 VK_USE_PLATFORM_WIN32_KHR)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_compile_definitions(vulkan-base PRIVATE _GLFW_X11 VK_USE_PLATFORM_XCB_KHR)
else()
    message(FATAL_ERROR "Unsupported platform: ${CMAKE_SYSTEM_NAME}")
endif()

target_include_directories(vulkan-base PRIVATE "${CMAKE_SOURCE_DIR}/third-party")

# needed for imgui vulkan backend, it includes "imgui.h".
target_include_directories(vulkan-base PRIVATE "${CMAKE_SOURCE_DIR}/third-party/imgui")

set_target_properties(vulkan-base PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    VS_DPI_AWARE "PerMonitor"
)

function(add_shader SHADER)
    get_filename_component(BASE_NAME ${SHADER} NAME_WLE)
    set(SPV_FILE "${CMAKE_SOURCE_DIR}/data/spirv/${BASE_NAME}.spv")
    set(SHADER_FILE "${CMAKE_SOURCE_DIR}/${SHADER}")
    add_custom_command(
        OUTPUT "${SPV_FILE}"
        COMMAND "$ENV{VULKAN_SDK}/bin/glslangValidator" "${SHADER_FILE}" -V --target-env vulkan1.2 -o "${SPV_FILE}"
        COMMAND "$ENV{VULKAN_SDK}/bin/spirv-opt" ${SPV_FILE} -O --strip-debug -o "${SPV_FILE}"
        MAIN_DEPENDENCY "${SHADER_FILE}"
    )
endfunction()

foreach(SHADER ${SHADER_SOURCE})
    add_shader(${SHADER})
endforeach()
