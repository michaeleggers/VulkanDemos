#version 450
#extension GL_ARB_separate_shader_objects : enable


layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform UBO
{
    mat4 view;
    mat4 projection;
	mat4 light_view_mat;
	mat4 light_proj_mat;

	vec4 light_pos;
	vec2 light_size;
	float light_near;
	float light_far;
	uint filter_method;
	uint poisson_disk_size;
	uint  use_textures;

} ubo;

void main() 
{
	outColor = vec4(1.f);
}
