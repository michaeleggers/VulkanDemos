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

layout (location = 0) in vec3 position;
layout (location = 1) in vec2 uv;
layout (location = 2) in vec3 normal;
//layout (location = 3) in vec3 barycentric;
layout (location = 3) in vec4 color;
layout (location = 4) in vec3 tangent;

layout (location = 0) out vec2 out_uv;

void main()
{
	gl_Position = vec4(position, 1.0);
	out_uv = uv;
}
