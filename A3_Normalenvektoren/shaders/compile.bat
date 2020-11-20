@echo off

%VULKAN_SDK%/bin/glslc shader.vert -o vert.spv
%VULKAN_SDK%/bin/glslc shader.frag -o frag.spv

%VULKAN_SDK%/bin/glslc gouraud_shader.frag -o gouraud_frag.spv
%VULKAN_SDK%/bin/glslc gouraud_shader.vert -o gouraud_vert.spv