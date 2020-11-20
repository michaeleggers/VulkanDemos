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

#include <assimp/cimport.h>        // Plain-C interface
#include <assimp/scene.h>          // Output data structure
#include <assimp/postprocess.h>    // Post processing flags

#include <imageloader/ImageLoader.h>

#include <imgui\imgui.h>
#include <imgui\imgui_impl_glfw.h>
#include <imgui\imgui_impl_vulkan.h>

//#define STB_IMAGE_IMPLEMENTATION
//#include "stb_image.h"

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

struct ModelUBO
{
	glm::vec4 show_wireframe;
	glm::vec4 light_color;
	glm::mat4 view;
	glm::mat4 projection;
	uint32_t texture_mapping_type; // 0 = object linear, 1 = eye linear, 2 = cubemap
};

struct SkyboxUBO
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

struct Image
{
	int width, height, channels;
	unsigned char * data;
};

/* GLOBALS */
static Platform p;
static Camera camera;
static int keys[GLFW_KEY_LAST];
static ModelUBO uniform_buffer;
static SkyboxUBO uniform_buffer_skybox;
static uint32_t framebuffer_resized;
static MouseState mouse_state;
static uint32_t depth_buffer_enable;
static VkPipeline model_pipeline;
static VkPipeline skybox_pipeline;
static ShaderStageSetup shader_setup;
static ShaderStageSetup shader_setup_skybox;
static uint32_t cull_mode_front;
static uint32_t cull_mode_back;
static uint32_t cull_enable;

static glm::vec3 old_camera_pos;
static GLFWwindow * window;


Model create_model_from_file(char const * file)
{
	Model model = {};
	aiScene const * scene = aiImportFile(file, aiProcess_Triangulate | aiProcess_FlipUVs);
	for (int i = 0; i < scene->mNumMeshes; ++i) {
		model.vertex_count += scene->mMeshes[i]->mNumVertices;
	}
	model.vertices = (Vertex*)malloc(model.vertex_count * sizeof(Vertex));
	//p.push_array(arena, model.vertices, model.vertex_count * sizeof(Vertex));
	for (int i = 0; i < scene->mNumMeshes; ++i) {
		for (int j = 0; j < scene->mMeshes[i]->mNumVertices; j += 3) {
			aiVector3D vertex0 = scene->mMeshes[i]->mVertices[j];
			aiVector3D vertex1 = scene->mMeshes[i]->mVertices[j + 1];
			aiVector3D vertex2 = scene->mMeshes[i]->mVertices[j + 2];
			model.vertices[j].pos = { vertex0.x, vertex0.y, vertex0.z };
			model.vertices[j + 1].pos = { vertex1.x, vertex1.y, vertex1.z };
			model.vertices[j + 2].pos = { vertex2.x, vertex2.y, vertex2.z };
            
			model.vertices[j].barycentric = { 1, 0, 0 };
			model.vertices[j + 1].barycentric = { 0, 1, 0 };
			model.vertices[j + 2].barycentric = { 0, 0, 1 };
            
			if (scene->mMeshes[i]->HasNormals()) {
				aiVector3D normal0 = scene->mMeshes[i]->mNormals[j];
				aiVector3D normal1 = scene->mMeshes[i]->mNormals[j + 1];
				aiVector3D normal2 = scene->mMeshes[i]->mNormals[j + 2];
				model.vertices[j].normal = glm::normalize(glm::vec3( normal0.x, normal0.y, normal0.z ));
				model.vertices[j + 1].normal = glm::normalize(glm::vec3(normal1.x, normal1.y, normal1.z));
				model.vertices[j + 2].normal = glm::normalize(glm::vec3(normal2.x, normal2.y, normal2.z));
			}
			else {
				aiVector3D a = vertex1 - vertex0;
				aiVector3D b = vertex2 - vertex0;
				glm::vec3 normal = glm::cross(normalize(glm::vec3(a.x, a.y, a.z)), normalize(glm::vec3(b.x, b.y, b.z)));
				model.vertices[j].normal = normal;
				model.vertices[j + 1].normal = normal;
				model.vertices[j + 2].normal = normal;
			}
			if (scene->mMeshes[i]->HasTextureCoords(i)) {
				model.vertices[j].uv.x = scene->mMeshes[i]->mTextureCoords[i][j].x;
				model.vertices[j].uv.y = scene->mMeshes[i]->mTextureCoords[i][j].y;
				model.vertices[j + 1].uv.x = scene->mMeshes[i]->mTextureCoords[i][j + 1].x;
				model.vertices[j + 1].uv.y = scene->mMeshes[i]->mTextureCoords[i][j + 1].y;
				model.vertices[j + 2].uv.x = scene->mMeshes[i]->mTextureCoords[i][j + 2].x;
				model.vertices[j + 2].uv.y = scene->mMeshes[i]->mTextureCoords[i][j + 2].y;
			}
			else {
				// No UVs!
			}
		}
	}
    
	return model;
}



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
		if (key == GLFW_KEY_1) {
			uniform_buffer.show_wireframe.x = !uniform_buffer.show_wireframe.x;
		}
		if (key == GLFW_KEY_2) {
			depth_buffer_enable = !depth_buffer_enable;
		}
		if (key == GLFW_KEY_3) {
			cull_enable = !cull_enable;
		}
		if (key == GLFW_KEY_F) {
			cull_mode_front != cull_mode_front;
		}
		if (key == GLFW_KEY_B) {
			cull_mode_back != cull_mode_back;
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
	window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", 0, 0);
	glfwSetKeyCallback(window, glfw_key_callback);
	glfwSetMouseButtonCallback(window, glfw_mouse_button_callback);
	glfwSetFramebufferSizeCallback(window, glfw_framebuffer_size_callback);
}

Image load_image(const char * file)
{
	Image image;
	img::ImageLoader imageloader; // this is fucked up!
	image.data = imageloader.LoadTextureFromFile(file , &image.width, &image.height, false);
	//image.data = stbi_load(file, &image.width, &image.height, &image.channels, 4);
	massert(image.data);
	return image;
}

void free_image(Image image)
{
	massert(image.data)
    //stbi_image_free(image.data);
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
#define CAMERA_DOLLY_SPEED 10.f
			glm::vec3 view_dir = normalize(camera.center - camera.pos);
			camera.pos = CAMERA_DOLLY_SPEED * (float)dt * (float)dx*view_dir + camera.pos;
		}
	}
	else {
		glfwGetCursorPos(window, &mouse_state.xpos, &mouse_state.ypos);
		mouse_state.xpos_old = mouse_state.xpos;
		mouse_state.ypos_old = mouse_state.ypos;
	}
}

Model setup_skybox()
{
	Model cube;
	cube.vertices = (Vertex*)malloc(36 * sizeof(Vertex));
	float cube_pos[] = {
		-10.0f, 10.0f, -10.0f,
		-10.0f, -10.0f, -10.0f,
		10.0f, -10.0f, -10.0f,
		10.0f, -10.0f, -10.0f,
		10.0f, 10.0f, -10.0f,
		-10.0f, 10.0f, -10.0f,
		-10.0f, -10.0f, 10.0f,
		-10.0f, -10.0f, -10.0f,
		-10.0f, 10.0f, -10.0f,
		-10.0f, 10.0f, -10.0f,
		-10.0f, 10.0f, 10.0f,
		-10.0f, -10.0f, 10.0f,
		10.0f, -10.0f, -10.0f,
		10.0f, -10.0f, 10.0f,
		10.0f, 10.0f, 10.0f,
		10.0f, 10.0f, 10.0f,
		10.0f, 10.0f, -10.0f,
		10.0f, -10.0f, -10.0f,
		-10.0f, -10.0f, 10.0f,
		-10.0f, 10.0f, 10.0f,
		10.0f, 10.0f, 10.0f,
		10.0f, 10.0f, 10.0f,
		10.0f, -10.0f, 10.0f,
		-10.0f, -10.0f, 10.0f,
		-10.0f, 10.0f, -10.0f,
		10.0f, 10.0f, -10.0f,
		10.0f, 10.0f, 10.0f,
		10.0f, 10.0f, 10.0f,
		-10.0f, 10.0f, 10.0f,
		-10.0f, 10.0f, -10.0f,
		-10.0f, -10.0f, -10.0f,
		-10.0f, -10.0f, 10.0f,
		10.0f, -10.0f, -10.0f,
		10.0f, -10.0f, -10.0f,
		-10.0f, -10.0f, 10.0f,
		10.0f, -10.0f, 10.0f
	};
	for (int i = 0; i < 36; ++i) {
		cube.vertices[i].pos = glm::vec3(cube_pos[3 * i], cube_pos[3 * i + 1], cube_pos[3 * i + 2]);
	}
	cube.vertex_count = 36;
	return cube;
}

int main() {
    
	//HelloTriangleApplication app = {};
	init_window();
	init_platform(&p);
	VkalInfo * vkal_info = init_vulkan(window);
	init_imgui(vkal_info);
	printf("init_vulkan done\n");
    
	// Descriptor Sets
	VkDescriptorSetLayoutBinding set_layout_global[] = {
		{
			0, // binding id ( matches with shader )
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			1, // number of resources with this layout binding
			VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT,
			0
		}
	};
    
	VkDescriptorSetLayoutBinding set_layout_cubemap[] = { // for reference, not used here
		{
			0, // binding ID
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			1,
			VK_SHADER_STAGE_FRAGMENT_BIT,
			0
		}
	};
	DescriptorSetLayout descriptor_set_layout_global = vkal_create_descriptor_set_layout(*set_layout_global);
	DescriptorSetLayout descriptor_set_layout_cubemap = vkal_create_descriptor_set_layout(*set_layout_cubemap);
	VkDescriptorSetLayout descriptor_set_layouts[] = {
		descriptor_set_layout_global.layout,
		descriptor_set_layout_cubemap.layout
	}; // Order in this array matches the set number in shaders.
	uint32_t descriptor_set_layout_count = sizeof(descriptor_set_layouts) / sizeof(*descriptor_set_layouts);
    
    
    
	// Model shader
	char * vertex_byte_code = 0;
	int vertex_code_size;
	p.rfb("shaders\\vert.spv", &vertex_byte_code, &vertex_code_size);
	char * fragment_byte_code = 0;
	int fragment_code_size;
	p.rfb("shaders\\frag.spv", &fragment_byte_code, &fragment_code_size);
	shader_setup = vkal_create_shaders(vertex_byte_code, vertex_code_size, fragment_byte_code, fragment_code_size);
	
    // Skybox shader
    p.rfb("shaders\\skybox_vert.spv", &vertex_byte_code, &vertex_code_size);
	p.rfb("shaders\\skybox_frag.spv", &fragment_byte_code, &fragment_code_size);
    shader_setup_skybox = vkal_create_shaders(vertex_byte_code, vertex_code_size, fragment_byte_code, fragment_code_size);
    
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

    // Pipeline for regular models
	VkPipelineLayout pipeline_layout = create_pipeline_layout(descriptor_set_layouts, descriptor_set_layout_count, push_constant_ranges, 2);
    model_pipeline = vkal_create_graphics_pipeline(descriptor_set_layouts, descriptor_set_layout_count,
                                                   shader_setup, 
                                                   VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL, 
                                                   VK_CULL_MODE_NONE, VK_POLYGON_MODE_FILL, 
                                                   VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FRONT_FACE_CLOCKWISE,
												   vkal_info->render_pass, pipeline_layout);
   

	// Another pipeline for rendering the skybox
	skybox_pipeline = vkal_create_graphics_pipeline(descriptor_set_layouts, descriptor_set_layout_count, 
                                                    shader_setup_skybox, 
                                                    VK_FALSE, VK_COMPARE_OP_LESS_OR_EQUAL, 
                                                    VK_CULL_MODE_BACK_BIT, VK_POLYGON_MODE_FILL, 
                                                    VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FRONT_FACE_CLOCKWISE,
													vkal_info->render_pass, pipeline_layout);
    
	// Uniform Buffers
	UniformBuffer model_ubo = vkal_create_uniform_buffer(descriptor_set_layout_global, sizeof(ModelUBO));
	UniformBuffer skybox_ubo = vkal_create_uniform_buffer(descriptor_set_layout_global, sizeof(SkyboxUBO));
    
	depth_buffer_enable = 1;
	cull_enable = 1;
    
    // Camera init
	camera.pos = glm::vec3(0, 0, 10);
	camera.center = glm::vec3(0, 0, 0);
	camera.up = glm::vec3(0, -1, 0);

    
	// Initial Projection Matrix 
	glm::mat4 projection = glm::perspective(glm::radians(45.f), 1.f, 0.1f, 1000.0f);
    
	// Memory Arenas:
	void * memory = p.initialize_memory(1000 * 1024 * 1024);
#define MANY_MODEL_COUNT 100
	MemoryArena model_arena;
	initialize_arena(&model_arena, memory, (MANY_MODEL_COUNT) * sizeof(Model) );
	MemoryArena texture_arena;
	initialize_arena(&texture_arena, (uint8_t*)memory + model_arena.size, 100 * 1024 * 1024);
    
	// Cubemap Textures:
	Image cubemap_posx = load_image("..\\assets\\textures\\mountains_cubemap\\posx.jpg");
	Image cubemap_negx = load_image("..\\assets\\textures\\mountains_cubemap\\negx.jpg");
	Image cubemap_posy = load_image("..\\assets\\textures\\mountains_cubemap\\posy.jpg");
	Image cubemap_negy = load_image("..\\assets\\textures\\mountains_cubemap\\negy.jpg");
	Image cubemap_posz = load_image("..\\assets\\textures\\mountains_cubemap\\posz.jpg");
	Image cubemap_negz = load_image("..\\assets\\textures\\mountains_cubemap\\negz.jpg");
	Image cubemap_array[] = {
		cubemap_posx, cubemap_negx, 
		cubemap_posy, cubemap_negy,
		cubemap_posz, cubemap_negz
	};
	uint32_t size_per_cubemap_image = cubemap_negx.width * cubemap_negx.height * 4;
	unsigned char * cubemap_data = (unsigned char *)p.mmalloc(&texture_arena, 6*size_per_cubemap_image);
	unsigned char * cubemap_ptr = cubemap_data;
	for (int i = 0; i < 6; ++i) {
		memcpy(cubemap_ptr, cubemap_array[i].data, size_per_cubemap_image);
		cubemap_ptr += size_per_cubemap_image;
	}
    
	Texture cubemap_texture = vkal_create_texture(
		descriptor_set_layout_cubemap,
		cubemap_data,
		cubemap_negx.width, cubemap_negx.height, cubemap_negx.channels,
		VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT, VK_IMAGE_VIEW_TYPE_CUBE,
		0, 1,
		0, 6,
		VK_FILTER_LINEAR, VK_FILTER_LINEAR);
    
	// Models
	Model g_model = create_model_from_file("..\\assets\\obj\\sphere.obj");
	g_model.offset = vkal_vertex_buffer_add(g_model.vertices, g_model.vertex_count);
	g_model.pos = glm::vec3(0, 0, 0);
	glm::mat4 translate = glm::translate(glm::mat4(1.f), g_model.pos);
	g_model.model_matrix = translate;

	Model g_cube = setup_skybox();
	g_cube.offset = vkal_vertex_buffer_add(g_cube.vertices, g_cube.vertex_count);
	g_cube.pos = glm::vec3(0, 0, 0);
	g_cube.model_matrix = translate;
    
	// Initialize Uniform Buffer
	uniform_buffer.texture_mapping_type = 1;
	update_uniform(&model_ubo, &uniform_buffer);
    
    //Timer Setup
	double timer_frequency = glfwGetTimerFrequency();
	double timestep = 1.f / timer_frequency; // step in second
	double title_update_timer = 0;
	double dt = 0;
    
	int width = WIDTH;
	int height = HEIGHT;
	// MAIN LOOP
	while (!glfwWindowShouldClose(window)) {
        
		double start_time = glfwGetTime();
        
		glfwPollEvents();
        
		// Start the Dear ImGui frame
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		ImGui::Begin("Versuch 4 - Cubemapping");                          // Create a window called "Hello, world!" and append into it.
		ImGui::Text("Select Texturing Mode");
		ImGui::RadioButton("Object Linear", (int*)&uniform_buffer.texture_mapping_type, 0);
		ImGui::RadioButton("Eye Linear", (int*)&uniform_buffer.texture_mapping_type, 1);
		ImGui::RadioButton("Cubemapping", (int*)&uniform_buffer.texture_mapping_type, 2);
		ImGui::End();
		ImGui::Render();

		update(float(dt*1000.f));
        
		// update model
		camera.center = g_model.pos;

		// Need new dimensions?
		if (framebuffer_resized) {
			framebuffer_resized = 0;
			glfwGetFramebufferSize(window, &width, &height);
		}
        
		// Mouse update
		if (!ImGui::IsMouseHoveringAnyWindow() && !ImGui::IsAnyItemActive()) {
			update_camera((float)dt);
		}
		
		uniform_buffer.projection = glm::perspective(glm::radians(45.f), float(width) / float(height), 0.1f, 1000.f);
		uniform_buffer.view = glm::lookAt(camera.pos, camera.center, camera.up);
		update_uniform(&model_ubo, &uniform_buffer);
        
		uniform_buffer_skybox.projection = glm::perspective(glm::radians(45.f), float(width) / float(height), 0.1f, 1000.f);
		uniform_buffer_skybox.view = glm::lookAt(camera.pos, camera.center, camera.up);
		update_uniform(&skybox_ubo, &uniform_buffer_skybox);
        
		uint32_t image_id = vkal_get_image();
		if (image_id >= 0) {
			vkal_begin(image_id, vkal_info->command_buffers[image_id], vkal_info->render_pass);
			vkal_bind_descriptor_set(image_id, 1, cubemap_texture.descriptor_set, pipeline_layout);
			vkal_bind_descriptor_set(image_id, 0, skybox_ubo.descriptor_set, pipeline_layout);
			vkal_record_models(image_id, skybox_pipeline, pipeline_layout, NULL, 0, &g_cube, 1);
			vkal_bind_descriptor_set(image_id, 0, model_ubo.descriptor_set, pipeline_layout);
			vkal_record_models(image_id, model_pipeline, pipeline_layout, NULL, 0, &g_model, 1);
			// Record Imgui Draw Data and draw funcs into command buffer
			ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), vkal_info->command_buffers[image_id]);
			vkal_end(vkal_info->command_buffers[image_id]);

			// Submit to GPU
			VkCommandBuffer command_buffers[] = { vkal_info->command_buffers[image_id] };
			vkal_queue_submit(command_buffers, 1);
			vkal_present(image_id);			
		}
        
		double end_time = glfwGetTime();
		dt = end_time - start_time;
		title_update_timer += dt;
        
		if ((title_update_timer) > .5f) {
			char window_title[256];
			sprintf(window_title, "time elapsed: %f", (dt * 1000.f));
			glfwSetWindowTitle(window, window_title);
			title_update_timer = 0;
		}
	}
    
	vkal_destroy_graphics_pipeline(model_pipeline);
	vkal_destroy_graphics_pipeline(skybox_pipeline);
	cleanup();
    
	return 0;
}

