#version 450
#extension GL_ARB_separate_shader_objects : enable

vec2 positions[3] = vec2[](
    vec2(0.0, -0.5),
    vec2(0.5, 0.5),
    vec2(-0.5, 0.5)
);

vec3 colors[3] = vec3[](
    vec3(1.0, 0.0, 0.0),
    vec3(0.0, 1.0, 0.0),
    vec3(0.0, 0.0, 1.0)
);

layout (location = 0) in vec3 position;
layout (location = 1) in vec2 uv;
layout (location = 2) in vec3 normal;
layout (location = 3) in vec4 color;
layout (location = 4) in vec3 tangent;

layout (set = 0, binding = 0) uniform UBO
{
    mat4 view;
    mat4 projection;
};

layout(set = 0, binding = 1) uniform LightUBO
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

layout (set = 0, binding = 3) uniform ModelUBO
{
	mat4 model_mat;
} Model;

layout (location = 0) out vec4 color_out;
layout (location = 1) out vec4 position_ndc;
layout (location = 2) out vec2 uv_out;
layout (location = 3) out vec3 out_shading;
layout (location = 4) out mat4 view_out;

void main()
{
    gl_Position = projection * view * Model.model_mat * vec4(position, 1.0);
    
    vec3 viewspace_pos       = (view * Model.model_mat * vec4(position, 1.f)).xyz;
	vec3 viewspace_light_pos = (view * light_pos).xyz;
    vec3 viewspace_normal    = normalize((view * Model.model_mat * vec4(normal, 0.f)).xyz);
	vec3 light_dir           = normalize(viewspace_light_pos - viewspace_pos);
	vec3 view_dir            = normalize(-viewspace_pos);

	/* Ambient Part */
	vec3 ambient = light_color.xyz * Material.ambient.xyz;

	/* Diffuse Part */
	float diffuse_strength = max(dot(viewspace_normal, view_dir), 0.0f);
	vec3 diffuse = diffuse_strength * Material.diffuse.xyz;

	/* Specular Part */
	vec3 reflect_light = normalize(reflect(-light_dir, viewspace_normal));
	float specular_strength = clamp(dot(reflect_light, view_dir), 0, 1);
	specular_strength = pow(specular_strength, Material.specular.w);
	vec3 specular = specular_strength * Material.specular.xyz;


    uv_out           = uv;
	view_out         = view;
	color_out        = color;
    position_ndc     = gl_Position;
	out_shading      = vec3(Material.emissive.xyz + ambient + diffuse + specular);
}
