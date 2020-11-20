#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_vulkan.h>


#include "platform.h"
#include "camera.h"
#include "model.h"
#include "vkal.h"
#include "vkal_imgui.h"


#define massert(statement) {\
if (!statement) { \
		printf("Assertion failed! LINE: %d, FILE: %s\n", __LINE__, __FILE__); \
		printf("Press any key to continue...\n");\
		getchar();\
		exit(-1);\
}\
}

#define GL_PI 3.1415f

struct ViewProjectionUBO
{
	glm::mat4 view;
	glm::mat4 projection;
};

struct LightUBO
{
	glm::vec3 pos;
	float     radius;
	uint32_t  sample_count;
	uint32_t  noise_function;
	uint32_t  use_textures;
};

struct MiscUBO
{
	uint32_t screen_width;
	uint32_t screen_height;
	uint32_t frame;
};

struct RaytracerUBO
{
	glm::mat4 view;
	glm::mat4 proj;
	glm::mat4 view_inverse;
	glm::mat4 proj_inverse;
};

struct PostprocessUBO
{
	uint32_t pass_id;
	uint32_t use_gauss_filter;
};

struct MouseState
{
	double xpos, ypos;
	double xpos_old, ypos_old;
	uint32_t left_button_down;
	uint32_t right_button_down;
};


/* GLOBALS */
static Platform p;
static Camera camera;
static int keys[GLFW_KEY_LAST];
static ViewProjectionUBO view_projection_data;
static LightUBO light_ubo;
static MiscUBO  misc_ubo;
static RaytracerUBO raytracing_ubo;
static PostprocessUBO postprocess_data;
static uint32_t framebuffer_resized;
static MouseState mouse_state;
static GLFWwindow * window;

static void update(Camera * camera, float dt)
{
	if (mouse_state.left_button_down || mouse_state.right_button_down) {
		mouse_state.xpos_old = mouse_state.xpos;
		mouse_state.ypos_old = mouse_state.ypos;
		glfwGetCursorPos(window, &mouse_state.xpos, &mouse_state.ypos);
		double dx = mouse_state.xpos - mouse_state.xpos_old;
		double dy = mouse_state.ypos - mouse_state.ypos_old;
		if (mouse_state.left_button_down) {
			rotate_camera(camera, dt, (float)dx, (float)dy);
		}
		if (mouse_state.right_button_down) {
			dolly_camera(camera, dt, dx);
		}
	}
	else {
		glfwGetCursorPos(window, &mouse_state.xpos, &mouse_state.ypos);
		mouse_state.xpos_old = mouse_state.xpos;
		mouse_state.ypos_old = mouse_state.ypos;
	}
}

// GLFW callbacks
static void glfw_key_callback(GLFWwindow * window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		printf("escape key pressed\n");
		glfwSetWindowShouldClose(window, GLFW_TRUE);
	}
	if (action == GLFW_PRESS) {
		if (key == GLFW_KEY_W) {
			keys[GLFW_KEY_W] = 1;
		}
		if (key == GLFW_KEY_S) {
			keys[GLFW_KEY_S] = 1;
		}
		if (key == GLFW_KEY_A) {
			keys[GLFW_KEY_A] = 1;
		}
		if (key == GLFW_KEY_D) {
			keys[GLFW_KEY_D] = 1;
		}
		if (key == GLFW_KEY_LEFT_ALT) {
			keys[GLFW_KEY_LEFT_ALT] = 1;
		}
	}
	else if (action == GLFW_RELEASE) {
		if (key == GLFW_KEY_W) {
			keys[GLFW_KEY_W] = 0;
		}
		if (key == GLFW_KEY_S) {
			keys[GLFW_KEY_S] = 0;
		}
		if (key == GLFW_KEY_A) {
			keys[GLFW_KEY_A] = 0;
		}
		if (key == GLFW_KEY_D) {
			keys[GLFW_KEY_D] = 0;
		}
		if (key == GLFW_KEY_LEFT_ALT) {
			keys[GLFW_KEY_LEFT_ALT] = 0;
		}
	}
}

static void glfw_framebuffer_size_callback(GLFWwindow * window, int width, int height)
{
	framebuffer_resized = 1;
	printf("window was resized\n");
}

static void glfw_mouse_button_callback(GLFWwindow * window, int mouse_button, int action, int mods)
{
	if (mouse_button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
		mouse_state.left_button_down = 1;
	}
	if (mouse_button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
		mouse_state.left_button_down = 0;
	}
	if (mouse_button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
		mouse_state.right_button_down = 1;
	}
	if (mouse_button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE) {
		mouse_state.right_button_down = 0;
	}
}

void init_window() {
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	window = glfwCreateWindow(VKAL_SCREEN_WIDTH, VKAL_SCREEN_HEIGHT, "Vulkan", 0, 0);
	glfwSetKeyCallback(window, glfw_key_callback);
	glfwSetMouseButtonCallback(window, glfw_mouse_button_callback);
	glfwSetFramebufferSizeCallback(window, glfw_framebuffer_size_callback);
}

int main() {
	init_window();
	init_platform(&p);
	void * memory = p.initialize_memory(100 * 1024 * 1024);
	MemoryArena model_memory;
	initialize_arena(&model_memory, memory, 100 * 1024 * 1024);

	char * device_extensions[] = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		// the following are required for using the nvidia raytracing extensions
		VK_NV_RAY_TRACING_EXTENSION_NAME,
		VK_KHR_MAINTENANCE3_EXTENSION_NAME,
		VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME,
		VK_KHR_16BIT_STORAGE_EXTENSION_NAME,
		VK_KHR_8BIT_STORAGE_EXTENSION_NAME,
		VK_KHR_STORAGE_BUFFER_STORAGE_CLASS_EXTENSION_NAME,
		VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME
	};
	uint32_t device_extension_count = sizeof(device_extensions) / sizeof(*device_extensions);

	char * instance_extensions[] = {
		VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME
#ifdef _DEBUG
		,VK_EXT_DEBUG_UTILS_EXTENSION_NAME
#endif
	};
	uint32_t instance_extension_count = sizeof(instance_extensions) / sizeof(*instance_extensions);

	VkalInfo * vkal_info = init_vulkan(
		window,
		device_extensions, device_extension_count,
		instance_extensions, instance_extension_count);
	init_imgui(vkal_info, window);
	init_raytracing();
	create_rt_storage_image();
	create_rt_target_image();

	/* Init Postprocess/Rasterizer-Pipelines, Shaders and Descriptor Sets */

	/* Postprocessing Descriptor Set */
	VkDescriptorSetLayoutBinding postprocess_set_layouts[] = {
		{   
			0,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			1,
			VK_SHADER_STAGE_FRAGMENT_BIT,
			0
		},
		{
			1,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			1,
			VK_SHADER_STAGE_FRAGMENT_BIT,
			0
		},
		{
			2,
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			1,
			VK_SHADER_STAGE_FRAGMENT_BIT,
			0
		}
	};
	VkDescriptorSetLayout postprocess_descriptor_set_layout = vkal_create_descriptor_set_layout(
		postprocess_set_layouts, 3);
	VkDescriptorSetLayout postprocess_layouts[] = {
		postprocess_descriptor_set_layout
	};
	uint32_t postprocess_layout_count = sizeof(postprocess_layouts) / sizeof(*postprocess_layouts);
	VkDescriptorSet * postprocess_descriptor_sets = (VkDescriptorSet*)malloc(sizeof(VkDescriptorSet));
	vkal_allocate_descriptor_sets(vkal_info->descriptor_pool, postprocess_layouts, 1, &postprocess_descriptor_sets);

	/* Rasterizer Descriptor Set */
	VkDescriptorSetLayoutBinding rasterizer_set_layouts[] = {
		{ /* Camera properties */
			0,
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			1,
			VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
			0
		},
		{ /* Light porperties */
			1, 
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			1,
			VK_SHADER_STAGE_FRAGMENT_BIT,
			0
		},
		{ /* Model diffuse textures */
			2,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			2,
			VK_SHADER_STAGE_FRAGMENT_BIT,
			0
		}
	};
	VkDescriptorSetLayout rasterizer_descriptor_set_layout = vkal_create_descriptor_set_layout(
		rasterizer_set_layouts, 3);
	VkDescriptorSetLayout rasterizer_layouts[] = {
		rasterizer_descriptor_set_layout
	};
	uint32_t rasterizer_layout_count = sizeof(rasterizer_layouts) / sizeof(*rasterizer_layouts);
	VkDescriptorSet * rasterizer_descriptor_sets = (VkDescriptorSet*)malloc(sizeof(VkDescriptorSet));
	vkal_allocate_descriptor_sets(vkal_info->descriptor_pool, rasterizer_layouts, 1, &rasterizer_descriptor_sets);

	/* Vertex/-Fragment shaders for postprocessing pipeline */
	uint8_t * vertex_byte_code = 0;
	int vertex_code_size;
	p.rfb("shaders\\postprocess_vert.spv", &vertex_byte_code, &vertex_code_size);
	uint8_t * fragment_byte_code = 0;
	int fragment_code_size;
	p.rfb("shaders\\postprocess_frag.spv", &fragment_byte_code, &fragment_code_size);
	ShaderStageSetup postprocess_shaders = vkal_create_shaders(vertex_byte_code, vertex_code_size, fragment_byte_code, fragment_code_size);

	/* Vertex/-Fragment shaders for rasterizer pipeline */
	vertex_byte_code = 0;
	vertex_code_size;
	p.rfb("shaders\\rasterizer_vert.spv", &vertex_byte_code, &vertex_code_size);
	fragment_byte_code = 0;
	fragment_code_size;
	p.rfb("shaders\\rasterizer_frag.spv", &fragment_byte_code, &fragment_code_size);
	ShaderStageSetup rasterizer_shaders = vkal_create_shaders(vertex_byte_code, vertex_code_size, fragment_byte_code, fragment_code_size);


	VkPipelineLayout postprocess_layout = vkal_create_pipeline_layout(postprocess_layouts, 1, NULL, 0);
	VkPipeline postprocess_pipeline = vkal_create_graphics_pipeline(
		postprocess_shaders, VK_TRUE, VK_COMPARE_OP_LESS, VK_CULL_MODE_NONE,
		VK_POLYGON_MODE_FILL, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FRONT_FACE_COUNTER_CLOCKWISE,
		vkal_info->render_pass, postprocess_layout);

	VkPushConstantRange pc[] = {
		{
			VK_SHADER_STAGE_VERTEX_BIT,
			0,
			sizeof(glm::mat4)
		},
		{
			VK_SHADER_STAGE_FRAGMENT_BIT,
			sizeof(glm::mat4), /* we reserve this for the model-matrix to be conform with the other examples */
			sizeof(Material)
		}
	};
	VkPipelineLayout rasterizer_layout = vkal_create_pipeline_layout(rasterizer_layouts, 1, pc, 2);
	VkPipeline rasterizer_pipeline = vkal_create_graphics_pipeline(
		rasterizer_shaders, VK_TRUE, VK_COMPARE_OP_LESS, VK_CULL_MODE_NONE,
		VK_POLYGON_MODE_FILL, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FRONT_FACE_COUNTER_CLOCKWISE,
		vkal_info->render_to_image_render_pass, rasterizer_layout);
	UniformBuffer rasterizer_modelview_ubo = vkal_create_uniform_buffer(sizeof(ViewProjectionUBO), 0);
	vkal_update_descriptor_set_uniform(rasterizer_descriptor_sets[0], rasterizer_modelview_ubo);
	UniformBuffer rasterizer_light_ubo = vkal_create_uniform_buffer(sizeof(LightUBO), 1);
	vkal_update_descriptor_set_uniform(rasterizer_descriptor_sets[0], rasterizer_light_ubo);

	RenderImage render_image = create_render_image(VKAL_SCREEN_WIDTH, VKAL_SCREEN_HEIGHT);
	vkal_dbg_image_name(get_image(render_image.image), "Render Image");
	VkSampler sampler = create_sampler(VK_FILTER_NEAREST, VK_FILTER_NEAREST, 
		VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, 
		VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER);
	vkal_descriptor_set_add_image_sampler(postprocess_descriptor_sets[0], 0, get_image_view(vkal_info->nv_rt_ctx.target_image.image_view), sampler);
	vkal_descriptor_set_add_image_sampler(postprocess_descriptor_sets[0], 1, get_image_view(render_image.image_view), sampler);
	UniformBuffer postprocess_ubo = vkal_create_uniform_buffer(sizeof(PostprocessUBO), 2);
	vkal_update_descriptor_set_uniform(postprocess_descriptor_sets[0], postprocess_ubo);

	/* End Graphics Pipeline Setup */

	/* Configure the Vulkan Pipeline and Uniform Buffers*/
	// Descriptor Sets
	VkDescriptorSetLayoutBinding set_layout[] = {
		{
			0,
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			1,
			VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV | VK_SHADER_STAGE_VERTEX_BIT,
			0
		},
		{
			1,
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			1,
			VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV | VK_SHADER_STAGE_RAYGEN_BIT_NV,
			0
		},
		{
			2,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			1,
			VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV,
			0
		}
	};
	VkDescriptorSetLayout descriptor_set_layout = vkal_create_descriptor_set_layout(set_layout, 3);
	VkDescriptorSetLayout layouts[] = {
		descriptor_set_layout
	};
	uint32_t layout_count = sizeof(layouts) / sizeof(*layouts);

	// Initialize Uniform Buffers
	UniformBuffer light_ubo_buffer = vkal_create_uniform_buffer(sizeof(LightUBO), 0);
	light_ubo.sample_count = 32;
	light_ubo.radius = 0.5f;
	light_ubo.noise_function = 0;
	light_ubo.pos = glm::vec3(12.0f, 7.f, 0.f);

	UniformBuffer misc_ubo_buffer = vkal_create_uniform_buffer(sizeof(MiscUBO), 1);
	misc_ubo.frame = 0;
	misc_ubo.screen_width  = VKAL_RT_SIZE_X;
	misc_ubo.screen_height = VKAL_RT_SIZE_Y;

	/* Load bluenoise texture from disk and upload as texture to GPU */
	Image bluenoise_image = load_image_file("../assets/textures/bluenoise/128_128/LDR_RGB1_1.png");
	Texture bluenoise_texture = vkal_create_texture(
		2,
		bluenoise_image.data, bluenoise_image.width, bluenoise_image.height, bluenoise_image.channels,
		0, VK_IMAGE_VIEW_TYPE_2D,
		0, 1, 0, 1,
		VK_FILTER_NEAREST, VK_FILTER_NEAREST);

	// Init Camera
	camera.pos = glm::vec3(0, 0, 5);
	camera.center = glm::vec3(0, 0, 0);
	camera.up = glm::vec3(0, 1, 0);

	// Model and Projection Matrices
	float near_plane = 0.1f;
	float far_plane = 1000.0f;
	glm::mat4 projection = glm::perspective(glm::radians(45.f), 1.f, near_plane, far_plane);

	/* Geometry data*/
	/* Unitplane for putting the rasterizer/ratracer images on it during postprocessing. */
	Model unit_plane = create_model_from_file2("../assets/obj/unitplane.obj", p, &model_memory);
	unit_plane.offset = vkal_vertex_buffer_add(unit_plane.vertices, unit_plane.vertex_count);
	unit_plane.index_buffer_offset = vkal_index_buffer_add(unit_plane.indices, unit_plane.index_count);
	unit_plane.model_matrix = glm::mat4(1);

	/* Load models from file and assign textures. */
	//Model mario_model = create_model_from_file2("../assets/obj/spheres.obj", p, &model_memory);
	Model mario_model = create_model_from_file2("../assets/obj/pknight_small.obj", p, &model_memory);
	mario_model.offset = vkal_vertex_buffer_add(mario_model.vertices, mario_model.vertex_count);
	mario_model.index_buffer_offset = vkal_index_buffer_add(mario_model.indices, mario_model.index_count);
	mario_model.material.diffuse = glm::vec4(1.0, 0.84, 0.0, 1.0);
	mario_model.model_matrix = glm::mat4(1);
	Image mario_image = load_image_file("../assets/md2/pknight/knight.png");
	Texture mario_texture = vkal_create_texture(2, mario_image.data, mario_image.width, mario_image.height, mario_image.channels, 0,
		VK_IMAGE_VIEW_TYPE_2D, 0, 1, 0, 1, VK_FILTER_LINEAR, VK_FILTER_LINEAR);
	assign_texture_to_model(&mario_model, mario_texture, 0, TEXTURE_TYPE_DIFFUSE);
	mario_model.material.is_textured = 1; /* deactivate texturing manually for now, because I do not have an appropriate palmtree texture. */
	vkal_update_descriptor_set_texturearray(rasterizer_descriptor_sets[0], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, mario_model.texture.binding, mario_model.material.texture_id, mario_model.texture);

	Model plane_model = create_model_from_file2("../assets/obj/plane.obj", p, &model_memory);
	plane_model.offset = vkal_vertex_buffer_add(plane_model.vertices, plane_model.vertex_count);
	plane_model.index_buffer_offset = vkal_index_buffer_add(plane_model.indices, plane_model.index_count);
	plane_model.model_matrix = glm::mat4(1);
	plane_model.material.diffuse = glm::vec4(1, 1, 1, 1);
	Image plane_image = load_image_file("../assets/textures/sand_diffuse.jpg");
	Texture plane_texture = vkal_create_texture(2, plane_image.data, plane_image.width, plane_image.height, plane_image.channels, 0,
		VK_IMAGE_VIEW_TYPE_2D, 0, 1, 0, 1, VK_FILTER_LINEAR, VK_FILTER_LINEAR);
	assign_texture_to_model(&plane_model, plane_texture, 1, TEXTURE_TYPE_DIFFUSE);
	vkal_update_descriptor_set_texturearray(rasterizer_descriptor_sets[0], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, plane_model.texture.binding, plane_model.material.texture_id, plane_model.texture);
	
	/* Allocate device memory to store models' indices/vertices into a storage buffer. This is important for the raytracing shaders to
	   unpack triangles from.
	*/
	DeviceMemory vertex_memory = vkal_allocate_devicememory(512 * 1024 * 1024,
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	DeviceMemory index_memory = vkal_allocate_devicememory(512*1024*1024,
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	
	/* Create buffers for the models */
	/* Buffers for model 1*/
	Buffer mario_vertex_buffer = vkal_create_buffer(mario_model.vertex_count * sizeof(Vertex), &vertex_memory, 
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
	vkal_dbg_buffer_name(mario_vertex_buffer, "RT Vertex Buffer Mario");
	map_memory(&mario_vertex_buffer, mario_model.vertex_count * sizeof(Vertex), mario_vertex_buffer.offset);
	memcpy(mario_vertex_buffer.mapped, mario_model.vertices, mario_model.vertex_count * sizeof(Vertex));
	unmap_memory(&mario_vertex_buffer);
	
	Buffer mario_index_buffer = vkal_create_buffer(mario_model.index_count * sizeof(uint32_t), &index_memory, 
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
	vkal_dbg_buffer_name(mario_index_buffer, "RT Index Buffer Mario");
	map_memory(&mario_index_buffer, mario_model.index_count * sizeof(uint32_t), mario_index_buffer.offset);
	memcpy(mario_index_buffer.mapped, mario_model.indices, mario_model.index_count * sizeof(uint32_t));
	unmap_memory(&mario_index_buffer);

	/* Buffers for model 2*/
	Buffer plane_vertex_buffer = vkal_create_buffer(plane_model.vertex_count * sizeof(Vertex), &vertex_memory, 
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
	vkal_dbg_buffer_name(plane_vertex_buffer, "RT Vertex Buffer Plane");
	map_memory(&plane_vertex_buffer, plane_model.vertex_count * sizeof(Vertex), plane_vertex_buffer.offset);
	memcpy(plane_vertex_buffer.mapped, plane_model.vertices, plane_model.vertex_count * sizeof(Vertex));
	unmap_memory(&plane_vertex_buffer);
	
	Buffer plane_index_buffer = vkal_create_buffer(plane_model.index_count * sizeof(uint32_t), &index_memory, 
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
	vkal_dbg_buffer_name(plane_index_buffer, "RT Index Buffer Plane");
	map_memory(&plane_index_buffer, plane_model.index_count * sizeof(uint32_t), plane_index_buffer.offset);
	memcpy(plane_index_buffer.mapped, plane_model.indices, plane_model.index_count * sizeof(uint32_t));
	unmap_memory(&plane_index_buffer);

	/* Convert our model to VkGeometryNV. The model data is expected to be in this form
	   by the BLAS. NOTE: we build one BLAS per object.
	*/
	VkGeometryNV mario_geometryNV = model_to_geometryNV(mario_model);
	VkGeometryNV plane_geometryNV = model_to_geometryNV(plane_model);
	VkGeometryNV geometryNVs[] = { mario_geometryNV, plane_geometryNV };
	create_rt_blas(geometryNVs, 2);

	/* Create the TLAS instances. We give it the handle of the previously generated BLAS.
	   NOTE: We generate one instance per BLAS. If we want to have multiple objects of the same
	   BLAS(= Model) then it makes sense to use multiple instances per BLAS, as the geometry is the same
	   but maybe not the transformation matrix. This is the same thing as in rasterization: We wouldn't
	   store the same model geometry on the GPU but referring to it with different textures, model-matrices, etc...
	*/
	DeviceMemory instance_memory = vkal_allocate_devicememory(16 * 1024 * 1024,
		VK_BUFFER_USAGE_RAY_TRACING_BIT_NV,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	Buffer instance_buffer = vkal_create_buffer(vkal_info->nv_rt_ctx.blas_count*sizeof(GeometryInstance),
		&instance_memory, VK_BUFFER_USAGE_RAY_TRACING_BIT_NV);
	vkal_dbg_buffer_name(instance_buffer, "RT TLAS Instance Buffer");
	map_memory(&instance_buffer, instance_buffer.size, instance_buffer.offset);
	
	/* Fill the instance-buffer with as many instances as models to be rendered.
	   NOTE: It is possible to create more instances than BLAS (=models).
	*/
	glm::mat3x4 transform = {
		1.f, 0.f, 0.f, 0.f,
		0.f, 1.f, 0.f, 0.f,
		0.f, 0.f, 1.f, 0.f
	};
	for (int i = 0; i < vkal_info->nv_rt_ctx.blas_count; ++i) {
		GeometryInstance geometry_instance = {};
		geometry_instance.transform = transform;
		geometry_instance.instance_id = i;  // accessed in shaders through gl_InstanceID
		geometry_instance.mask = 0xFF;
		geometry_instance.hit_group_id = 0; // index-offset into shader binding table hitgroup-list
		geometry_instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_CULL_DISABLE_BIT_NV;
		geometry_instance.acceleration_structure_handle = vkal_info->nv_rt_ctx.blas[i].handle;
		memcpy( ((GeometryInstance*)instance_buffer.mapped) + i, &geometry_instance, sizeof(GeometryInstance));
	}
	create_rt_tlas(vkal_info->nv_rt_ctx.blas_count);

	/* Build the Acceleration Structure */
	build_rt_acceleration_structure(geometryNVs, 2, instance_buffer.buffer);
	
	/* create RayTracing uniform buffer for (inverse) view-projection matrices*/
	vkal_info->nv_rt_ctx.rt_uniform_buffer_device_memory = vkal_allocate_devicememory(
		16 * 1024 * 1024,
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	vkal_info->nv_rt_ctx.ubo = vkal_create_buffer(
		sizeof(RaytracerUBO),
		&vkal_info->nv_rt_ctx.rt_uniform_buffer_device_memory,
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
	vkal_dbg_buffer_name(vkal_info->nv_rt_ctx.ubo, "RT global Uniform Buffer");
	map_memory(
		&vkal_info->nv_rt_ctx.ubo,
		sizeof(RaytracerUBO), vkal_info->nv_rt_ctx.ubo.offset);

	/* Allocate Raytracing descriptor set. It will setup the required descriptors for storage-image,
	   TLAS and a uniform-buffer, which contains raytracing specific information such as inverse view- and modelmatrices.
	   User can provide additional descriptors.
	   NOTE: All user provided sets start with set = 1!
	*/
	create_rt_descriptor_sets(layouts, layout_count);
	
	/* For now, the write descriptor sets have to be updated by the user manually!
	   TODO: Just pass the UnifromBuffer object! When creating the UniformBuffer assign the
	   correct descriptor-set to it. That means, that the UniformBuffer creation can only
	   happen after the descriptor set has been allocated! This will result in 
	   less friction becasuse we already introduced some kind of high-level interface for
	   uniform buffers with the UniformBuffer struct. So it makes sense to keep it simple and
	   not just put one kind of complexity into another.
	*/
	vkal_update_descriptor_set_uniform(vkal_info->nv_rt_ctx.descriptor_sets[1], light_ubo_buffer);
	vkal_update_descriptor_set_uniform(vkal_info->nv_rt_ctx.descriptor_sets[1], misc_ubo_buffer);
	vkal_update_descriptor_set_texture(vkal_info->nv_rt_ctx.descriptor_sets[1], bluenoise_texture);

	vkal_update_descriptor_set_bufferarray(vkal_info->nv_rt_ctx.descriptor_sets[0], VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3, 0, mario_vertex_buffer);
	vkal_update_descriptor_set_bufferarray(vkal_info->nv_rt_ctx.descriptor_sets[0], VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3, 1, plane_vertex_buffer);

	vkal_update_descriptor_set_bufferarray(vkal_info->nv_rt_ctx.descriptor_sets[0], VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 4, 0, mario_index_buffer);
	vkal_update_descriptor_set_bufferarray(vkal_info->nv_rt_ctx.descriptor_sets[0], VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 4, 1, plane_index_buffer);


	/* Raytracing Pipeline with descriptor set and shaders */
	create_rt_pipeline(layouts, layout_count);

	/* Shader binding table: Binds shaders to TLAS */
	create_rt_shader_binding_table();

	/* We use dedicated command buffers for doing RayTracing */
	create_rt_command_buffers();

	/* Record into the RayTracing command buffers. Also takes care of image transitions */
	build_rt_commandbuffers();

	double timer_frequency = glfwGetTimerFrequency();
	double timestep = 1.f / timer_frequency; // step in second
	double title_update_timer = 0;
	double dt = 0;

	int width = VKAL_SCREEN_WIDTH;
	int height = VKAL_SCREEN_HEIGHT;

	/* Initialize some Uniforms */
	postprocess_data.use_gauss_filter = 0;

	/* Main Render Loop */
	uint32_t current_frame = 0;

	while (!glfwWindowShouldClose(window)) {
		double start_time = glfwGetTime();

		glfwPollEvents();

		/* Build the ImGUI Frame */
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		ImGui::Begin("Raytraced Shadow Demo");
		ImGui::Checkbox("Use Textures", (bool*)&light_ubo.use_textures);
		ImGui::Text("Light Parameters");
		//ImGui::SliderFloat3("Position", &light_ubo.pos[0], -20.f, 20.f);
		ImGui::SliderFloat("Radius", &light_ubo.radius, 0.0f, 1.0f);
		ImGui::SliderInt("Sample Count", (int*)&light_ubo.sample_count, 1, 128);
		ImGui::Text("Noise Function");
		const char* items[] = { "Blue Noise", "White Noise" };
		static const char* current_item = items[0];
		if (ImGui::BeginCombo("##combo", current_item)) // The second parameter is the label previewed before opening the combo.
		{
			for (int n = 0; n < IM_ARRAYSIZE(items); n++)
			{
				bool is_selected = (current_item == items[n]); // You can store your selection however you want, outside or inside your objects
				if (ImGui::Selectable(items[n], is_selected)) {
					current_item = items[n];
					light_ubo.noise_function = n;
				}
				if (is_selected)
					ImGui::SetItemDefaultFocus();   // You may set the initial focus when opening the combo (scrolling + for keyboard navigation support)
			}
			ImGui::EndCombo();
		}
		
		ImGui::Text("Image Select");
		const char * image_items[] = { "Final", "Shadow", "Color" };
		static const char * current_image = image_items[0];
		if (ImGui::BeginCombo("combo", current_image)) {
			for (int n = 0; n < IM_ARRAYSIZE(image_items); n++) {
				bool is_selected = (current_image == image_items[n]);
				if (ImGui::Selectable(image_items[n], is_selected)) {
					current_image = image_items[n];
					postprocess_data.pass_id = n;
				}
				if (is_selected) {
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}
		ImGui::End();
		ImGui::Render();

		/* Need new dimensions? */
		if (framebuffer_resized) {
			framebuffer_resized = 0;
			glfwGetFramebufferSize(window, &width, &height);
		}

		/* Only update our scene (camera movement etc.) when mouse cursor is NOT on any ImGUI element */
		if (!ImGui::IsMouseHoveringAnyWindow() && !ImGui::IsAnyItemActive()) {
			update(&camera, dt);
		}

		view_projection_data.projection = glm::perspective(glm::radians(45.f), float(width) / float(height), near_plane, far_plane);
		view_projection_data.view = glm::lookAt(camera.pos, camera.center, camera.up);
		current_frame++;
		misc_ubo.frame = current_frame;

		vkal_update_uniform(&misc_ubo_buffer, &misc_ubo);
		vkal_update_uniform(&rasterizer_modelview_ubo, &view_projection_data);
		vkal_update_uniform(&rasterizer_light_ubo, &light_ubo);
		static float light_pos_x = 1.0f;
		static float light_pos_z = 1.0f;
		light_pos_x += 0.03f;
		light_pos_z += 0.03f;
		light_ubo.pos.x = 5.0f*cosf(light_pos_x);
		light_ubo.pos.z = 5.0f*sinf(light_pos_z);
		light_ubo.pos.y = 2.0f;
		vkal_update_uniform(&light_ubo_buffer, &light_ubo);
		vkal_update_uniform(&postprocess_ubo, &postprocess_data);

		uint32_t image_id = vkal_get_image();
		if (image_id >= 0) {

			raytracing_ubo.proj = view_projection_data.projection;
			raytracing_ubo.view = view_projection_data.view;
			raytracing_ubo.proj_inverse = glm::inverse(view_projection_data.projection);
			raytracing_ubo.view_inverse = glm::inverse(view_projection_data.view);
			memcpy(vkal_info->nv_rt_ctx.ubo.mapped, &raytracing_ubo, sizeof(RaytracerUBO));

			vkal_begin_command_buffer(vkal_info->command_buffers[image_id]);
			
			/* Rasterizer: render models as usual */
			Model models_to_rasterize[] = { mario_model, plane_model };
			vkal_render_to_image(image_id, vkal_info->command_buffers[image_id], vkal_info->render_to_image_render_pass, render_image);
			vkal_bind_descriptor_set(image_id, 0, &rasterizer_descriptor_sets[0], 1, rasterizer_layout);
			vkal_record_models_dimensions(image_id, rasterizer_pipeline, rasterizer_layout, NULL, 0, models_to_rasterize, 2, float(VKAL_SCREEN_WIDTH), float(VKAL_SCREEN_HEIGHT));
			vkal_end_renderpass(vkal_info->command_buffers[image_id]);

			/* Postprocess: combine raytraced shadows with rasterized image */
			vkal_begin_render_pass(image_id, vkal_info->command_buffers[image_id], vkal_info->render_pass);
			vkal_bind_descriptor_set(image_id, 0, &postprocess_descriptor_sets[0], 1, postprocess_layout);
			vkal_record_models(image_id, postprocess_pipeline, postprocess_layout, NULL, 0, &unit_plane, 1);
			/* Record ImGUI Draw Data and draw functions into command buffer */
			ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), vkal_info->command_buffers[image_id]);
			vkal_end_renderpass(vkal_info->command_buffers[image_id]);
 			
			vkal_end_command_buffer(vkal_info->command_buffers[image_id]);

			/* Submit raytracing and rasterization/postprocess commands to GPU's queue and execute */
			VkCommandBuffer command_buffers[] = { vkal_info->nv_rt_ctx.command_buffers[image_id], vkal_info->command_buffers[image_id] };
			vkal_queue_submit(command_buffers, 2);

			/* Present to screen (swap) */
			vkal_present(image_id);
		}

		double end_time = glfwGetTime();
		dt = end_time - start_time;
		title_update_timer += dt;

		if ((title_update_timer) > .5f) {
			char window_title[256];
			sprintf(window_title, "frametime: %fms (%f FPS)", (dt * 1000.f), 1.0f/dt);
			glfwSetWindowTitle(window, window_title);
			title_update_timer = 0;
		}
	}

	ImGui_ImplVulkan_Shutdown();
	vkal_cleanup();

	return 0;
}

