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

	vec3 model_pos;
};

layout (location = 1) out vec4 out_world_pos;
layout (location = 2) out vec4 out_light_pos;
layout (location = 3) out vec4 out_viewspace_normal;
layout (location = 4) out vec2 out_uv;
layout (location = 5) out vec4 out_color;

const mat4 bias_mat = mat4( 
	0.5, 0.0, 0.0, 0.0,
	0.0, 0.5, 0.0, 0.0,
	0.0, 0.0, 1.0, 0.0,
	0.5, 0.5, 0.0, 1.0 );

void main()
{
	vec4 world_pos = Model.model_mat * vec4(position, 1.0);
	out_world_pos = world_pos;

	out_viewspace_normal = view * Model.model_mat * vec4(normal, 0.0);
	out_uv = uv;
	out_color = color;
	
	/* The Vertex as seen from the light's point of view in biased clipspace */
	out_light_pos = bias_mat * light_proj_mat * light_view_mat * world_pos;

    gl_Position = projection * view * world_pos;
    
    

}
