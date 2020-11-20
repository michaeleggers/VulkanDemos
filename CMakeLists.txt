
cmake_minimum_required(VERSION 3.10)

project(VulkanThesis VERSION 1.0)

cmake_minimum_required(VERSION 3.12)

find_package(Vulkan REQUIRED)

add_executable(A1_Versuch1c
    A1_Versuch1c/Versuch1c_v2.cpp
    dependencies/imgui/imgui.cpp
    dependencies/imgui/imgui_draw.cpp
    dependencies/imgui/imgui_impl_glfw.cpp
    dependencies/imgui/imgui_impl_vulkan.cpp
    dependencies/imgui/imgui_widgets.cpp
)
target_include_directories(A1_Versuch1c
    PUBLIC dependencies/stb  dependencies
)

add_executable(A3_Normalenvektoren
    A3_Normalenvektoren/A3_Normalenvektoren.cpp
    dependencies/imgui/imgui.cpp
    dependencies/imgui/imgui_draw.cpp
    dependencies/imgui/imgui_impl_glfw.cpp
    dependencies/imgui/imgui_impl_vulkan.cpp
    dependencies/imgui/imgui_widgets.cpp
    A3_Normalenvektoren/shaders/shader.vert
    A3_Normalenvektoren/shaders/shader.frag
    A3_Normalenvektoren/shaders/gouraud_shader.vert
    A3_Normalenvektoren/shaders/gouraud_shader.frag
)
target_include_directories(A3_Normalenvektoren
    PUBLIC dependencies/stb  dependencies VulkanFramework
)

add_executable(A4_Cubemapping
    A4_Cubemapping/main.cpp
    dependencies/imgui/imgui.cpp
    dependencies/imgui/imgui_draw.cpp
    dependencies/imgui/imgui_impl_glfw.cpp
    dependencies/imgui/imgui_impl_vulkan.cpp
    dependencies/imgui/imgui_widgets.cpp
    A4_Cubemapping/shaders/skybox.frag
    A4_Cubemapping/shaders/skybox.vert
    A4_Cubemapping/shaders/scene.vert
    A4_Cubemapping/shaders/scene.frag
)
target_include_directories(A4_Cubemapping
    PUBLIC dependencies/stb  dependencies VulkanFramework
)

add_executable(A5_PCSS
    A5_PCSS/main.cpp
    dependencies/imgui/imgui.cpp
    dependencies/imgui/imgui_draw.cpp
    dependencies/imgui/imgui_impl_glfw.cpp
    dependencies/imgui/imgui_impl_vulkan.cpp
    dependencies/imgui/imgui_widgets.cpp
    A5_PCSS/shaders/shadowmap.vert
    A5_PCSS/shaders/shadowmap.frag
    A5_PCSS/shaders/shader.vert
    A5_PCSS/shaders/shader.frag
    A5_PCSS/shaders/lights.vert
    A5_PCSS/shaders/lights.frag
)
target_include_directories(A5_PCSS
    PUBLIC dependencies/stb dependencies VulkanFramework
)

# Nvidia RTX Raytracing
add_executable(rtx-raytracing
    rtx-raytracing/main.cpp
    dependencies/imgui/imgui.cpp
    dependencies/imgui/imgui_draw.cpp
    dependencies/imgui/imgui_impl_glfw.cpp
    dependencies/imgui/imgui_impl_vulkan.cpp
    dependencies/imgui/imgui_widgets.cpp
    rtx-raytracing/shaders/postprocess.vert
    rtx-raytracing/shaders/postprocess.frag
    rtx-raytracing/shaders/raygen.rgen
    rtx-raytracing/shaders/miss.rmiss
    rtx-raytracing/shaders/closesthit.rchit
    rtx-raytracing/shaders/shadowmiss.rmiss
    rtx-raytracing/shaders/rasterizer.vert
    rtx-raytracing/shaders/rasterizer.frag
)
target_include_directories(rtx-raytracing
    PUBLIC dependencies/stb dependencies VulkanFramework
)


# DEMOS

# Z-Fighting
add_executable(z-fighting-demo
    z-fighting-demo/main.cpp
    dependencies/imgui/imgui.cpp
    dependencies/imgui/imgui_draw.cpp
    dependencies/imgui/imgui_impl_glfw.cpp
    dependencies/imgui/imgui_impl_vulkan.cpp
    dependencies/imgui/imgui_widgets.cpp
    z-fighting-demo/shaders/shader.vert
    z-fighting-demo/shaders/shader.frag
)
target_include_directories(z-fighting-demo
    PUBLIC dependencies/stb  dependencies VulkanFramework
)

# Bump-Mapping
add_executable(bump-mapping-demo
    bump-mapping-demo/main.cpp
    dependencies/imgui/imgui.cpp
    dependencies/imgui/imgui_draw.cpp
    dependencies/imgui/imgui_impl_glfw.cpp
    dependencies/imgui/imgui_impl_vulkan.cpp
    dependencies/imgui/imgui_widgets.cpp
    bump-mapping-demo/shaders/shader.vert
    bump-mapping-demo/shaders/shader.frag
)
target_include_directories(bump-mapping-demo
    PUBLIC dependencies/stb  dependencies VulkanFramework
)

# Hello World
add_executable(hello-world
    hello-world/main.cpp
    dependencies/imgui/imgui.cpp
    dependencies/imgui/imgui_draw.cpp
    dependencies/imgui/imgui_impl_glfw.cpp
    dependencies/imgui/imgui_impl_vulkan.cpp
    dependencies/imgui/imgui_widgets.cpp
    hello-world/shaders/shader.vert
    hello-world/shaders/shader.frag
)
target_include_directories(hello-world
    PUBLIC dependencies/stb  dependencies VulkanFramework
)



# GLFW, https://www.glfw.org/docs/latest/build.html#build_link_cmake_source
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(dependencies/glfw332src)

# ASSIMP
add_subdirectory(dependencies/assimpsrc)

# MODELLOADER
add_library(libmodelloader
    dependencies/modelloader/Model.cpp
)
target_include_directories(libmodelloader
    PUBLIC dependencies/modelloader
)

# IMAGELOADER
add_library(libimageloader
    dependencies/imageloader/ImageLoader.cpp
)
target_include_directories(libimageloader
    PUBLIC dependencies/imageloader dependencies/stb
)

# ME PLATFORM LAYER
add_library(libmeplatform
    dependencies/me_platform/platform.cpp
)
target_include_directories(libmeplatform
    PUBLIC dependencies/me_platform
)

# Vulkan Abstraction Layer
add_library(libvkal
    VulkanFramework/vkal_imgui.cpp
    VulkanFramework/vkal_imgui.h
    VulkanFramework/camera.h
    VulkanFramework/camera.cpp
    VulkanFramework/model.h
    VulkanFramework/model.cpp
    VulkanFramework/vkal.cpp
    VulkanFramework/vkal.h
)
target_include_directories(libvkal
    PUBLIC dependencies Vulkan::Vulkan
)
target_link_libraries(libvkal PUBLIC
    Vulkan::Vulkan 
    glfw
    assimp
    libmeplatform
)

SET_PROPERTY(TARGET glfw  PROPERTY FOLDER "Libraries")
SET_PROPERTY(TARGET assimp PROPERTY FOLDER "Libraries")
SET_PROPERTY(TARGET libmodelloader PROPERTY FOLDER "Libraries")
SET_PROPERTY(TARGET libimageloader PROPERTY FOLDER "Libraries")
SET_PROPERTY(TARGET libvkal PROPERTY FOLDER "Libraries")
SET_PROPERTY(TARGET libmeplatform PROPERTY FOLDER "Libraries")

# Executables
set_property(TARGET A1_Versuch1c PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/A1_Versuch1c")
target_link_libraries(A1_Versuch1c
    PUBLIC libvkal
    PUBLIC glfw
)

set_property(TARGET A3_Normalenvektoren PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/A3_Normalenvektoren")
target_link_libraries(A3_Normalenvektoren
    PUBLIC libvkal
    PUBLIC glfw
    PUBLIC assimp
    PUBLIC libmeplatform
    PUBLIC libmodelloader
)


set_property(TARGET A4_Cubemapping PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/A4_Cubemapping")
target_link_libraries(A4_Cubemapping
    PUBLIC libvkal
    PUBLIC glfw
    PUBLIC assimp
    PUBLIC libmeplatform
    PUBLIC libimageloader
)

set_property(TARGET A5_PCSS PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/A5_PCSS")
target_link_libraries(A5_PCSS
    PUBLIC libvkal
    PUBLIC glfw
    PUBLIC assimp
    PUBLIC libmeplatform
)

set_property(TARGET z-fighting-demo PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/z-fighting-demo")
target_link_libraries(z-fighting-demo
    PUBLIC libvkal
    PUBLIC glfw
    PUBLIC assimp
    PUBLIC libmeplatform
    PUBLIC libmodelloader
)

set_property(TARGET bump-mapping-demo PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/bump-mapping-demo")
target_link_libraries(bump-mapping-demo
    PUBLIC libvkal
    PUBLIC glfw
    PUBLIC assimp
    PUBLIC libmeplatform
    PUBLIC libmodelloader
)

set_property(TARGET hello-world PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/hello-world")
target_link_libraries(hello-world
    PUBLIC libvkal
    PUBLIC glfw
    PUBLIC assimp
    PUBLIC libmeplatform
    PUBLIC libmodelloader
)

set_property(TARGET rtx-raytracing PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/rtx-raytracing")
target_link_libraries(rtx-raytracing
    PUBLIC libvkal
    PUBLIC glfw
    PUBLIC assimp
    PUBLIC libmeplatform
    PUBLIC libmodelloader
)

SET_PROPERTY(TARGET A1_Versuch1c PROPERTY FOLDER "Aufgaben")
SET_PROPERTY(TARGET A3_Normalenvektoren PROPERTY FOLDER "Aufgaben")
SET_PROPERTY(TARGET A4_Cubemapping  PROPERTY FOLDER "Aufgaben")
SET_PROPERTY(TARGET A5_PCSS PROPERTY FOLDER "Aufgaben")
SET_PROPERTY(TARGET z-fighting-demo PROPERTY FOLDER "Demos")
SET_PROPERTY(TARGET bump-mapping-demo PROPERTY FOLDER "Demos")
SET_PROPERTY(TARGET hello-world PROPERTY FOLDER "Demos")
SET_PROPERTY(TARGET rtx-raytracing PROPERTY FOLDER "Aufgaben")