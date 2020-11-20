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

layout(set = 0, binding = 1) uniform LightUBO
{
	vec4 light_pos;
	vec4 light_color;
};

layout (location = 1) out vec3 out_light_dir;
layout (location = 2) out vec3 out_view_dir;
layout (location = 3) out vec2 out_uv;

void main()
{
    mat4 model_view_mat = view * Model.model_mat;
    vec4 P = model_view_mat * vec4(position, 1.0f);
    
	vec3 N = normalize(mat3(model_view_mat) * normal);
	vec3 T = normalize(mat3(model_view_mat) * tangent);
	vec3 B = cross(N, T);

	vec3 L = (view * light_pos).xyz - P.xyz;
	out_light_dir = normalize(vec3(dot(L, T), dot(L, B), dot(L, N)));

	vec3 V = -P.xyz;
	out_view_dir = normalize(vec3(dot(V, T), dot(V, B), dot(V, N)));

	out_uv = uv;

	gl_Position = projection * P;
}
