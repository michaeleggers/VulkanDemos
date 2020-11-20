#version 450
#extension GL_ARB_separate_shader_objects : enable


layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform UBO
{
    vec4 draw_wireframe;
    vec4 light_color;
    mat4 unused1;
    mat4 unused2;
	float time;
	float delta_time;
};

layout (location = 1) in vec4 viewspace_pos;
layout (location = 2) in vec4 viewspace_normal;
layout (location = 3) in vec3 barycentric;
layout (location = 4) in flat vec4 color;
layout (location = 5) in vec4 position_ndc;
layout (location = 6) in vec2 uv;
layout (location = 7) in mat4 view_matrix;

layout (push_constant) uniform Material_t
{
    layout (offset = 64)  vec4 emissive;
	layout (offset = 80)  vec4 ambient;
	layout (offset = 96)  vec4 diffuse;
	layout (offset = 112) vec4 specular;
} Material;



float random (vec2 st) 
{
    return fract(sin(dot(st.xy,
                         vec2(12.9898,78.233)))*43758.345);
}

void main() 
{

	// WIREFRAME
    // closest fragment to edge   
    float minbary = min( min(barycentric.x, barycentric.y ), barycentric.z);
    // sloap (rate of change of the barycentric coordinate-component within 2x2 area, 
    // see https://developer.download.nvidia.com/cg/fwidth.html)
    float delta = fwidth(minbary);
    // smoothstep interpolates from 0 to 1 based on minbary compared to 0 and delta.
    minbary = smoothstep(0.f, delta, minbary);
    vec3 wireframe_color = vec3(0.f, 1.f, 0.f);
    vec4 wireframe = vec4(vec3(1.f), 0.f);
    if (draw_wireframe.x > 0) {
		float intensity = clamp(abs(sin(time*1.5f)*.5f) + 0.25f, 0.f, 1.f);
        wireframe = vec4(intensity*wireframe_color, 1.f-minbary);
    }
   
	// NOTE: when wireframe drawing is disabled there is some visual glitching (looks like z-fighting).
	//       Not sure why, though...
    outColor = vec4(color.xyz, 1.f);

}
