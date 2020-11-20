#version 450
#extension GL_ARB_separate_shader_objects : enable

vec2 positions[3] = vec2[](
    vec2(0.0, -0.5),
    vec2(0.5, 0.5),
    vec2(-0.5, 0.5)
);

vec2 uvs[3] = vec2[](
    vec2(1, 0),
    vec2(0.5, 1),
    vec2(0.0, 0)
);

layout(binding = 0, set = 0) uniform CameraProperties 
{
    mat4 view;
	mat4 proj;
} cam;

layout (push_constant) uniform Material
{
	layout (offset = 0)   mat4 model_mat;
} material_info;

layout (location = 0) in vec3 position;
layout (location = 1) in vec2 uv;
layout (location = 2) in vec3 normal;
layout (location = 3) in vec4 color;
layout (location = 4) in vec3 tangent;

layout (location = 0) out vec2 out_uv;
layout (location = 1) out vec4 out_position;
layout (location = 2) out vec4 out_normal;
void main()
{
	gl_Position = cam.proj * cam.view * material_info.model_mat * vec4(position, 1.0);
	out_uv = uv;
    out_position = cam.view * material_info.model_mat * vec4(position, 1.f);
    out_normal = cam.view * material_info.model_mat * vec4(normal.xyz, 0.f);
}
