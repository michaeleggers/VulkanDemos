#version 450
#extension GL_ARB_separate_shader_objects : enable


layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 1) uniform UBO
{
	vec4 light_pos;
	vec4 light_color;
};

layout (location = 1) in vec4 viewspace_pos;
layout (location = 2) in vec4 viewspace_normal;
layout (location = 3) in vec4 color;
layout (location = 4) in vec4 position_ndc;
layout (location = 5) in vec2 uv;
layout (location = 6) in mat4 view_matrix;

layout (push_constant) uniform Material_t
{
    layout (offset = 64)  vec4 emissive;
	layout (offset = 80)  vec4 ambient;
	layout (offset = 96)  vec4 diffuse;
	layout (offset = 112) vec4 specular;
} Material;

void main() 
{
	vec3 normal_n = normalize(viewspace_normal.xyz);
	vec3 view_dir_n = normalize(-viewspace_pos.xyz);

	vec3 ambient_part = vec3(light_color * Material.ambient);

	float diffuse_strength = clamp(dot(normal_n, view_dir_n), 0.f, 1.f);
	vec3 diffuse_part = diffuse_strength * Material.diffuse.xyz;

	vec3 light_dir_n = normalize(light_pos.xyz - viewspace_pos.xyz);
	vec3 reflect_light_n = normalize(reflect(-light_dir_n, normal_n));
	float specular_strength = clamp(dot(reflect_light_n, view_dir_n), 0, 1);
	specular_strength = pow(specular_strength, Material.specular.w);
	vec3 specular_part = specular_strength * Material.specular.xyz;

	outColor = vec4(diffuse_part + specular_part + 0.1f*ambient_part, 1.f);
}
