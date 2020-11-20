#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) in vec3 position;
layout (location = 1) in vec2 uv;
layout (location = 2) in vec3 normal;
layout (location = 3) in vec4 color;
layout (location = 4) in vec3 tangent;

layout (location = 0) out vec2 out_uv;

layout (push_constant) uniform Model_t
{
    mat4 model_mat;
} Model;


layout (set = 0, binding = 0) uniform UBO
{
    mat4 view;
    mat4 projection;
};
layout (set = 0, binding = 2) uniform ModelUniform_t { float scale; } ModelUniform;

void main()
{
	float scale = ModelUniform.scale;
	mat4 scale_mat = mat4(scale, 0.f, 0.f, 0.f,
						  0.f, scale, 0.f, 0.f,
						  0.f, 0.f, scale, 0.f,
						  0.f, 0.f, 0.f, 1.f);
    mat4 model_view_mat = view * scale_mat * Model.model_mat;
    vec4 P = model_view_mat * vec4(position, 1.0f);
	out_uv = uv;

	gl_Position = projection * P;
}
