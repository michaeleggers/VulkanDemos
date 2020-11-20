#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#undef min
#undef max

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_vulkan.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <modelloader\Model.h>

#include "platform.h"
#include "vkal.h"

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

struct ViewProjectionUBO
{
	glm::mat4 view;
	glm::mat4 projection;
};

struct SettingsUBO
{
	glm::vec4 light_pos;
	glm::vec4 light_color;
	uint32_t show_normals;
};

struct MaterialUBO
{
	glm::vec4 emissive;
	glm::vec4 ambient;
	glm::vec4 diffuse;
	glm::vec4 specular;
};

struct ModelUBO
{
	glm::mat4 model_mat;
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
static SettingsUBO settings_data;
static MaterialUBO material_data;
static ModelUBO model_data;
static uint32_t framebuffer_resized;
static MouseState mouse_state;
static bool update_pipeline;
static bool draw_normals;
static GLFWwindow * window;
static Model g_model;
static Model g_model_normals;
static float angle_threshold = 0;

#define SPEED 0.01f
static void update(float dt)
{
	if (keys[GLFW_KEY_W]) {
		glm::vec3 direction = glm::normalize(camera.center - camera.pos);
		camera.pos += SPEED*direction*dt;
		camera.center += SPEED * direction * dt;
	}
	if (keys[GLFW_KEY_S]) {
		glm::vec3 direction = glm::normalize(camera.center - camera.pos);
		camera.pos -= SPEED*direction*dt;
		camera.center -= SPEED * direction * dt;
	}
	if (keys[GLFW_KEY_A]) {
		glm::vec3 direction = glm::normalize(camera.center - camera.pos);
		glm::vec3 side = glm::normalize(glm::cross(direction, camera.up));
		camera.pos -= SPEED*side * dt;
		camera.center -= SPEED*side*dt;
	}
	if (keys[GLFW_KEY_D]) {
		glm::vec3 direction = glm::normalize(camera.center - camera.pos);
		glm::vec3 side = glm::normalize(glm::cross(direction, camera.up));
		camera.pos += SPEED*side*dt;
		camera.center += SPEED*side*dt;
	}
}

float rand_between(float min, float max)
{
	float range = max - min;
	float step = range / RAND_MAX;
	return (step * rand()) + min;
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

struct Image_t
{
	int width, height, channels;
	unsigned char * data;
};

Image_t load_image(const char * file)
{
	Image_t image;
	image.data = stbi_load(file, &image.width, &image.height, &image.channels, 4);
	massert(image.data);
	return image;
}

void free_image(Image_t image)
{
	massert(image.data)
		stbi_image_free(image.data);
}

void check_vk_result(VkResult err)
{
	if (err == 0) return;
	printf("VkResult %d\n", err);
	if (err < 0)
abort();
}

void init_imgui(VkalInfo * vkal_info)
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGui::StyleColorsDark();
	QueueFamilyIndicies indicies = find_queue_families(vkal_info->physical_device, vkal_info->surface);

	ImGui_ImplVulkan_InitInfo initInfo = {};
	initInfo.Instance = vkal_info->instance;
	initInfo.PhysicalDevice = vkal_info->physical_device;
	initInfo.Device = vkal_info->device;
	initInfo.QueueFamily = indicies.graphics_family;
	initInfo.Queue = vkal_info->graphics_queue;
	initInfo.DescriptorPool = vkal_info->descriptor_pool;
	initInfo.CheckVkResultFn = check_vk_result;

	ImGui_ImplGlfw_InitForVulkan(window, false);
	ImGui_ImplVulkan_Init(&initInfo, vkal_info->imgui_render_pass); // ToDo: RenderPass

	{
		VkCommandBufferAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = vkal_info->command_pools[0];
		allocInfo.commandBufferCount = 1;

		VkCommandBuffer command_buffer;
		vkAllocateCommandBuffers(vkal_info->device, &allocInfo, &command_buffer);

		VkCommandBufferBeginInfo begin_info = {};
		begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		VkResult err = vkBeginCommandBuffer(command_buffer, &begin_info);
		check_vk_result(err);

		ImGui_ImplVulkan_CreateFontsTexture(command_buffer);

		VkSubmitInfo end_info = {};
		end_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		end_info.commandBufferCount = 1;
		end_info.pCommandBuffers = &command_buffer;
		err = vkEndCommandBuffer(command_buffer);
		check_vk_result(err);
		err = vkQueueSubmit(vkal_info->graphics_queue, 1, &end_info, VK_NULL_HANDLE);
		check_vk_result(err);

		err = vkDeviceWaitIdle(vkal_info->device);
		check_vk_result(err);
		ImGui_ImplVulkan_InvalidateFontUploadObjects();

		vkFreeCommandBuffers(vkal_info->device, vkal_info->command_pools[0], 1, &command_buffer);
	}
}

void rotate_camera(Camera * camera, float up_angle, float right_angle)
{
	glm::vec3 camera_dir = glm::normalize(camera->center - camera->pos);
	camera->right = glm::normalize(glm::cross(camera->up, camera_dir));
	camera->up = glm::vec3(0, -1, 0); // glm::normalize(glm::cross(oriented_camera.right, camera_dir));

	glm::mat3 rotate_up = glm::rotate(glm::mat4(1.f), glm::radians(up_angle), camera->up);
	glm::mat3 rotate_right = glm::rotate(glm::mat4(1.f), glm::radians(right_angle), camera->right);
	camera->pos = rotate_up * rotate_right * camera->pos;
}

glm::vec3 compute_face_normal(obj::Triangle const * tri)
{
	obj::Vertex * v1 = tri->vertex[0];
	obj::Vertex * v2 = tri->vertex[1];
	obj::Vertex * v3 = tri->vertex[2];
	glm::vec3 glm_v1(v1->pos.x, v1->pos.y, v1->pos.z);
	glm::vec3 glm_v2(v2->pos.x, v2->pos.y, v2->pos.z);
	glm::vec3 glm_v3(v3->pos.x, v3->pos.y, v3->pos.z);
	glm::vec3 v1v2 = glm_v1 - glm_v2;
	glm::vec3 v1v3 = glm_v1 - glm_v3;
	return glm::normalize(glm::cross(v1v2, v1v3));
}

void update_geometry()
{
	obj::Model model("../assets/obj/cylinder.obj");
	uint32_t normal_index = 0;

	for (int i = 0; i < model.GetTriangleCount(); ++i) {
		obj::Triangle * tri = model.GetTriangle(i);
		glm::vec3 face_normal = compute_face_normal(tri);

		for (int j = 0; j < 3; ++j) {
			obj::Vertex * vertex = tri->vertex[j];
			glm::vec3 vertex_pos = glm::vec3(vertex->pos.x, vertex->pos.y, vertex->pos.z);
			g_model.vertices[3 * i + j].pos = vertex_pos;

			std::vector<obj::Triangle*> adj_tris;
			model.GetAdjacentTriangles(adj_tris, vertex);
			std::vector<glm::vec3> normals;
			normals.push_back(face_normal);

			for (int k = 0; k < adj_tris.size(); ++k) {
				glm::vec3 candidate = compute_face_normal(adj_tris[k]);
				bool should_add = true;
				if (1 - glm::abs(glm::dot(face_normal, candidate)) > angle_threshold) continue;

				for (int n = 0; n < normals.size(); ++n) {
					glm::vec3 normal = normals[n];
					glm::vec3 v = normal - candidate;
					float EPSILON = 10e-5f;
					if (glm::length(v) < EPSILON) {
						should_add = false;
						break;
					}
				}
				if (should_add) normals.push_back(candidate);
			}

			glm::vec3 mean_normal(0);
			for (int k = 0; k < normals.size(); ++k) mean_normal += normals[k];
			mean_normal /= normals.size();
			mean_normal = glm::normalize(mean_normal);
			g_model.vertices[3 * i + j].normal = mean_normal;
			g_model_normals.vertices[normal_index++].pos = vertex_pos;
			g_model_normals.vertices[normal_index++].pos = vertex_pos + 0.5f*mean_normal;
		}
	}

	g_model_normals.vertex_count = normal_index - 1;
}

void setup_geometry()
{
	obj::Model model("../assets/obj/cylinder.obj");
	g_model.vertices = (Vertex*)malloc(3 * model.GetTriangleCount() * sizeof(Vertex));
	g_model_normals.vertices = (Vertex*)malloc(2 * 3 * model.GetTriangleCount() * sizeof(Vertex));
	uint32_t normal_index = 0;

	for (int i = 0; i < model.GetTriangleCount(); ++i) {
		obj::Triangle * tri = model.GetTriangle(i);
		glm::vec3 face_normal = compute_face_normal(tri);
		
		for (int j = 0; j < 3; ++j) {
			obj::Vertex * vertex = tri->vertex[j];
			glm::vec3 vertex_pos = glm::vec3(vertex->pos.x, vertex->pos.y, vertex->pos.z);
			g_model.vertices[3 * i + j].pos = vertex_pos;

			std::vector<obj::Triangle*> adj_tris;
			model.GetAdjacentTriangles(adj_tris, vertex);
			std::vector<glm::vec3> normals;
			normals.push_back(face_normal);
		
			for (int k = 0; k < adj_tris.size(); ++k) {
				glm::vec3 candidate = compute_face_normal(adj_tris[k]);
				bool should_add = true;
				if (1 - glm::abs(glm::dot(face_normal, candidate)) > angle_threshold) continue;
				
				for (int n = 0; n < normals.size(); ++n) {
					glm::vec3 normal = normals[n];
					glm::vec3 v = normal - candidate;
					float EPSILON = 10e-5f;
					if (glm::length(v) < EPSILON) {
						should_add = false;
						break;
					}
				}
				if (should_add) normals.push_back(candidate);			
			}

			glm::vec3 mean_normal(0);
			for (int k = 0; k < normals.size(); ++k) mean_normal += normals[k];
			mean_normal /= normals.size();
			mean_normal = glm::normalize(mean_normal);
			g_model.vertices[3 * i + j].normal = mean_normal;
			g_model_normals.vertices[normal_index++].pos = vertex_pos;
			g_model_normals.vertices[normal_index++].pos = vertex_pos + 0.5f*mean_normal;
		}
	}

	g_model.vertex_count = 3 * model.GetTriangleCount();
	g_model_normals.vertex_count = normal_index - 1;
	g_model.pos = glm::vec3(0);
	g_model_normals.pos = glm::vec3(0);
	glm::mat4 translate = glm::translate(glm::mat4(1), g_model.pos);
	g_model.model_matrix = translate;
	g_model_normals.model_matrix = translate;
	g_model.offset = vkal_vertex_buffer_add(g_model.vertices, g_model.vertex_count);
	g_model_normals.offset = vkal_vertex_buffer_add(g_model_normals.vertices, g_model_normals.vertex_count);
	g_model_normals.material.emissive = glm::vec4(0, 0.1, 0, 1);
	g_model.material.emissive = glm::vec4(0.1, 0, 0.1, 1);
}

void update_camera(float dt)
{
	if (mouse_state.left_button_down || mouse_state.right_button_down) {
		mouse_state.xpos_old = mouse_state.xpos;
		mouse_state.ypos_old = mouse_state.ypos;
		glfwGetCursorPos(window, &mouse_state.xpos, &mouse_state.ypos);
		double dx = mouse_state.xpos - mouse_state.xpos_old;
		double dy = mouse_state.ypos - mouse_state.ypos_old;
		if (mouse_state.left_button_down) {
			rotate_camera(&camera, (float)dx, (float)dy);
		}
		if (mouse_state.right_button_down) {
			glm::vec3 view_dir = normalize(camera.center - camera.pos);
			camera.pos = dt * (float)dx*view_dir + camera.pos;
		}
	}
	else {
		glfwGetCursorPos(window, &mouse_state.xpos, &mouse_state.ypos);
		mouse_state.xpos_old = mouse_state.xpos;
		mouse_state.ypos_old = mouse_state.ypos;
	}
	camera.center = glm::vec3(0);
}

int main() {
	init_window();
	init_platform(&p);

	char * device_extensions[] = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	};
	uint32_t device_extension_count = sizeof(device_extensions) / sizeof(*device_extensions);

	char * instance_extensions[] = {
		VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME // TODO: remove this. Just here so the program doesn't crash. Mistake on my end...
#ifdef _DEBUG
		,VK_EXT_DEBUG_UTILS_EXTENSION_NAME
#endif
	};
	uint32_t instance_extension_count = sizeof(instance_extensions) / sizeof(*instance_extensions);

	VkalInfo * vkal_info =  init_vulkan(
		window,
		device_extensions, device_extension_count,
		instance_extensions, instance_extension_count);

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
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			1,
			VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT,
			0
		},
		{
			2,
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			1,
			VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
			0
		},
		{
			3,
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			1,
			VK_SHADER_STAGE_VERTEX_BIT,
			0
		}
	};

	VkDescriptorSetLayout descriptor_set_layout_1 = vkal_create_descriptor_set_layout(set_layout, 4);

	VkDescriptorSetLayout layouts[] = {
		descriptor_set_layout_1
	};
	uint32_t descriptor_set_layout_count = sizeof(layouts)/sizeof(*layouts);

	VkDescriptorSet * descriptor_set_1 = (VkDescriptorSet*)malloc(descriptor_set_layout_count * sizeof(VkDescriptorSet));
	vkal_allocate_descriptor_sets(vkal_info->descriptor_pool, layouts, 1, &descriptor_set_1);

	// Shaders
	uint8_t * vertex_byte_code = 0;
	int vertex_code_size;
	p.rfb("shaders\\vert.spv", &vertex_byte_code, &vertex_code_size);
	uint8_t * fragment_byte_code = 0;
	int fragment_code_size;
	p.rfb("shaders\\frag.spv", &fragment_byte_code, &fragment_code_size);
	ShaderStageSetup shader_setup = vkal_create_shaders(vertex_byte_code, vertex_code_size, fragment_byte_code, fragment_code_size);

	VkPipelineLayout pipeline_layout = vkal_create_pipeline_layout(layouts, descriptor_set_layout_count, NULL, 0);

	VkPipeline pipeline_filled_polys = vkal_create_graphics_pipeline(
		shader_setup, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL, VK_CULL_MODE_BACK_BIT, VK_POLYGON_MODE_FILL, 
		VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		VK_FRONT_FACE_CLOCKWISE,
		vkal_info->render_pass, pipeline_layout);

	VkPipeline pipeline_lines = vkal_create_graphics_pipeline(
		shader_setup, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL, VK_CULL_MODE_NONE, VK_POLYGON_MODE_FILL, 
		VK_PRIMITIVE_TOPOLOGY_LINE_LIST, 
		VK_FRONT_FACE_CLOCKWISE,
		vkal_info->render_pass, pipeline_layout);

	// Uniform Buffers
	UniformBuffer uniform_buffer_handle = vkal_create_uniform_buffer(sizeof(ViewProjectionUBO), 0);
	UniformBuffer uniform_buffer_handle2 = vkal_create_uniform_buffer(sizeof(SettingsUBO), 1);
	UniformBuffer uniform_buffer_material = vkal_create_uniform_buffer(sizeof(MaterialUBO), 2);
	UniformBuffer uniform_buffer_model = vkal_create_uniform_buffer(sizeof(ModelUBO), 3);

	vkal_update_descriptor_set_uniform(descriptor_set_1[0], uniform_buffer_handle);
	vkal_update_descriptor_set_uniform(descriptor_set_1[0], uniform_buffer_handle2);
	vkal_update_descriptor_set_uniform(descriptor_set_1[0], uniform_buffer_material);
	vkal_update_descriptor_set_uniform(descriptor_set_1[0], uniform_buffer_model);

	// Initialize Uniform Buffer
	settings_data.show_normals = 0;
	settings_data.light_color = glm::vec4(1, 1, 1, 1);
	settings_data.light_pos = glm::vec4(1, 1, 1, 1);
	vkal_update_uniform(&uniform_buffer_handle, &view_projection_data);
	vkal_update_uniform(&uniform_buffer_handle2, &settings_data);

	// Init Camera
	camera.pos = glm::vec3(0, 0, 5);
	camera.center = glm::vec3(0, 0, 0);
	camera.up = glm::vec3(0, -1, 0);

	// Model and Projection Matrices
	glm::mat4 model = glm::translate(glm::mat4(1), glm::vec3(0, 0, 0));
	glm::mat4 projection = glm::perspective(glm::radians(45.f), 1.f, 0.1f, 1000.0f);

	// Build cone geometry
	setup_geometry();
	// User Interface 
	init_imgui(vkal_info);

	double timer_frequency = glfwGetTimerFrequency();
	double timestep = 1.f / timer_frequency; // step in second
	double title_update_timer = 0;
	double dt = 0;

	int width = VKAL_SCREEN_WIDTH;
	int height = VKAL_SCREEN_HEIGHT;
	// MAIN LOOP
	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
	float old_angle_threshold = angle_threshold;
	while (!glfwWindowShouldClose(window)) {
		double start_time = glfwGetTime();

		glfwPollEvents();

		// Start the Dear ImGui frame
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		
		ImGui::NewFrame();

		ImGui::Begin("Versuch 3 - Normalenvektoren");
		ImGui::SliderFloat("Threshold", &angle_threshold, 0, 1);
		ImGui::Checkbox("Draw Normals", &draw_normals);
		ImGui::ColorEdit3("Light Color", (float*)&settings_data.light_color);
		ImGui::ColorEdit3("Material Emissive", (float*)&g_model.material.emissive);
		ImGui::ColorEdit3("Material Ambient", (float*)&g_model.material.ambient);
		ImGui::ColorEdit3("Material Diffuse", (float*)&g_model.material.diffuse);
		ImGui::ColorEdit3("Material Specular", (float*)&g_model.material.specular);
		ImGui::SliderFloat("Shininess", &g_model.material.specular.w, 0, 256);

		/* Update Material and Model UBOs */
		material_data.emissive = g_model.material.emissive;
		material_data.ambient = g_model.material.ambient;
		material_data.diffuse = g_model.material.diffuse;
		material_data.specular = g_model.material.specular;
		vkal_update_uniform(&uniform_buffer_material, &material_data);
		model_data.model_mat = g_model.model_matrix;
		vkal_update_uniform(&uniform_buffer_model, &model_data);

		ImGui::End();
		ImGui::Render();

		update(float(dt*1000.f));

		// Need new dimensions?
		if (framebuffer_resized) {
			framebuffer_resized = 0;
			glfwGetFramebufferSize(window, &width, &height);
		}

		// Mouse update
		if (!ImGui::IsMouseHoveringAnyWindow() && !ImGui::IsAnyItemActive()) {
			update_camera(dt);
		}

		// Angle Threshold has changed -> update Normal-Buffer
		if (old_angle_threshold != angle_threshold) {
			update_geometry();
			vkal_vertex_buffer_update(g_model_normals.vertices, g_model_normals.vertex_count, g_model_normals.offset);
			vkal_vertex_buffer_update(g_model.vertices, g_model.vertex_count, g_model.offset);
			old_angle_threshold = angle_threshold;
		}

		view_projection_data.projection = glm::perspective(glm::radians(45.f), float(width) / float(height), 0.1f, 1000.f);
		view_projection_data.view = glm::lookAt(camera.pos, camera.center, camera.up);
		vkal_update_uniform(&uniform_buffer_handle, &view_projection_data);
		vkal_update_uniform(&uniform_buffer_handle2, &settings_data);

		uint32_t image_id = vkal_get_image();
		if (image_id >= 0) {

			vkal_begin(image_id, vkal_info->command_buffers[image_id], vkal_info->render_pass);
			vkal_bind_descriptor_set(image_id, 0, &descriptor_set_1[0], 1, pipeline_layout);
	
			// Draw Model(s)
			vkal_record_models(image_id, pipeline_filled_polys, pipeline_layout, NULL, 0, &g_model, 1);
			if (draw_normals) {
				vkal_record_models(image_id, pipeline_lines, pipeline_layout, NULL, 0, &g_model_normals, 1);
			}

			// Record Imgui Draw Data and draw funcs into command buffer
			ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), vkal_info->command_buffers[image_id]);

			vkal_end(vkal_info->command_buffers[image_id]);

			
			VkCommandBuffer command_buffers[] = { vkal_info->command_buffers[image_id] };
			vkal_queue_submit(command_buffers, 1);
			vkal_present(image_id);
		}

		double end_time = glfwGetTime();
		dt = end_time - start_time;
		title_update_timer += dt;

		if ((title_update_timer) > .5f) {
			char window_title[256];
			sprintf(window_title, "frametime: %fms (%f FPS)", (dt * 1000.f), (1000.f)/(dt*1000.f));
			glfwSetWindowTitle(window, window_title);
			title_update_timer = 0;
		}
	}

	ImGui_ImplVulkan_Shutdown();
	vkal_cleanup();

	return 0;
}

