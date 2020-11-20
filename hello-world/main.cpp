#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include <modelloader/Model.h>

#include "platform.h"
#include "vkal.h"
#include "vkal_imgui.h"
#include "model.h"
#include "camera.h"

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

struct ViewProjection
{
	glm::mat4 view;
	glm::mat4 projection;
};

struct ModelUniform
{
	float scale;
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
static ViewProjection view_projection_data;
static ModelUniform   model_uniform;
static uint32_t framebuffer_resized;
static MouseState mouse_state;
static bool update_pipeline;
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

void update_camera(float dt)
{
	if (mouse_state.left_button_down || mouse_state.right_button_down) {
		mouse_state.xpos_old = mouse_state.xpos;
		mouse_state.ypos_old = mouse_state.ypos;
		glfwGetCursorPos(window, &mouse_state.xpos, &mouse_state.ypos);
		double dx = mouse_state.xpos - mouse_state.xpos_old;
		double dy = mouse_state.ypos - mouse_state.ypos_old;
		if (mouse_state.left_button_down) {
			rotate_camera(&camera, dt, (float)dx, (float)dy);
		}
		if (mouse_state.right_button_down) {
			dolly_camera(&camera, dt, dx);
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
		VK_KHR_MAINTENANCE3_EXTENSION_NAME,
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
	VkalInfo * vkal_info =  init_vulkan(
		window, 
		device_extensions, device_extension_count,
		instance_extensions, instance_extension_count);
	init_imgui(vkal_info, window);

	/* Configure Descriptor Sets. We will use only one set that includes two Bindings:
	   Binding 0: Uniform Buffer for Vertex Shader. Contains View-Projection Matrices.
	   Binding 1: Uniform for sampler2D: Array of Textures
	   Binding 2: Uniform for controlling the model's scale
	*/
	VkDescriptorSetLayoutBinding set_layout[] = {
		{
			0, // binding id ( matches with shader )
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			1, // number of resources with this layout binding
			VK_SHADER_STAGE_VERTEX_BIT,
			0
		},
		{
			1,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			1, /* Texture Array-Count: How many Textures do we need? */
			VK_SHADER_STAGE_FRAGMENT_BIT,
			0
		},
		{
			2, // binding id ( matches with shader )
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			1, // number of resources with this layout binding
			VK_SHADER_STAGE_VERTEX_BIT,
			0
		}
	};

	VkDescriptorSetLayout descriptor_set_layout_1 = vkal_create_descriptor_set_layout(set_layout, 3);

	VkDescriptorSetLayout layouts[] = {
		descriptor_set_layout_1
	};
	uint32_t descriptor_set_layout_count = sizeof(layouts)/sizeof(*layouts);

	VkDescriptorSet * descriptor_set_1 = (VkDescriptorSet*)malloc(sizeof(VkDescriptorSet));
	vkal_allocate_descriptor_sets(vkal_info->descriptor_pool, layouts, 1, &descriptor_set_1);

	/* Shader Setup */
	uint8_t * vertex_byte_code = 0;
	int vertex_code_size;
	p.rfb("shaders\\vert.spv", &vertex_byte_code, &vertex_code_size);
	uint8_t * fragment_byte_code = 0;
	int fragment_code_size;
	p.rfb("shaders\\frag.spv", &fragment_byte_code, &fragment_code_size);
	ShaderStageSetup shader_setup = vkal_create_shaders(
		vertex_byte_code, vertex_code_size, 
		fragment_byte_code, fragment_code_size);

	/* We use Push Constants to upload Model specific data light Model-Matrix and Material Data */
	VkPushConstantRange push_constant_ranges[] =
	{
		{ // Model Matrix
			VK_SHADER_STAGE_VERTEX_BIT,
			0, 
			sizeof(glm::mat4)
		},
		{ // Material Properties
			VK_SHADER_STAGE_FRAGMENT_BIT,
			sizeof(glm::mat4),
			sizeof(Material)
		}
	};
	uint32_t push_constant_range_count = sizeof(push_constant_ranges) / sizeof(*push_constant_ranges);

	/* One Pipeline layout: Define blueprint of Descriptor Sets to use and Push Constants */
	VkPipelineLayout pipeline_layout = vkal_create_pipeline_layout(
		layouts, descriptor_set_layout_count, 
		push_constant_ranges, push_constant_range_count);

	/* Graphics Pipeline */
	VkPipeline graphics_pipeline = vkal_create_graphics_pipeline(
		shader_setup, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL, VK_CULL_MODE_BACK_BIT, VK_POLYGON_MODE_FILL, 
		VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		VK_FRONT_FACE_COUNTER_CLOCKWISE,
		vkal_info->render_pass, pipeline_layout);

	/* Create Uniform Buffers for light settings and view/projection matrices */
	UniformBuffer view_proj_ubo = vkal_create_uniform_buffer(sizeof(ViewProjection), 0);
	UniformBuffer model_ubo = vkal_create_uniform_buffer(sizeof(ModelUniform), 2);

	/* Add them to the Descriptor Set */
	vkal_update_descriptor_set_uniform(descriptor_set_1[0], view_proj_ubo);
	vkal_update_descriptor_set_uniform(descriptor_set_1[0], model_ubo);

	/* Initialize Camera */
	camera.pos = glm::vec3(0, 0, 30);
	camera.center = glm::vec3(0, 0, 0);
	camera.up = glm::vec3(0, -1, 0);

	/* Initialize Model UBO */
	model_uniform.scale = 0.1f;
	
	/* Model creation and Texture assignment */
	Model model_1 = create_model_from_file("../assets/obj/pknight.obj");
	model_1.offset = vkal_vertex_buffer_add(model_1.vertices, model_1.vertex_count);
	model_1.pos = glm::vec3(0);
	model_1.model_matrix = glm::mat4(1);
	glm::mat4 model_1_scale = glm::scale(glm::mat4(1), glm::vec3(1.f));
	model_1.model_matrix = model_1_scale * model_1.model_matrix;

	Image image = load_image_file("../assets/md2/pknight/knight.png");
	Texture texture = vkal_create_texture(1, image.data, image.width, image.height, image.channels, 0,
		VK_IMAGE_VIEW_TYPE_2D, 0, 1, 0, 1, VK_FILTER_LINEAR, VK_FILTER_LINEAR);
	free(image.data);
	assign_texture_to_model(&model_1, texture, 0, TEXTURE_TYPE_DIFFUSE); 
	vkal_update_descriptor_set_texturearray(
		descriptor_set_1[0], 
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 
		texture.binding, 0, 
		model_1.texture);

	/* Basic Timer Stuff */
	double timer_frequency = glfwGetTimerFrequency();
	double timestep = 1.f / timer_frequency; // step in second
	double title_update_timer = 0;
	double dt = 0;

	int width = VKAL_SCREEN_WIDTH;
	int height = VKAL_SCREEN_HEIGHT;

	// MAIN LOOP
	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
	bool is_running = true;
	bool is_open = true;
	while (!glfwWindowShouldClose(window) && is_running) {
		double start_time = glfwGetTime();

		glfwPollEvents();

		// Start the Dear ImGui frame
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		ImGui::Begin("My Window", &is_open, ImGuiWindowFlags_MenuBar);
		if(ImGui::BeginMenuBar())
		{
			if (ImGui::BeginMenu("Menu"))
			{
				ImGui::MenuItem("Quit", NULL, &is_running);
				ImGui::EndMenu();
			}
			ImGui::EndMenuBar();
		}
		ImGui::SliderFloat("Scale", &model_uniform.scale, 0.01f, 10.0f);
		ImGui::End();

		ImGui::Render(); 

		// Need new dimensions?
		if (framebuffer_resized) {
			framebuffer_resized = 0;
			glfwGetFramebufferSize(window, &width, &height);
		}

		// Mouse update
		if (!ImGui::IsMouseHoveringAnyWindow() && !ImGui::IsAnyItemActive()) {
			update_camera(dt);
		}
		camera.center = model_1.pos;

		view_projection_data.projection = glm::perspective(glm::radians(45.f), float(width) / float(height), 0.1f, 1000.f);
		view_projection_data.view = glm::lookAt(camera.pos, camera.center, camera.up);
		vkal_update_uniform(&view_proj_ubo, &view_projection_data);
		vkal_update_uniform(&model_ubo, &model_uniform);

		{
			uint32_t image_id = vkal_get_image();

			vkal_begin_command_buffer(vkal_info->command_buffers[image_id]);
			vkal_begin_render_pass(image_id, vkal_info->command_buffers[image_id], vkal_info->render_pass);
			vkal_bind_descriptor_set(image_id, 0, &descriptor_set_1[0], 1, pipeline_layout);
			vkal_record_models_pc(image_id, graphics_pipeline, pipeline_layout, &model_1, 1);
			ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), vkal_info->command_buffers[image_id]);
			vkal_end_renderpass(vkal_info->command_buffers[image_id]);
			vkal_end_command_buffer(vkal_info->command_buffers[image_id]);
			
			VkCommandBuffer command_buffers[] = { vkal_info->command_buffers[image_id] };
			vkal_queue_submit(command_buffers, 1);

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
