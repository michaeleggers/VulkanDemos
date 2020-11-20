#include <stdio.h>

#define GLM_FORCE_RADIANS
// necessary so that glm::perspective produces z-clip coordinates 0<z_clip<w_clip
// see: https://matthewwellings.com/blog/the-new-vulkan-coordinate-system/
// see: https://twitter.com/pythno/status/1230478042096709632
#define GLM_FORCE_DEPTH_ZERO_TO_ONE 

#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_vulkan.h>

#include <assimp/cimport.h>        // Plain-C interface
#include <assimp/scene.h>          // Output data structure
#include <assimp/postprocess.h>    // Post processing flags

#include "platform.h"
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

// Definition der Kreiszahl 
#define GL_PI 3.1415f

#pragma pack(push, 16)
struct SceneUBO
{
	glm::mat4 view;
	glm::mat4 projection;
	glm::mat4 light_view_mat;
	glm::mat4 light_proj_mat;

	glm::vec4 light_pos;

	glm::vec2 light_radius_uv;
	float	  light_near;
	float	  light_far;
	uint32_t  filter_method;
	uint32_t  poisson_disk_size;
	uint32_t  use_textures;
	float     light_radius_bias;

	glm::vec3 model_pos;
};
#pragma pack(pop, 16)

struct ShadowUBO
{
	glm::mat4 view;
	glm::mat4 projection;
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
static Camera g_camera;
static int g_keys[GLFW_KEY_LAST];
static SceneUBO scene_data;
static ShadowUBO shadow_data;
static uint32_t framebuffer_resized;
static MouseState mouse_state;
static GLFWwindow * window;

// GLFW callbacks
static void glfw_key_callback(GLFWwindow * window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		printf("escape key pressed\n");
		glfwSetWindowShouldClose(window, GLFW_TRUE);
	}
	if (action == GLFW_PRESS) {
		if (key == GLFW_KEY_W) {
			g_keys[GLFW_KEY_W] = 1;
		}
		if (key == GLFW_KEY_S) {
			g_keys[GLFW_KEY_S] = 1;
		}
		if (key == GLFW_KEY_A) {
			g_keys[GLFW_KEY_A] = 1;
		}
		if (key == GLFW_KEY_D) {
			g_keys[GLFW_KEY_D] = 1;
		}
		if (key == GLFW_KEY_LEFT_ALT) {
			g_keys[GLFW_KEY_LEFT_ALT] = 1;
		}
	}
	else if (action == GLFW_RELEASE) {
		if (key == GLFW_KEY_W) {
			g_keys[GLFW_KEY_W] = 0;
		}
		if (key == GLFW_KEY_S) {
			g_keys[GLFW_KEY_S] = 0;
		}
		if (key == GLFW_KEY_A) {
			g_keys[GLFW_KEY_A] = 0;
		}
		if (key == GLFW_KEY_D) {
			g_keys[GLFW_KEY_D] = 0;
		}
		if (key == GLFW_KEY_LEFT_ALT) {
			g_keys[GLFW_KEY_LEFT_ALT] = 0;
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

glm::vec3 transformCoord(glm::mat4 m, glm::vec3 v)
{
	const float zero = (0.0);
	const float one = (1.0);
	glm::vec4 r = m * glm::vec4(v, one);
	float oow = r.w == zero ? one : (one / r.w);
	return glm::vec3(r) * oow;
}

int main() {

	init_window();
	init_platform(&p);
	
	char * device_extensions[] = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		VK_KHR_MAINTENANCE3_EXTENSION_NAME,
		VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME
	};
	uint32_t device_extension_count = sizeof(device_extensions) / sizeof(*device_extensions);

	char * instance_extensions[] = {
		VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME // TODO: remove this. Just here so the program doesn't crash. Mistake on my end...
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

	/* Configure the Vulkan Pipeline and Uniform Buffers*/
	// Descriptor Sets
	VkDescriptorSetLayoutBinding set_layout[] = {
		{
			0, // binding id ( matches with shader )
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			1, // number of resources with this layout binding
			VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT,
			0
		},
		{
			1,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			2, /* up to 10 textures */
			VK_SHADER_STAGE_FRAGMENT_BIT,
			0
		}
	};
	VkDescriptorSetLayoutBinding set_layout_depth_sampler = {
		0,
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		1,
		VK_SHADER_STAGE_FRAGMENT_BIT,
		0
	};

	VkDescriptorSetLayout descriptor_set_layouts[] =
	{
		vkal_create_descriptor_set_layout(set_layout, 2),
		vkal_create_descriptor_set_layout(&set_layout_depth_sampler, 1)
	};

	/* The order of the entries determines how the shaders access the sets! */
	VkDescriptorSetLayout vk_descriptor_set_layouts[] =
	{
		descriptor_set_layouts[0], /* in shader: set = 0 */
		descriptor_set_layouts[1]  /* in shader: set = 1 */
	};
	uint32_t descriptor_set_layout_count = sizeof(vk_descriptor_set_layouts)/sizeof(*vk_descriptor_set_layouts);

	VkDescriptorSet * descriptor_set_scene = (VkDescriptorSet*)malloc(sizeof(VkDescriptorSet));
    vkal_allocate_descriptor_sets(vkal_info->descriptor_pool, &vk_descriptor_set_layouts[0], 1, &descriptor_set_scene);
	VkDescriptorSet * descriptor_set_shadow = (VkDescriptorSet*)malloc(sizeof(VkDescriptorSet));
	vkal_allocate_descriptor_sets(vkal_info->descriptor_pool, &vk_descriptor_set_layouts[0], 1, &descriptor_set_shadow);


	// Shaders
	uint8_t * vertex_byte_code = 0;
	int vertex_code_size;
	p.rfb("shaders/shader.vert", &vertex_byte_code, &vertex_code_size);
	p.rfb("shaders/vert.spv", &vertex_byte_code, &vertex_code_size);
	uint8_t * fragment_byte_code = 0;
	int fragment_code_size;
	p.rfb("shaders/frag.spv", &fragment_byte_code, &fragment_code_size);
	ShaderStageSetup shader_setup = vkal_create_shaders(vertex_byte_code, vertex_code_size, fragment_byte_code, fragment_code_size);

	p.rfb("shaders/shadowmap_vert.spv", &vertex_byte_code, &vertex_code_size);
	p.rfb("shaders/shadowmap_frag.spv", &fragment_byte_code, &fragment_code_size);
	ShaderStageSetup shader_setup_shadowmap = vkal_create_shaders(vertex_byte_code, vertex_code_size, fragment_byte_code, fragment_code_size);

	p.rfb("shaders/lights_vert.spv", &vertex_byte_code, &vertex_code_size);
	p.rfb("shaders/lights_frag.spv", &fragment_byte_code, &fragment_code_size);
	ShaderStageSetup shader_setup_lights = vkal_create_shaders(vertex_byte_code, vertex_code_size, fragment_byte_code, fragment_code_size);

	VkPushConstantRange push_constant_ranges[] =
	{
		{ // Model Matrix
			VK_SHADER_STAGE_VERTEX_BIT,
			0, // offset
			sizeof(glm::mat4) // size, not sure if this has to be aligned... probably...
		},
		{ // Material Properties for Emissive, Ambient, Diffuse, Specular
			VK_SHADER_STAGE_FRAGMENT_BIT,
			sizeof(glm::mat4),
			sizeof(Material)
		}
	};

	VkPipelineLayout pipeline_layout_shadowmap = vkal_create_pipeline_layout(&vk_descriptor_set_layouts[0], 1, push_constant_ranges, 2);
	VkPipelineLayout pipeline_layout = vkal_create_pipeline_layout(vk_descriptor_set_layouts, descriptor_set_layout_count, push_constant_ranges, 2);

	VkPipeline graphics_pipeline = vkal_create_graphics_pipeline(
		shader_setup, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL, VK_CULL_MODE_BACK_BIT, VK_POLYGON_MODE_FILL,
		VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FRONT_FACE_COUNTER_CLOCKWISE, vkal_info->render_pass, pipeline_layout);

	VkPipeline shadow_pipeline = vkal_create_graphics_pipeline(
		shader_setup_shadowmap, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL, VK_CULL_MODE_BACK_BIT, VK_POLYGON_MODE_FILL,
		VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FRONT_FACE_COUNTER_CLOCKWISE, vkal_info->offscreen_pass.render_pass, pipeline_layout_shadowmap);

	VkPipeline graphics_pipeline_lights = vkal_create_graphics_pipeline(
		shader_setup_lights, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL, VK_CULL_MODE_NONE, VK_POLYGON_MODE_FILL,
		VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FRONT_FACE_COUNTER_CLOCKWISE, vkal_info->render_pass, pipeline_layout);

	// Uniform Buffers
	UniformBuffer uniform_buffer_handle = vkal_create_uniform_buffer(sizeof(SceneUBO), 0);
	vkal_update_descriptor_set_uniform(descriptor_set_scene[0], uniform_buffer_handle);
	// Initialize Uniform Buffer
	vkal_update_uniform(&uniform_buffer_handle, &scene_data);

	UniformBuffer uniform_buffer_handle_shadow = vkal_create_uniform_buffer(sizeof(ShadowUBO), 0 );
	vkal_update_descriptor_set_uniform(descriptor_set_shadow[0], uniform_buffer_handle_shadow);

	// Init Image Sampler to sample from Shadow Map in color pass
	create_offscreen_descriptor_set(descriptor_set_layouts[1], 0);

	// Init Camera
	g_camera.pos = glm::vec3(0, 0, 5);
	g_camera.center = glm::vec3(0, 0, 0);
	g_camera.up = glm::vec3(0, -1, 0);

	/* Use Textures at startup */
	scene_data.use_textures = 1;
	/* Search Radius Bias should be at least 1! */
	scene_data.light_radius_bias = 1.0f;

	/* Model Loading and Texture assignments */
	Model model_plane = create_model_from_file2("../assets/obj/plane.obj", p, NULL);
	model_plane.offset = vkal_vertex_buffer_add(model_plane.vertices, model_plane.vertex_count);
	glm::mat4 plane_scale = glm::scale(glm::mat4(1), glm::vec3(10));
	glm::mat4 translate = glm::translate(glm::mat4(1), glm::vec3(0.0f, 0.0f, 0.0f));
	glm::mat4 model_mat = translate * plane_scale;
	model_plane.pos = glm::vec3(0.0f, 0.0f, 0.0f);
	model_plane.model_matrix = glm::translate(glm::mat4(1), model_plane.pos) * plane_scale;
	model_plane.material.emissive = glm::vec4(0.2, 0.2, 0.5, 1);
	model_plane.material.diffuse = glm::vec4(1, 1, 1, 1);

	//Model model_tree = create_model_from_file2("../assets/obj/tree_single.obj", p, NULL);
	Model model_tree = create_model_from_file2("../assets/obj/torus.obj", p, NULL);
	model_tree.offset = vkal_vertex_buffer_add(model_tree.vertices, model_tree.vertex_count);
	model_tree.pos = glm::vec3(0.f, 0.f, 0.f);
	translate = glm::translate(glm::mat4(1), model_tree.pos);
	model_tree.model_matrix = translate;
	model_tree.material.diffuse = glm::vec4(1.0, 0.84, 0.0, 1);

	Model model_diffuse_light = create_model_from_file2("../assets/obj/sphere.obj", p, NULL);
	model_diffuse_light.offset = vkal_vertex_buffer_add(model_diffuse_light.vertices, model_diffuse_light.vertex_count);
	model_diffuse_light.material.emissive = glm::vec4(1, 1, 1, 1);
	model_diffuse_light.pos = glm::vec3(5, 8, 10);
	glm::mat4 light_scale = glm::scale(glm::mat4(1), glm::vec3(0.2));
	model_diffuse_light.model_matrix = glm::translate(glm::mat4(1), model_diffuse_light.pos) * light_scale;

	Image sand_image = load_image_file("../assets/textures/sand_diffuse.jpg");
	Image tree_image = load_image_file("../assets/md2/pknight/knight.png");
	
	Texture plane_texture = vkal_create_texture(1, sand_image.data, sand_image.width, sand_image.height, 4, 0,
		VK_IMAGE_VIEW_TYPE_2D, 0, 1, 0, 1, VK_FILTER_LINEAR, VK_FILTER_LINEAR);
	Texture tree_texture = vkal_create_texture(1, tree_image.data, tree_image.width, tree_image.height, 4, 0,
		VK_IMAGE_VIEW_TYPE_2D, 0, 1, 0, 1, VK_FILTER_LINEAR, VK_FILTER_LINEAR);
	
	assign_texture_to_model(&model_plane, plane_texture, 0, TEXTURE_TYPE_DIFFUSE);
	assign_texture_to_model(&model_tree, tree_texture, 1, TEXTURE_TYPE_DIFFUSE);
	model_tree.material.is_textured = 0;
	
	/* Update Descriptor Sets */
	vkal_update_descriptor_set_texturearray(descriptor_set_scene[0], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, model_plane.material.texture_id, model_plane.texture);
	vkal_update_descriptor_set_texturearray(descriptor_set_scene[0], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, model_tree.material.texture_id, model_tree.texture);

	// Init Shadow Data Stuff
	Camera shadow_cam = {};
	shadow_cam.pos = glm::vec3(scene_data.light_pos);
	shadow_cam.center = glm::vec3(0);
	shadow_cam.up = glm::vec3(0, -1, 0);
	glm::vec3 shadow_cam_direction = glm::normalize(shadow_cam.center - shadow_cam.pos);
	glm::vec3 shadow_cam_side = glm::normalize(glm::cross(shadow_cam_direction, shadow_cam.up));

	double timer_frequency = glfwGetTimerFrequency();
	double timestep = 1.f / timer_frequency; // step in second
	double title_update_timer = 0;
	double dt = 0;
	float light_animator = 0.f;

	int width = VKAL_SCREEN_WIDTH;
	int height = VKAL_SCREEN_HEIGHT;
	// MAIN LOOP
	while (!glfwWindowShouldClose(window)) {
		double start_time = glfwGetTime();

		glfwPollEvents();

		// Start the Dear ImGui frame
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		ImGui::Begin("Shadow Mapping");
		ImGui::Checkbox("Use Textures", (bool*)&scene_data.use_textures);
		ImGui::Text("Filter");
		ImGui::RadioButton("PCF", (int*)&scene_data.filter_method, 0);
		ImGui::RadioButton("PCSS", (int*)&scene_data.filter_method, 1);
		ImGui::RadioButton("No Shadow", (int*)&scene_data.filter_method, 2);
		ImGui::Text("Poisson Disk");
		ImGui::RadioButton("25", (int*)&scene_data.poisson_disk_size, 0);
		ImGui::RadioButton("32", (int*)&scene_data.poisson_disk_size, 1);
		ImGui::RadioButton("64", (int*)&scene_data.poisson_disk_size, 2);
		ImGui::RadioButton("2x2 fixed", (int*)&scene_data.poisson_disk_size, 3);
		ImGui::RadioButton("4x4 fixed", (int*)&scene_data.poisson_disk_size, 4);
		ImGui::RadioButton("16x16 fixed", (int*)&scene_data.poisson_disk_size, 5);
		ImGui::RadioButton("32x32 fixed", (int*)&scene_data.poisson_disk_size, 6);
		ImGui::RadioButton("64x64 fixed", (int*)&scene_data.poisson_disk_size, 7);
		//ImGui::SliderFloat3("Light Position", &model_diffuse_light.pos[0], -50, 50);
		static float light_size = 0.5f;
		ImGui::SliderFloat("Light Size", &light_size, 0, 10);
		static float light_cam_pov = 45.f;
		static float light_cam_near = 0.01f;
		static float light_cam_far = 1000.f;
		ImGui::SliderFloat("Light Cam POV", &light_cam_pov, 0.f, 180.f);
		ImGui::SliderFloat("Light Cam Near", &light_cam_near, 0.01f, 10.0f);
		ImGui::SliderFloat("Light Cam Far", &light_cam_far, 0.f, 2000.f);
		ImGui::SliderFloat("Search Radius Bias", &scene_data.light_radius_bias, 1.0, 10.0);
		ImGui::Text("Plane Model Properties");
		ImGui::SliderFloat3("Position", (float*)&scene_data.model_pos, -10.f, 10.f);

		ImGui::End();
		ImGui::Render();

		/* Update Light Model Matrix */
		static float light_pos_x = 1.0;
		static float light_pos_z = 1.0;
		light_pos_x += 0.001;
		light_pos_z += 0.001;
		model_diffuse_light.pos.x = 8.0f*cosf(light_pos_x);
		model_diffuse_light.pos.z = 8.0f*sinf(light_pos_z);
		model_diffuse_light.model_matrix = glm::translate(glm::mat4(1), model_diffuse_light.pos) * light_scale;

		// Need new dimensions?
		if (framebuffer_resized) {
			framebuffer_resized = 0;
			glfwGetFramebufferSize(window, &width, &height);
		}

		// Mouse update
		if (!ImGui::IsMouseHoveringAnyWindow() && !ImGui::IsAnyItemActive()) {
			update(&g_camera, dt);
		}

		// Update uniforms
		scene_data.projection = glm::perspective(glm::radians(45.f), float(width) / float(height), 0.1f, 1000.f);
		scene_data.view = glm::lookAt(g_camera.pos, g_camera.center, g_camera.up);
		scene_data.light_pos = glm::vec4(model_diffuse_light.pos, 1);

		/*update light view projection*/
		{
			shadow_cam.pos = glm::vec3(model_diffuse_light.pos);
			shadow_cam.center = glm::vec3(0);
			shadow_cam.up = glm::vec3(0, -1, 0);
			shadow_data.view = glm::lookAt(shadow_cam.pos, shadow_cam.center, shadow_cam.up);
			
			/* Get the extents of the Model that casts a shadow in Light's viewspace*/
			glm::vec4 mins = model_tree.bounding_box.min_xyz;
			glm::vec4 maxs = model_tree.bounding_box.max_xyz;
			//mins += glm::vec4(model_tree.pos, 1.0f);
			//maxs += glm::vec4(model_tree.pos, 1.0f);
			//nv::vec3f(maxs[0] - mins[0], maxs[1] - mins[1], maxs[2] - mins[2]) * 0.5f;
			//m_center = nv::vec3f(mins[0], mins[1], mins[2]) + m_extents;
			glm::vec3 extents = glm::vec3(maxs.x - mins.x, maxs.y - mins.y, maxs.z - mins.z) * 0.5f;
			glm::vec3 center = glm::vec3(mins.x, mins.y, mins.z) + extents;


			glm::vec3 box[2];
			box[0] = center - extents;
			box[1] = center + extents;
			glm::vec3 bbox[2];

			bbox[0][0] = bbox[0][1] = bbox[0][2] = 99999.9f;
			bbox[1][0] = bbox[1][1] = bbox[1][2] = -99999.9f;
			// Transform the vertices of box and extend bbox accordingly
			for (int i = 0; i < 8; ++i)
			{
				glm::vec3 v = glm::vec3(
					box[(i & 1) ? 0 : 1][0],
					box[(i & 2) ? 0 : 1][1],
					box[(i & 4) ? 0 : 1][2]);

				glm::vec3 v1 = transformCoord(shadow_data.view, v);
				for (int j = 0; j < 3; ++j)
				{
					bbox[0][j] = min(bbox[0][j], v1[j]);
					bbox[1][j] = max(bbox[1][j], v1[j]);
				}
			}

			float frustumWidth = max(glm::abs(bbox[0].x), glm::abs(bbox[1].x)) * 2.0f;
			float frustumHeight = max(glm::abs(bbox[0].y), glm::abs(bbox[1].y)) * 2.0f;
			float aspect = frustumWidth / frustumHeight;
			float z_near = -bbox[1][2];
			float z_far = light_cam_far;
			float frustumHeightHalf = (frustumHeight / 2.0f);
			float frustumWidthHalf = (frustumWidth / 2.0f);
			float ymin = -frustumHeightHalf;
			float ymax = frustumHeightHalf;
			float xmin = ymin * aspect;
			float xmax = ymax * aspect;
			shadow_data.projection = glm::frustum(xmin, xmax, ymin, ymax, z_near, z_far);
			//shadow_data.projection = glm::perspective(glm::radians(light_cam_pov), 1.f, light_cam_near, light_cam_far);
			//shadow_data.projection = glm::ortho(-.5f, .5f, -5.f, .5f, 1.f, 1000.f);

			scene_data.light_view_mat = shadow_data.view;
			scene_data.light_proj_mat = shadow_data.projection;
			scene_data.light_near = z_near;
			scene_data.light_far = light_cam_far;
			scene_data.light_radius_uv = glm::vec2(light_size / frustumWidth, light_size / frustumHeight);
		}


		vkal_update_uniform(&uniform_buffer_handle, &scene_data);
		vkal_update_uniform(&uniform_buffer_handle_shadow, &shadow_data);

		/* Plane model's position is adjustable */
		model_plane.model_matrix = glm::translate(glm::mat4(1), model_plane.pos + scene_data.model_pos) * plane_scale;

		Model models[] = { model_plane, model_tree };

		uint32_t image_id = vkal_get_image();
		if (image_id >= 0) {
			// shadow-map
			Model shadow_models[] = { model_tree, model_plane }; // Models affected by light
			update_shadow_command_buffer(image_id, shadow_models, 2, shadow_pipeline, pipeline_layout_shadowmap, &descriptor_set_shadow[0], 0, 1);
			offscreen_buffers_submit(image_id);

			// Stuff the user actually sees
			vkal_begin_command_buffer(vkal_info->command_buffers[image_id]);
			
			vkal_begin_render_pass(image_id, vkal_info->command_buffers[image_id], vkal_info->render_pass);
			vkal_bind_descriptor_set(image_id, 0, &descriptor_set_scene[0], 1, pipeline_layout);
			vkal_bind_descriptor_set(image_id, 1, &vkal_info->offscreen_pass.descriptor_sets[0], 1, pipeline_layout);
			vkal_record_models_pc(image_id, graphics_pipeline, pipeline_layout, models, 2);

			// render lights
			vkal_record_models_pc(image_id, graphics_pipeline_lights, pipeline_layout, &model_diffuse_light, 1);

			// ImGui
			ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), vkal_info->command_buffers[image_id]);
			vkal_end_renderpass(vkal_info->command_buffers[image_id]);
			
			vkal_end_command_buffer(vkal_info->command_buffers[image_id]);

			// Submit to GPU
			VkCommandBuffer command_buffers[] = { vkal_info->command_buffers[image_id] };
			vkal_queue_submit(command_buffers, 1);
			
			/* Blit to surface */
			vkal_present(image_id);
		}

		double end_time = glfwGetTime();
		dt = end_time - start_time;
		title_update_timer += dt;

#ifndef _DEBUG
		if ((title_update_timer) > 1.f) {
			char window_title[256];
			sprintf(window_title, "frametime: %fms (%f FPS)", dt * 1000.f, 1.f / dt);
			glfwSetWindowTitle(window, window_title);
			title_update_timer = 0;
		}
#endif
	}

	ImGui_ImplVulkan_Shutdown();
	vkal_cleanup();

	return 0;
}

