#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) out vec4 outColor;
layout (location = 0) in vec2 in_uv;
layout (location = 1) in vec4 in_position;
layout (location = 2) in vec4 in_normal;

layout(binding = 0, set = 0) uniform CameraProperties 
{
    mat4 view;
	mat4 proj;
} cam;

layout (binding = 1, set = 0) uniform LightUBO
{
	vec3   pos;
	float  radius;
	uint   unused1;
	uint   unused2;
	uint   use_textures;
} light;

layout (binding = 2, set = 0) uniform sampler2D[] model_texture;

layout (push_constant) uniform Material
{
    layout (offset = 64)  vec4 emissive;
	layout (offset = 80)  vec4 ambient;
	layout (offset = 96)  vec4 diffuse;
	layout (offset = 112) vec4 specular;
    layout (offset = 128) uint texture_id;
	layout (offset = 132) uint is_textured;
	layout (offset = 136) uint normal_id;
	layout (offset = 140) uint has_normal_map;
} material_info;

void main() 
{
	vec4 diffuse = material_info.diffuse;
	if ( (material_info.is_textured == 1) && (light.use_textures == 1) ) {
		diffuse = texture(model_texture[material_info.texture_id], in_uv);
	}
    vec4 light_pos_view = cam.view * vec4(light.pos, 1.0f);
    vec3 light_dir = normalize(light_pos_view.xyz - in_position.xyz);
	vec3 normal = normalize(in_normal.xyz);
    float cos_light = max(dot(light_dir, normal), 0.0);
    outColor = vec4(vec3(cos_light), 1.0);
    outColor = vec4( cos_light * diffuse.xyz, 1.0f);
    //outColor = vec4( diffuse.xyz, 1.0f);

}
