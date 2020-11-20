#version 460
#extension GL_NV_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 2) rayPayloadInNV   vec3 hitValueDebug;
hitAttributeNV vec3 attribs;

layout(binding = 0, set = 0) uniform accelerationStructureNV topLevelAS;
layout(binding = 2, set = 0) uniform CameraProperties 
{
    mat4 view;
	mat4 proj;
	mat4 viewInverse;
	mat4 projInverse;
} cam;

/* NOTE: Most convenient if we upload this in view-space coordinates (see Sascha Willems) */
layout (binding = 0, set = 1) uniform LightUBO
{
	vec4   pos;
	float  radius;
} light;

layout (binding = 1, set = 1) uniform miscUBO
{
	uint screen_width;
	uint screen_height;
	uint frame;
} misc;

layout (binding = 2, set = 1) uniform sampler2D bluenoise_texture;

struct Vertex
{
	vec3 pos;
	vec2 uv;
	vec3 normal;
	//vec3 barycentric;
	vec4 color;
	//float dummy; /* necessary to get data from storage buffer via indexing. Dummy will get it to 16*float*/
};

/* Vertex array is actually just an array of all the vertex data. Stride for getting pos, uv, normals, etc. has to be computed by user! */
layout(binding = 3, set = 0) buffer Vertices { vec4 v[]; } vertices[]; 
layout(binding = 4, set = 0) buffer Indices  { uint i[]; } indices[];

/* Constants */
const float PI               = 3.14159265359f;
const uint sample_count      = 64;
const vec2 BlueNoiseInDisk[64] = vec2[64](
    vec2(0.478712,0.875764),
    vec2(-0.337956,-0.793959),
    vec2(-0.955259,-0.028164),
    vec2(0.864527,0.325689),
    vec2(0.209342,-0.395657),
    vec2(-0.106779,0.672585),
    vec2(0.156213,0.235113),
    vec2(-0.413644,-0.082856),
    vec2(-0.415667,0.323909),
    vec2(0.141896,-0.939980),
    vec2(0.954932,-0.182516),
    vec2(-0.766184,0.410799),
    vec2(-0.434912,-0.458845),
    vec2(0.415242,-0.078724),
    vec2(0.728335,-0.491777),
    vec2(-0.058086,-0.066401),
    vec2(0.202990,0.686837),
    vec2(-0.808362,-0.556402),
    vec2(0.507386,-0.640839),
    vec2(-0.723494,-0.229240),
    vec2(0.489740,0.317826),
    vec2(-0.622663,0.765301),
    vec2(-0.010640,0.929347),
    vec2(0.663146,0.647618),
    vec2(-0.096674,-0.413835),
    vec2(0.525945,-0.321063),
    vec2(-0.122533,0.366019),
    vec2(0.195235,-0.687983),
    vec2(-0.563203,0.098748),
    vec2(0.418563,0.561335),
    vec2(-0.378595,0.800367),
    vec2(0.826922,0.001024),
    vec2(-0.085372,-0.766651),
    vec2(-0.921920,0.183673),
    vec2(-0.590008,-0.721799),
    vec2(0.167751,-0.164393),
    vec2(0.032961,-0.562530),
    vec2(0.632900,-0.107059),
    vec2(-0.464080,0.569669),
    vec2(-0.173676,-0.958758),
    vec2(-0.242648,-0.234303),
    vec2(-0.275362,0.157163),
    vec2(0.382295,-0.795131),
    vec2(0.562955,0.115562),
    vec2(0.190586,0.470121),
    vec2(0.770764,-0.297576),
    vec2(0.237281,0.931050),
    vec2(-0.666642,-0.455871),
    vec2(-0.905649,-0.298379),
    vec2(0.339520,0.157829),
    vec2(0.701438,-0.704100),
    vec2(-0.062758,0.160346),
    vec2(-0.220674,0.957141),
    vec2(0.642692,0.432706),
    vec2(-0.773390,-0.015272),
    vec2(-0.671467,0.246880),
    vec2(0.158051,0.062859),
    vec2(0.806009,0.527232),
    vec2(-0.057620,-0.247071),
    vec2(0.333436,-0.516710),
    vec2(-0.550658,-0.315773),
    vec2(-0.652078,0.589846),
    vec2(0.008818,0.530556),
    vec2(-0.210004,0.519896) 
);
const float c_goldenRatioConjugate = 0.61803398875f; // also just fract(goldenRatio)

/* white noise, from https://www.shadertoy.com/view/4djSRW */
vec2 hash23(vec3 p3)
{
	p3 = fract(p3 * vec3(.1031, .1030, .0973));
    p3 += dot(p3, p3.yzx+33.33);
    return fract((p3.xx+p3.yz)*p3.zy);
}

Vertex unpack(uint index)
{
	vec4 d0 = vertices[gl_InstanceID].v[3 * index + 0];
	vec4 d1 = vertices[gl_InstanceID].v[3 * index + 1];
	vec4 d2 = vertices[gl_InstanceID].v[3 * index + 2];

	Vertex v;
	v.pos = vec3(d0.x, d0.y, d0.z);
	v.uv = vec2(d0.w, d1.x);
	v.normal = vec3(d1.y, d1.z, d1.w);
	v.color = d2;
	return v;
}

void main()
{
    ivec3 ind = ivec3(
		indices[gl_InstanceID].i[3*gl_PrimitiveID + 0],
		indices[gl_InstanceID].i[3*gl_PrimitiveID + 1],
		indices[gl_InstanceID].i[3*gl_PrimitiveID + 2]
	);

	Vertex v0 = unpack(ind.x);
	Vertex v1 = unpack(ind.y);
	Vertex v2 = unpack(ind.z);

	const vec3 barys = vec3(1.0f - attribs.x - attribs.y, attribs.x, attribs.y);
    vec3 normal = normalize(barys.x*v0.normal + barys.y*v1.normal + barys.z*v2.normal);
    hitValueDebug = 0.5*normal+0.5;
}
