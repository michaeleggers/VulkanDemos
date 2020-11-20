#version 460
#extension GL_NV_ray_tracing : require

layout(location = 3) rayPayloadInNV bool is_lit;

void main()
{
    is_lit = true;
}