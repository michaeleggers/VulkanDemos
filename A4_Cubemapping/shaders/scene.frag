#version 450
#extension GL_ARB_separate_shader_objects : enable


layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform UBO
{
    mat4 view;
    mat4 projection;
	uint texture_mapping_type;
};

layout(set = 0, binding = 1) uniform samplerCube cubemap_sampler;

layout (location = 1) in vec4 viewspace_pos;
layout (location = 2) in vec4 viewspace_normal;
layout (location = 3) in vec4 position_ndc;
layout (location = 4) in vec2 uv;
layout (location = 5) in mat4 view_matrix;
layout (location = 9) in mat4 model_matrix;

layout (push_constant) uniform Material_t
{
    layout (offset = 64)  vec4 emissive;
	layout (offset = 80)  vec4 ambient;
	layout (offset = 96)  vec4 diffuse;
	layout (offset = 112) vec4 specular;
} Material;

vec3 g_light_pos2 = vec3(0, 0, 10);

vec3 material_diffuse_color  = vec3(1.f, 0.f, 0.f);

void main() 
{   
	
	vec4 color = vec4(0);

	if (texture_mapping_type == 0) { // OBJECT LINEAR
		
		color = texture(cubemap_sampler, (inverse(view_matrix) * viewspace_pos).xyz);	
	}
	else if (texture_mapping_type == 1) {// EYE LINEAR
		vec4 eye_plane_s = inverse(view_matrix*model_matrix) * vec4(10, 0, 0, 1);
		vec4 eye_plane_t = inverse(view_matrix*model_matrix) * vec4(0, 10, 0, 1);
		vec4 eye_plane_r = inverse(view_matrix*model_matrix) * vec4(0, 0, 10, 1);

		vec3 tex_coords = vec3(dot(eye_plane_s, viewspace_pos), dot(eye_plane_t, viewspace_pos), dot(eye_plane_r, viewspace_pos));
	    color = texture(cubemap_sampler, tex_coords);
	}
	else { // Using Cubemap for reflection

		vec3 frag_to_camera = normalize(-viewspace_pos.xyz);
		vec3 reflection = normalize(reflect(-frag_to_camera, normalize(viewspace_normal.xyz)));
		vec3 uvw = vec3(inverse(view_matrix) * vec4(reflection, 0.f));
		color = texture(cubemap_sampler, uvw);
	
	}

    outColor = vec4(color.xyz, 1.f);
}