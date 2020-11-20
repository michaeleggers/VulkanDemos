#version 450
#extension GL_ARB_separate_shader_objects : enable


layout(location = 0) out vec4 outColor;
layout (location = 0) in vec2 in_uv;

layout (binding = 0, set = 0) uniform sampler2D raytracing_shadow_texture;
layout (binding = 1, set = 0) uniform sampler2D rasterizer_output_texture;
layout (binding = 2, set = 0) uniform PassUBO 
{
	uint      id;
	uint      use_gauss_filter;
} pass;


const float PI               = 3.14159265359f;

/* white noise, from https://www.shadertoy.com/view/4djSRW */
vec2 hash23(vec3 p3)
{
	p3 = fract(p3 * vec3(.1031, .1030, .0973));
    p3 += dot(p3, p3.yzx+33.33);
    return fract((p3.xx+p3.yz)*p3.zy);
}

void main() 
{
    vec4 shadow;
	shadow = texture (raytracing_shadow_texture, in_uv);

	vec4 color  = texture (rasterizer_output_texture, in_uv);
	vec3 final_color = shadow.xyz * color.xyz;
	
	if (pass.id == 0) {
		outColor = vec4(final_color, 1.0f);
	}
	else if (pass.id == 1) {
		outColor = vec4(shadow.xyz, 1.0f);
	}
	else {
		outColor = vec4(color.xyz, 1.0f);
	}
}
