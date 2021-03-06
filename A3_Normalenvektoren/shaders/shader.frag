#version 450
#extension GL_ARB_separate_shader_objects : enable


layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 1) uniform UBO
{
	vec4 light_pos;
	vec4 light_color;
    uint show_normals;
};

layout (set = 0, binding = 2) uniform MaterialUBO
{
	vec4 emissive;
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
} Material;

layout (location = 1) in vec4 viewspace_pos;
layout (location = 2) in vec4 viewspace_normal;
layout (location = 4) in vec4 color;
layout (location = 5) in vec4 position_ndc;
layout (location = 6) in vec2 uv;
layout (location = 7) in mat4 view_matrix;

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

	outColor = vec4(Material.emissive.xyz + ambient_part + diffuse_part + specular_part, 1.f);
}
