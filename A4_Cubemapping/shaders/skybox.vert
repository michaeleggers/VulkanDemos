#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) in vec3 position;
layout (location = 1) in vec2 uv;
layout (location = 2) in vec3 normal;
layout (location = 3) in vec3 color;
layout (location = 4) in vec3 tangent;

layout (push_constant) uniform Model_t
{
    mat4 model_mat;
} Model;

layout (set = 0, binding = 0) uniform UBO
{
    mat4 view;
    mat4 projection;
};

layout (location = 0) out vec3 out_position;

void main()
{
    
    gl_Position = projection * view * Model.model_mat * vec4(position, 1.f);
    out_position = position.xyz;
}