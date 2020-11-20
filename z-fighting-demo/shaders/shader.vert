#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) in vec3 position;
layout (location = 1) in vec2 uv;
layout (location = 2) in vec3 normal;
layout (location = 3) in vec4 color;
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

layout (location = 1) out vec4 viewspace_pos;
layout (location = 2) out vec4 viewspace_normal;
layout (location = 3) out vec4 color_out;
layout (location = 4) out vec4 position_ndc;
layout (location = 5) out vec2 uv_out;
layout (location = 6) out mat4 view_out;

void main()
{
    
    gl_Position = projection * view * Model.model_mat * vec4(position, 1.0);
    
    viewspace_pos    = view * Model.model_mat * vec4(position, 1.f);
    viewspace_normal = view * Model.model_mat * vec4(normal, 0.f);
    uv_out           = uv;
	view_out         = view;
	color_out        = color;
    position_ndc     = gl_Position / gl_Position.w;
}
