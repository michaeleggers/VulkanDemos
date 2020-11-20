@echo off

%VULKAN_SDK%/bin/glslc postprocess.vert -o postprocess_vert.spv
%VULKAN_SDK%/bin/glslc postprocess.frag -o postprocess_frag.spv

%VULKAN_SDK%/bin/glslc rasterizer.vert -o rasterizer_vert.spv
%VULKAN_SDK%/bin/glslc rasterizer.frag -o rasterizer_frag.spv

%VULKAN_SDK%/bin/glslc raygen.rgen      -o raygen.rgen.spv
%VULKAN_SDK%/bin/glslc miss.rmiss       -o miss.rmiss.spv
%VULKAN_SDK%/bin/glslc closesthit.rchit -o closesthit.rchit.spv
%VULKAN_SDK%/bin/glslc shadowmiss.rmiss -o shadowmiss.rmiss.spv

%VULKAN_SDK%/bin/glslc closesthit_debug.rchit -o closesthit_debug.rchit.spv