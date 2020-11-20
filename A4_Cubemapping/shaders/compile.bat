@echo off

%VULKAN_SDK%/bin/glslc scene.vert -o scene_vert.spv
%VULKAN_SDK%/bin/glslc scene.frag -o scene_frag.spv

REM skybox
%VULKAN_SDK%/bin/glslc skybox.vert -o skybox_vert.spv
%VULKAN_SDK%/bin/glslc skybox.frag -o skybox_frag.spv
