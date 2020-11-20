@echo off

%VULKAN_SDK%/bin/glslc shader.vert -o vert.spv
%VULKAN_SDK%/bin/glslc shader.frag -o frag.spv

REM skybox
%VULKAN_SDK%/bin/glslc vert_v2.vert -o vert_v2.spv
%VULKAN_SDK%/bin/glslc frag_v2.frag -o frag_v2.spv
