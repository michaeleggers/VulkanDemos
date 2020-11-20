#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_nonuniform_qualifier    : enable

layout (location = 1) in vec3 in_light_dir;
layout (location = 2) in vec3 in_view_dir;
layout (location = 3) in vec2 in_uv;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 1) uniform UBO
{
	vec4 light_pos;
	vec4 light_color;
};

layout (set = 0, binding = 2) uniform sampler2D textures[];

layout (push_constant) uniform Material_t
{
    layout (offset = 64)  vec4 emissive;
	layout (offset = 80)  vec4 ambient;
	layout (offset = 96)  vec4 diffuse;
	layout (offset = 112) vec4 specular;
} Material;

const float c_texture_repeat = 5.0f;

void main() 
{
	vec3 V = normalize(in_view_dir);
	vec3 L = normalize(in_light_dir);
	vec3 N = normalize(texture(textures[1], in_uv * c_texture_repeat).rgb * 2.0 - vec3(1.0));
	
	vec3 diffuse_albedo = texture(textures[0], in_uv * c_texture_repeat).rgb;
	vec3 diffuse = max(dot(N, L), 0.0) * diffuse_albedo;
	
	vec3 R = reflect(-L, N);
	vec3 specular_albedo = vec3(1.0);
	vec3 specular = vec3(0.0);
	if (dot(in_view_dir, in_light_dir) > 0.0) {
		specular = max(pow(dot(R, V), Material.specular.w), 0.0) * specular_albedo;
	}

	outColor = vec4(diffuse + specular, 1.0);
}
