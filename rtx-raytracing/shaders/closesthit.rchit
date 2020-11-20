#version 460
#extension GL_NV_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 5) rayPayloadInNV vec3 hitValue;
layout(location = 3) rayPayloadNV   bool is_lit;
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
	vec3   pos;
	float  radius;
	uint   sample_count;
	uint   noise_function;
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
	vec3 tangent;
	float dummy; /* necessary to get data from storage buffer via indexing. Dummy will get it to 16*float*/
};

/* Vertex array is actually just an array of all the vertex data. Stride for getting pos, uv, normals, etc. has to be computed by user! */
layout(binding = 3, set = 0) buffer Vertices { vec4 v[]; } vertices[]; 
layout(binding = 4, set = 0) buffer Indices  { uint i[]; } indices[];

/* Constants */
const float PI               = 3.14159265359f;
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
	/* Remember: vertices is just an array of vec4.
	   So when accessing the data as vec4's we have to jump
	   to the beginning of each by a multiple of them.
	*/
	vec4 d0 = vertices[gl_InstanceID].v[4 * index + 0];
	vec4 d1 = vertices[gl_InstanceID].v[4 * index + 1];
	vec4 d2 = vertices[gl_InstanceID].v[4 * index + 2];
	vec4 d3 = vertices[gl_InstanceID].v[4 * index + 3];

	Vertex v;
	v.pos = vec3(d0.x, d0.y, d0.z);
	v.uv = vec2(d0.w, d1.x);
	v.normal = vec3(d1.y, d1.z, d1.w);
	v.color = d2;
	v.tangent = d3.xyz;

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
	
	/* Interpolate hitpoint */
	vec3 hitpoint = barys.x*v0.pos + barys.y*v1.pos + barys.z*v2.pos;
	
	/* Interpolate normal */
	vec3 normal = normalize(barys.x*v0.normal + barys.y*v1.normal + barys.z*v2.normal);

	/* light stuff */
	vec3 light_dir       = normalize(light.pos);
	float light_distance = length(light_dir);
	float light_atten    = max( dot(light_dir, normal), 0.0 );

	/* Shadow */
	/* Convert ray from worldspace to a pixel-position on the virtual viewplane in screenspace */
	uint frame         = misc.frame % 64;
	float aspect_ratio = misc.screen_width >= misc.screen_height ? float(misc.screen_width) / float(misc.screen_height) : float(misc.screen_height) / float(misc.screen_width);
	vec4 ray_clipspace = cam.proj * cam.view * vec4(gl_WorldRayOriginNV + gl_WorldRayDirectionNV, 1.0f);
	vec3 ray_ndc       = normalize(ray_clipspace/ray_clipspace.w).xyz;
	vec3 pixel_pos     = 0.5f * ray_ndc + 0.5f;
	pixel_pos          = vec3(misc.screen_width*1.f/aspect_ratio*pixel_pos.x, misc.screen_height*pixel_pos.y, 0.f);

	vec3 light_tangent       = normalize( cross(light_dir, vec3(0, 1, 0)) );
	vec3 light_bitangent     = normalize( cross(light_dir, light_tangent) );
	float shadow_term        = 1.0f;

	/* bluenoise sampling setup */
	float bluenoise = texture(bluenoise_texture, pixel_pos.xy).r;
	bluenoise = fract(bluenoise + c_goldenRatioConjugate * float(frame));
	float theta = bluenoise * 2.0f * PI;
	float cosTheta = cos(theta);
	float sinTheta = sin(theta);

	if (light_atten > 0.0) { /* only sample from light when our normal vector is not perpendicular to  the light vector! */
		for (uint i = 0; i < light.sample_count; ++i) {
			
			vec2 disk_point;

			if (light.noise_function == 0) { /* blue noise */
				if (i >= 64) break; /* precomputed blue-noise disk is only 64 wide! */
				vec2 sample_pos = BlueNoiseInDisk[i];
				/* Apply rotation matrix:
				   | cosTheta -sinTheta 0 |
				   | sinTheta  cosTheta 0 |
				*/
				disk_point = vec2(sample_pos.x * cosTheta - sample_pos.y * sinTheta, sample_pos.x * sinTheta + sample_pos.y * cosTheta);
				disk_point *= light.radius;
			}
			else { /* white noise */
				vec2 rng = hash23( vec3( pixel_pos.xy, float( frame * light.sample_count + i) ) );
				/* point in unit disk */
				float point_radius = light.radius * sqrt(rng.x); /* normal -> uniform distribution */
				float point_angle  = rng.y * 2.0f * PI;
				disk_point    = vec2(point_radius * cos(point_angle), point_radius * sin(point_angle));
			}
			vec3 shadow_ray_dir = normalize(light_dir + disk_point.x*light_tangent + disk_point.y*light_bitangent);
		
			/* trace ray towards light-disk and check if we "see" the light. */
			is_lit = false;
			uint rayFlags = gl_RayFlagsOpaqueNV | gl_RayFlagsTerminateOnFirstHitNV | gl_RayFlagsSkipClosestHitShaderNV;
			uint cullMask = 0xff;
			float tmin = 0.000;
			float tmax = 100.0;
			float bias = 0.0;
			//if (dot(normal, shadow_ray_dir) < 0.1) {
				bias = 0.001;
			//}
			vec3 origin = (gl_WorldRayOriginNV + gl_WorldRayDirectionNV * gl_HitTNV);// + bias*normal;
			origin += bias * (-gl_WorldRayDirectionNV);
			traceNV(topLevelAS, rayFlags, cullMask, 0, 0, 1, origin, tmin, shadow_ray_dir, tmax, 3);
			
			shadow_term = mix( shadow_term, is_lit ? 1.0f : 0.0f, 1.0f/float(i+1) );
		}
	}

	hitValue = barys;
	hitValue = shadow_term * light_atten * (0.5*normal + 0.5);
	hitValue = shadow_term * (0.5*normal + 0.5);

	hitValue = vec3(shadow_term);
}
