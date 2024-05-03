cmake_minimum_required(VERSION 3.15)
project(vulkan-base)

set(PROGRAM_SOURCE
    src/demo.cpp
    src/demo.h
    src/lib.cpp
    src/lib.h
    src/main.cpp
    src/vk.cpp
    src/vk.h
)
set(SHADER_SOURCE
    src/shaders/mesh.vert.glsl
    src/shaders/mesh.frag.glsl
)

set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
source_group("" FILES ${PROGRAM_SOURCE})
source_group(TREE "${CMAKE_SOURCE_DIR}/src/shaders" PREFIX shaders FILES ${SHADER_SOURCE})

if (MSVC)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
    add_compile_options(/MP /W3)
    # Match MSVC Release
    add_compile_options($<$<CONFIG:Release>:/Zi>)
    add_compile_options($<$<CONFIG:Release>:/GL>)
    add_compile_options($<$<CONFIG:Release>:/Gy>)
    add_link_options($<$<CONFIG:Release>:/DEBUG>)
    add_link_options($<$<CONFIG:Release>:/OPT:REF>)
    add_link_options($<$<CONFIG:Release>:/OPT:ICF>)
    add_link_options($<$<CONFIG:Release>:/LTCG>)
else()
    add_compile_options(-Wall -Wextra)
endif()

add_executable(vulkan-base ${PROGRAM_SOURCE} ${SHADER_SOURCE})
target_compile_features(vulkan-base PRIVATE cxx_std_20)
add_subdirectory(third-party)
target_link_libraries(vulkan-base third-party)

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(vulkan-base PRIVATE
        -Wno-unused-parameter
        -Wno-missing-field-initializers
    )
endif()

set_target_properties(vulkan-base PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    VS_DPI_AWARE "PerMonitor"
)
set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT vulkan-base)

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
