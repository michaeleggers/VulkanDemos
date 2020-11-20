#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) out vec4 out_color;
layout (location = 0) in vec3 in_position;

layout(set = 2, binding = 0) uniform samplerCube cubemap_sampler;

void main()
{
    vec4 cubemap_color = texture(cubemap_sampler, in_position);
    out_color = vec4(cubemap_color.xyz, 1.f);
}