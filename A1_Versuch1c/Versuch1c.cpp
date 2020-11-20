//#ifndef _CRT_SECURE_NO_WARNINGS
//#define _CRT_SECURE_NO_WARNINGS
//#endif
//
//#define WIN32_LEAN_AND_MEAN
//#include <Windows.h>
//#undef min
//#undef max
//
//#define GLM_FORCE_RADIANS
//#define GLM_FORCE_DEPTH_ZERO_TO_ONE
//#include <glm/glm.hpp>
//#include <glm/ext.hpp>
//
//#include <imgui\imgui.h>
//#include <imgui\imgui_impl_glfw.h>
//#include <imgui\imgui_impl_vulkan.h>
//
//#include <assimp/cimport.h>        // Plain-C interface
//#include <assimp/scene.h>          // Output data structure
//#include <assimp/postprocess.h>    // Post processing flags
//
//#define STB_IMAGE_IMPLEMENTATION
//#include "stb_image.h"
//
//#include "platform.h"
//#include "vkal.h"
//
//#define massert(statement) {\
//if (!statement) { \
//		printf("Assertion failed! LINE: %d, FILE: %s\n", __LINE__, __FILE__); \
//		printf("Press any key to continue...\n");\
//		getchar();\
//		exit(-1);\
//}\
//}
//
//// Definition der Kreiszahl 
//#define GL_PI 3.1415f
//
//struct MyUBO
//{
//	glm::vec4 show_wireframe;
//	glm::vec4 light_color;
//	glm::mat4 view;
//	glm::mat4 projection;
//	float time;
//	float delta_time;
//};
//
//struct SkyboxUBO
//{
//	glm::mat4 view;
//	glm::mat4 projection;
//};
//
//struct MouseState
//{
//	double xpos, ypos;
//	double xpos_old, ypos_old;
//	uint32_t left_button_down;
//	uint32_t right_button_down;
//};
//
///* GLOBALS */
//static Platform p;
//static Camera camera;
//static int keys[GLFW_KEY_LAST];
//static MyUBO uniform_buffer;
//static uint32_t framebuffer_resized;
//static MouseState mouse_state;
//static uint32_t cull_mode_front;
//static uint32_t cull_mode_back;
//static bool backface_wireframe_enable;
//static bool depth_buffer_enable = true;
//static bool cull_enable = false;
//static bool update_pipeline;
//VkCullModeFlagBits cull_mode = VK_CULL_MODE_BACK_BIT;
//VkPrimitiveTopology primitive_topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
//VkFrontFace front_face = VK_FRONT_FACE_CLOCKWISE;
//static GLFWwindow * window;
//static Model konus;
//static Model boden;
//
//#define SPEED 0.01f
//static void update(float dt)
//{
//	if (keys[GLFW_KEY_W]) {
//		glm::vec3 direction = glm::normalize(camera.center - camera.pos);
//		camera.pos += SPEED*direction*dt;
//		camera.center += SPEED * direction * dt;
//	}
//	if (keys[GLFW_KEY_S]) {
//		glm::vec3 direction = glm::normalize(camera.center - camera.pos);
//		camera.pos -= SPEED*direction*dt;
//		camera.center -= SPEED * direction * dt;
//	}
//	if (keys[GLFW_KEY_A]) {
//		glm::vec3 direction = glm::normalize(camera.center - camera.pos);
//		glm::vec3 side = glm::normalize(glm::cross(direction, camera.up));
//		camera.pos -= SPEED*side * dt;
//		camera.center -= SPEED*side*dt;
//	}
//	if (keys[GLFW_KEY_D]) {
//		glm::vec3 direction = glm::normalize(camera.center - camera.pos);
//		glm::vec3 side = glm::normalize(glm::cross(direction, camera.up));
//		camera.pos += SPEED*side*dt;
//		camera.center += SPEED*side*dt;
//	}
//}
//
//float rand_between(float min, float max)
//{
//	float range = max - min;
//	float step = range / RAND_MAX;
//	return (step * rand()) + min;
//}
//
//// GLFW callbacks
//static void glfw_key_callback(GLFWwindow * window, int key, int scancode, int action, int mods)
//{
//	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
//		printf("escape key pressed\n");
//		glfwSetWindowShouldClose(window, GLFW_TRUE);
//	}
//	if (action == GLFW_PRESS) {
//		if (key == GLFW_KEY_W) {
//			keys[GLFW_KEY_W] = 1;
//		}
//		if (key == GLFW_KEY_S) {
//			keys[GLFW_KEY_S] = 1;
//		}
//		if (key == GLFW_KEY_A) {
//			keys[GLFW_KEY_A] = 1;
//		}
//		if (key == GLFW_KEY_D) {
//			keys[GLFW_KEY_D] = 1;
//		}
//		if (key == GLFW_KEY_LEFT_ALT) {
//			keys[GLFW_KEY_LEFT_ALT] = 1;
//		}
//		if (key == GLFW_KEY_1) {
//			uniform_buffer.show_wireframe.x = !uniform_buffer.show_wireframe.x;
//		}
//		if (key == GLFW_KEY_2) {
//			depth_buffer_enable = !depth_buffer_enable;
//			update_pipeline = true;
//		}
//		if (key == GLFW_KEY_4) {
//			backface_wireframe_enable = !backface_wireframe_enable;
//		}
//	}
//	else if (action == GLFW_RELEASE) {
//		if (key == GLFW_KEY_W) {
//			keys[GLFW_KEY_W] = 0;
//		}
//		if (key == GLFW_KEY_S) {
//			keys[GLFW_KEY_S] = 0;
//		}
//		if (key == GLFW_KEY_A) {
//			keys[GLFW_KEY_A] = 0;
//		}
//		if (key == GLFW_KEY_D) {
//			keys[GLFW_KEY_D] = 0;
//		}
//		if (key == GLFW_KEY_LEFT_ALT) {
//			keys[GLFW_KEY_LEFT_ALT] = 0;
//		}
//	}
//}
//
//static void glfw_framebuffer_size_callback(GLFWwindow * window, int width, int height)
//{
//	framebuffer_resized = 1;
//	printf("window was resized\n");
//}
//
//static void glfw_mouse_button_callback(GLFWwindow * window, int mouse_button, int action, int mods)
//{
//	if (mouse_button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
//		mouse_state.left_button_down = 1;
//	}
//	if (mouse_button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
//		mouse_state.left_button_down = 0;
//	}
//	if (mouse_button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
//		mouse_state.right_button_down = 1;
//	}
//	if (mouse_button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE) {
//		mouse_state.right_button_down = 0;
//	}
//}
//
//void init_window() {
//	glfwInit();
//	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
//	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
//	window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", 0, 0);
//	glfwSetKeyCallback(window, glfw_key_callback);
//	glfwSetMouseButtonCallback(window, glfw_mouse_button_callback);
//	glfwSetFramebufferSizeCallback(window, glfw_framebuffer_size_callback);
//}
//
//struct Image
//{
//	int width, height, channels;
//	unsigned char * data;
//};
//
//Image load_image(const char * file)
//{
//	Image image;
//	image.data = stbi_load(file, &image.width, &image.height, &image.channels, 4);
//	massert(image.data);
//	return image;
//}
//
//void free_image(Image image)
//{
//	massert(image.data)
//		stbi_image_free(image.data);
//}
//
//void check_vk_result(VkResult err)
//{
//	if (err == 0) return;
//	printf("VkResult %d\n", err);
//	if (err < 0)
//		abort();
//}
//
//void init_imgui(VkalInfo * vkal_info)
//{
//	IMGUI_CHECKVERSION();
//	ImGui::CreateContext();
//
//	ImGui::StyleColorsDark();
//	QueueFamilyIndicies indicies = find_queue_families(vkal_info->physical_device, vkal_info->surface);
//
//	ImGui_ImplVulkan_InitInfo initInfo = {};
//	initInfo.Instance = vkal_info->instance;
//	initInfo.PhysicalDevice = vkal_info->physical_device;
//	initInfo.Device = vkal_info->device;
//	initInfo.QueueFamily = indicies.graphics_family;
//	initInfo.Queue = vkal_info->graphics_queue;
//	initInfo.DescriptorPool = vkal_info->descriptor_pool;
//	initInfo.CheckVkResultFn = check_vk_result;
//
//	ImGui_ImplGlfw_InitForVulkan(window, false);
//	ImGui_ImplVulkan_Init(&initInfo, vkal_info->imgui_render_pass); // ToDo: RenderPass
//
//	{
//		VkCommandBufferAllocateInfo allocInfo = {};
//		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
//		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
//		allocInfo.commandPool = vkal_info->command_pools[0];
//		allocInfo.commandBufferCount = 1;
//
//		VkCommandBuffer command_buffer;
//		vkAllocateCommandBuffers(vkal_info->device, &allocInfo, &command_buffer);
//
//		VkCommandBufferBeginInfo begin_info = {};
//		begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
//		begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
//		VkResult err = vkBeginCommandBuffer(command_buffer, &begin_info);
//		check_vk_result(err);
//
//		ImGui_ImplVulkan_CreateFontsTexture(command_buffer);
//
//		VkSubmitInfo end_info = {};
//		end_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
//		end_info.commandBufferCount = 1;
//		end_info.pCommandBuffers = &command_buffer;
//		err = vkEndCommandBuffer(command_buffer);
//		check_vk_result(err);
//		err = vkQueueSubmit(vkal_info->graphics_queue, 1, &end_info, VK_NULL_HANDLE);
//		check_vk_result(err);
//
//		err = vkDeviceWaitIdle(vkal_info->device);
//		check_vk_result(err);
//		ImGui_ImplVulkan_InvalidateFontUploadObjects();
//
//		vkFreeCommandBuffers(vkal_info->device, vkal_info->command_pools[0], 1, &command_buffer);
//	}
//}
//
//void rotate_camera(Camera * camera, float up_angle, float right_angle)
//{
//	glm::vec3 camera_dir = glm::normalize(camera->center - camera->pos);
//	camera->right = glm::normalize(glm::cross(camera->up, camera_dir));
//	camera->up = glm::vec3(0, -1, 0); // glm::normalize(glm::cross(oriented_camera.right, camera_dir));
//
//	glm::mat3 rotate_up = glm::rotate(glm::mat4(1.f), glm::radians(up_angle), camera->up);
//	glm::mat3 rotate_right = glm::rotate(glm::mat4(1.f), glm::radians(right_angle), camera->right);
//	camera->pos = rotate_up * rotate_right * camera->pos;
//}
//
//void setup_cone()
//{
//	//Indices definieren
//	uint16_t indices[] = {
//		0, 1, 2,
//		0, 2, 3,
//		0, 3, 4,
//		0, 4, 5,
//		0, 5, 6,
//		0, 6, 7,
//		0, 7, 8,
//		0, 8, 9,
//		0, 9, 10,
//		0, 10,11,
//		0, 11,12,
//		0, 12,13,
//		0, 13,14,
//		0, 14,15,
//		0, 15,16,
//		0, 16,17
//	};
//	uint32_t index_count = sizeof(indices) / sizeof(*indices);
//
//	//18 Vertices anlegen
//	Vertex konusVertices[18];
//	// Die Spitze des Konus ist ein Vertex, den alle Triangles gemeinsam haben;
//	// um einen Konus anstatt einen Kreis zu produzieren muss der Vertex einen positiven z-Wert haben
//	konusVertices[0].pos = glm::vec3(0, 75, 0);
//	konusVertices[0].color = glm::vec4(0, 0.6, 1, 1);
//	// Kreise um den Mittelpunkt und spezifiziere Vertices entlang des Kreises
//	// um einen Triangle_Fan zu erzeugen
//	int iPivot = 1;
//	int i = 1;
//	for (float angle = 0.0f; angle < (2.0f*GL_PI); angle += (GL_PI / 8.0f))
//	{
//		// Berechne x und y Positionen des naechsten Vertex
//		float x = 50.0f*sin(angle);
//		float z = 50.0f*cos(angle);
//
//		// Alterniere die Farbe
//		if ((iPivot % 2) == 0)
//			konusVertices[i].color = glm::vec4(0.235, 0.235, 0.235, 1);
//		else
//			konusVertices[i].color = glm::vec4(0, 0.6, 1, 1);
//
//		// Inkrementiere iPivot um die Farbe beim naechsten mal zu wechseln
//		iPivot++;
//
//		// Spezifiziere den naechsten Vertex des Triangle_Fans
//		konusVertices[i].pos = glm::vec3(x, 0, z);
//		i++;
//	}
//	konus.vertices = konusVertices;
//	konus.vertex_count = 18;
//	konus.offset = vkal_vertex_buffer_add(konus.vertices, konus.vertex_count);
//	konus.indices = indices;
//	konus.index_count = index_count;
//	konus.index_buffer_offset = vkal_index_buffer_add(konus.indices, konus.index_count);
//	konus.pos = glm::vec3(0, 0, 0);
//	glm::mat4 translate = glm::translate(glm::mat4(1.f), konus.pos);
//	glm::mat4 scale = glm::scale(glm::mat4(1), glm::vec3(0.02, 0.02, 0.02));
//	glm::mat4 rot_x = glm::rotate(glm::mat4(1), glm::radians(2 * GL_PI), glm::vec3(1, 0, 0));
//	konus.model_matrix = scale * translate;
//
//	// Erzeuge einen weiteren Triangle_Fan um den Boden zu bedecken
//	Vertex bodenVertices[18];
//	// Das Zentrum des Triangle_Fans ist im Ursprung
//	bodenVertices[0].pos = glm::vec3(0, 0, 0);
//	bodenVertices[0].color = glm::vec4(0, 0.8, 0, 1);
//	i = 1;
//	for (float angle = 0.0f; angle < (2.0f*GL_PI); angle += (GL_PI / 8.0f))
//	{
//		// Berechne x und y Positionen des naechsten Vertex
//		float x = 50.0f*sin(angle);
//		float z = 50.0f*cos(angle);
//
//		// Alterniere die Farbe
//		if ((iPivot % 2) == 0)
//			bodenVertices[i].color = glm::vec4(1, 0.8, 0.2, 1);
//		else
//			bodenVertices[i].color = glm::vec4(0, 0.8, 0, 1);
//
//		// Inkrementiere iPivot um die Farbe beim naechsten mal zu wechseln
//		iPivot++;
//
//		// Spezifiziere den naechsten Vertex des Triangle_Fans
//		bodenVertices[i].pos = glm::vec3(x, 0, z);
//		i++;
//	}
//	boden.vertices = bodenVertices;
//	boden.vertex_count = 18;
//	boden.offset = vkal_vertex_buffer_add(boden.vertices, boden.vertex_count);
//	boden.indices = indices;
//	boden.index_count = index_count;
//	boden.index_buffer_offset = vkal_index_buffer_add(boden.indices, boden.index_count);
//	boden.pos = glm::vec3(0, 0, 0);
//	translate = glm::translate(glm::mat4(1.f), boden.pos);
//	boden.model_matrix = scale * translate;
//}
//
//void update_camera(float dt)
//{
//	if (mouse_state.left_button_down || mouse_state.right_button_down) {
//		mouse_state.xpos_old = mouse_state.xpos;
//		mouse_state.ypos_old = mouse_state.ypos;
//		glfwGetCursorPos(window, &mouse_state.xpos, &mouse_state.ypos);
//		double dx = mouse_state.xpos - mouse_state.xpos_old;
//		double dy = mouse_state.ypos - mouse_state.ypos_old;
//		if (mouse_state.left_button_down) {
//			rotate_camera(&camera, (float)dx, (float)dy);
//		}
//		if (mouse_state.right_button_down) {
//			glm::vec3 view_dir = normalize(camera.center - camera.pos);
//			camera.pos = dt * (float)dx*view_dir + camera.pos;
//		}
//	}
//	else {
//		glfwGetCursorPos(window, &mouse_state.xpos, &mouse_state.ypos);
//		mouse_state.xpos_old = mouse_state.xpos;
//		mouse_state.ypos_old = mouse_state.ypos;
//	}
//	camera.center = konus.pos;
//}
//
//int main() {
//	init_window();
//	init_platform(&p);
//	VkalInfo * vkal_info =  init_vulkan(window);
//
//	/* Configure the Vulkan Pipeline and Uniform Buffers*/
//	// Descriptor Sets
//	VkDescriptorSetLayoutBinding set_layout[] = {
//		{
//			0, // binding id ( matches with shader )
//			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
//			1, // number of resources with this layout binding
//			VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT,
//			0
//		}
//	};
//	DescriptorSetLayout descriptor_set_layout = vkal_create_descriptor_set_layout(*set_layout);
//	uint32_t descriptor_set_layout_count = 1;
//
//	// Shaders
//	char * vertex_byte_code = 0;
//	int vertex_code_size;
//	p.rfb("shaders\\vert.spv", &vertex_byte_code, &vertex_code_size);
//	char * fragment_byte_code = 0;
//	int fragment_code_size;
//	p.rfb("shaders\\frag.spv", &fragment_byte_code, &fragment_code_size);
//	ShaderStageSetup shader_setup = vkal_create_shaders(vertex_byte_code, vertex_code_size, fragment_byte_code, fragment_code_size);
//	VkPipeline graphics_pipeline_depth_disable_cull_disable = vkal_create_graphics_pipeline(&descriptor_set_layout.layout, descriptor_set_layout_count, 
//		shader_setup, VK_FALSE, VK_COMPARE_OP_LESS_OR_EQUAL, VK_CULL_MODE_NONE, VK_POLYGON_MODE_FILL, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN, VK_FRONT_FACE_CLOCKWISE);
//	VkPipeline graphics_pipeline_depth_disable_cull_enable = vkal_create_graphics_pipeline(&descriptor_set_layout.layout, descriptor_set_layout_count,
//		shader_setup, VK_FALSE, VK_COMPARE_OP_LESS_OR_EQUAL, VK_CULL_MODE_BACK_BIT, VK_POLYGON_MODE_FILL, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN, VK_FRONT_FACE_CLOCKWISE);
//	VkPipeline graphics_pipeline_depth_enable_cull_disable = vkal_create_graphics_pipeline(&descriptor_set_layout.layout, descriptor_set_layout_count,
//		shader_setup, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL, VK_CULL_MODE_NONE, VK_POLYGON_MODE_FILL, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN, VK_FRONT_FACE_CLOCKWISE);
//	VkPipeline graphics_pipeline_depth_enable_cull_enable = vkal_create_graphics_pipeline(&descriptor_set_layout.layout, descriptor_set_layout_count,
//		shader_setup, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL, VK_CULL_MODE_BACK_BIT, VK_POLYGON_MODE_FILL, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN, VK_FRONT_FACE_CLOCKWISE);
//
//	VkPipeline wireframe_depth_disable_cull_disable_pipeline = vkal_create_graphics_pipeline(&descriptor_set_layout.layout, descriptor_set_layout_count,
//		shader_setup, VK_FALSE, VK_COMPARE_OP_LESS_OR_EQUAL, VK_CULL_MODE_NONE, VK_POLYGON_MODE_FILL, VK_PRIMITIVE_TOPOLOGY_LINE_STRIP, VK_FRONT_FACE_COUNTER_CLOCKWISE);
//	VkPipeline wireframe_depth_disable_cull_enable_pipeline = vkal_create_graphics_pipeline(&descriptor_set_layout.layout, descriptor_set_layout_count,
//		shader_setup, VK_FALSE, VK_COMPARE_OP_LESS_OR_EQUAL, VK_CULL_MODE_BACK_BIT, VK_POLYGON_MODE_FILL, VK_PRIMITIVE_TOPOLOGY_LINE_STRIP, VK_FRONT_FACE_CLOCKWISE);
//	VkPipeline wireframe_depth_enable_cull_disable_pipeline = vkal_create_graphics_pipeline(&descriptor_set_layout.layout, descriptor_set_layout_count,
//		shader_setup, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL, VK_CULL_MODE_NONE, VK_POLYGON_MODE_FILL, VK_PRIMITIVE_TOPOLOGY_LINE_STRIP, VK_FRONT_FACE_CLOCKWISE);
//	VkPipeline wireframe_depth_enable_cull_enable_pipeline = vkal_create_graphics_pipeline(&descriptor_set_layout.layout, descriptor_set_layout_count,
//		shader_setup, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL, VK_CULL_MODE_BACK_BIT, VK_POLYGON_MODE_FILL, VK_PRIMITIVE_TOPOLOGY_LINE_STRIP, VK_FRONT_FACE_CLOCKWISE);
//
//	// Uniform Buffers
//	UniformBuffer uniform_buffer_handle = vkal_create_uniform_buffer(descriptor_set_layout, sizeof(MyUBO));
//	// Initialize Uniform Buffer
//	uniform_buffer.show_wireframe.x = 0;
//	update_uniform(&uniform_buffer_handle, &uniform_buffer);
//
//	// Init Camera
//	camera.pos = glm::vec3(0, 0, 5);
//	camera.center = glm::vec3(0, 0, 0);
//	camera.up = glm::vec3(0, -1, 0);
//
//	// Model and Projection Matrices
//	glm::mat4 model = glm::translate(glm::mat4(1), glm::vec3(0, 0, 0));
//	glm::mat4 projection = glm::perspective(glm::radians(45.f), 1.f, 0.1f, 1000.0f);
//
//	// Build cone geometry
//	setup_cone();
//	// User Interface 
//	init_imgui(vkal_info);
//
//	double timer_frequency = glfwGetTimerFrequency();
//	double timestep = 1.f / timer_frequency; // step in second
//	double title_update_timer = 0;
//	double dt = 0;
//
//	int width = WIDTH;
//	int height = HEIGHT;
//	// MAIN LOOP
//	uint32_t depth_test_state = depth_buffer_enable;
//	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
//	while (!glfwWindowShouldClose(window)) {
//		double start_time = glfwGetTime();
//
//		glfwPollEvents();
//
//		// Start the Dear ImGui frame
//		ImGui_ImplVulkan_NewFrame();
//		ImGui_ImplGlfw_NewFrame();
//		ImGui::NewFrame();
//
//		ImGui::Begin("Versuch 1c");                          // Create a window called "Hello, world!" and append into it.
//		ImGui::Checkbox("Enable Depth Buffer", &depth_buffer_enable);             // Display some text (you can use a format strings too)
//		ImGui::Checkbox("Render Backfaces as Wireframe", &backface_wireframe_enable);
//		ImGui::Checkbox("Enable Culling", &cull_enable);
//		ImGui::End();
//		ImGui::Render();
//
//		update(float(dt*1000.f));
//
//		// Need new dimensions?
//		if (framebuffer_resized) {
//			framebuffer_resized = 0;
//			glfwGetFramebufferSize(window, &width, &height);
//		}
//
//		// Mouse update
//		if (!ImGui::IsMouseHoveringAnyWindow() && !ImGui::IsAnyItemActive()) {
//			update_camera(dt);
//		}
//
//		uniform_buffer.projection = glm::perspective(glm::radians(45.f), float(width) / float(height), 0.1f, 1000.f);
//		uniform_buffer.view = glm::lookAt(camera.pos, camera.center, camera.up);
//		uniform_buffer.time += dt;
//		uniform_buffer.delta_time = dt;
//		update_uniform(&uniform_buffer_handle, &uniform_buffer);
//
//		Model models[] = { konus, boden };
//		uint32_t image_id = vkal_get_image();
//		if (image_id >= 0) {
//
//			vkal_begin(image_id, vkal_info->command_buffers[image_id], vkal_info->render_pass);
//
//			if (backface_wireframe_enable) {
//				if (cull_enable) goto CULLENABLE;
//				vkal_record_models_indexed(image_id, wireframe_depth_enable_cull_disable_pipeline, &uniform_buffer_handle, 1, models, 2);
//				vkal_record_models_indexed(image_id, graphics_pipeline_depth_enable_cull_enable, &uniform_buffer_handle, 1, &konus, 1);
//			}
//			else {
//				if (cull_enable) {
//CULLENABLE:			if (depth_buffer_enable) {
//						vkal_record_models_indexed(image_id, graphics_pipeline_depth_enable_cull_enable, &uniform_buffer_handle, 1, models, 2);
//					}
//					else {
//						vkal_record_models_indexed(image_id, graphics_pipeline_depth_disable_cull_enable, &uniform_buffer_handle, 1, models, 2);
//					}
//				}
//				else {
//					if (depth_buffer_enable) {
//						vkal_record_models_indexed(image_id, graphics_pipeline_depth_enable_cull_disable, &uniform_buffer_handle, 1, models, 2);
//					}
//					else {
//						vkal_record_models_indexed(image_id, graphics_pipeline_depth_disable_cull_disable, &uniform_buffer_handle, 1, models, 2);
//					}
//				}
//			}
//
//			// Record Imgui Draw Data and draw funcs into command buffer
//			ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), vkal_info->command_buffers[image_id]);
//
//			vkal_end(vkal_info->command_buffers[image_id]);
//			VkCommandBuffer command_buffers[] = { vkal_info->command_buffers[image_id] };
//			vkal_draw_frame(command_buffers, 1);
//			vkal_present(image_id);
//		}
//
//		double end_time = glfwGetTime();
//		dt = end_time - start_time;
//		title_update_timer += dt;
//
//		if ((title_update_timer) > .5f) {
//			char window_title[256];
//			sprintf(window_title, "frametime: %fms (%f FPS)", (dt * 1000.f), (1000.f)/(dt*1000.f));
//			glfwSetWindowTitle(window, window_title);
//			title_update_timer = 0;
//		}
//	}
//
//	vkal_destroy_graphics_pipeline(graphics_pipeline_depth_disable_cull_disable);
//	vkal_destroy_graphics_pipeline(graphics_pipeline_depth_disable_cull_enable);
//	vkal_destroy_graphics_pipeline(graphics_pipeline_depth_enable_cull_disable);
//	vkal_destroy_graphics_pipeline(graphics_pipeline_depth_enable_cull_enable);
//
//	vkal_destroy_graphics_pipeline(wireframe_depth_disable_cull_disable_pipeline);
//	vkal_destroy_graphics_pipeline(wireframe_depth_disable_cull_enable_pipeline);
//	vkal_destroy_graphics_pipeline(wireframe_depth_enable_cull_disable_pipeline);
//	vkal_destroy_graphics_pipeline(wireframe_depth_enable_cull_enable_pipeline);
//
//	cleanup();
//
//	return 0;
//}
//
