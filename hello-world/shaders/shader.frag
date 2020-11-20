#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_nonuniform_qualifier    : enable


layout(location = 0) out vec4 outColor;

layout (location = 0) in vec2 in_uv;

layout (set = 0, binding = 1) uniform sampler2D textures[];

layout (push_constant) uniform Material_t
{
    layout (offset = 64)  vec4 emissive;
	layout (offset = 80)  vec4 ambient;
	layout (offset = 96)  vec4 diffuse;
	layout (offset = 112) vec4 specular;
	layout (offset = 128) uint texture_id;
	layout (offset = 132) uint is_textured;
} Material;


void main() 
{
	
	vec3 diffuse = texture(textures[Material.texture_id], in_uv).rgb;

	outColor = vec4(diffuse, 1.0);
}
