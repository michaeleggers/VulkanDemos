#version 450
#extension GL_ARB_separate_shader_objects : enable

vec2 positions[3] = vec2[](
    vec2(0.0, -0.5),
    vec2(0.5, 0.5),
    vec2(-0.5, 0.5)
);

vec3 colors[3] = vec3[](
    vec3(1.0, 0.0, 0.0),
    vec3(0.0, 1.0, 0.0),
    vec3(0.0, 0.0, 1.0)
);

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
	mat4 ass;
};

layout (location = 1) out vec4 viewspace_pos;
layout (location = 2) out vec4 viewspace_normal;
layout (location = 3) out vec4 position_ndc;
layout (location = 4) out vec2 uv_out;
layout (location = 5) out mat4 view_out;
layout (location = 9) out mat4 model_out;

void main()
{
    
    gl_Position = projection * view * Model.model_mat * vec4(position, 1.0);
    
    viewspace_pos    = view * Model.model_mat * vec4(position, 1.f);
    viewspace_normal = view * Model.model_mat * vec4(normal, 0.f);
    uv_out           = uv;
	view_out         = view;
	model_out        = Model.model_mat;
    position_ndc     = gl_Position;
}