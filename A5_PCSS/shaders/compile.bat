@echo off

%VULKAN_SDK%/bin/glslc shader.vert -o vert.spv
%VULKAN_SDK%/bin/glslc shader.frag -o frag.spv

%VULKAN_SDK%/bin/glslc shadowmap.vert -o shadowmap_vert.spv
%VULKAN_SDK%/bin/glslc shadowmap.frag -o shadowmap_frag.spv

%VULKAN_SDK%/bin/glslc lights.vert -o lights_vert.spv
%VULKAN_SDK%/bin/glslc lights.frag -o lights_frag.spv