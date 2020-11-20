
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include <assimp/cimport.h>        // Plain-C interface
#include <assimp/scene.h>          // Output data structure
#include <assimp/postprocess.h>    // Post processing flags

#include <imageloader/ImageLoader.h>

#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_vulkan.h>

#include "platform.h"
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

struct ModelUBO
{
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

struct Image_t
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

// GLFW callbacks
static void glfw_key_callback(GLFWwindow * window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		printf("escape key pressed\n");
		glfwSetWindowShouldClose(window, GLFW_TRUE);
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

Image_t load_image(const char * file)
{
	Image_t image;
	img::ImageLoader imageloader;
	image.data = imageloader.LoadTextureFromFile(file , &image.width, &image.height, false);
	massert(image.data);
	return image;
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
}

Model build_skybox_model()
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
    
	init_window();
	init_platform(&p);
	// Memory Arenas:
	void * memory = p.initialize_memory(1000 * 1024 * 1024);
	MemoryArena model_arena;
	initialize_arena(&model_arena, memory, 100 * 1024 * 1024);
	MemoryArena texture_arena;
	initialize_arena(&texture_arena, (uint8_t*)memory + model_arena.size, 100 * 1024 * 1024);

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
	VkalInfo * vkal_info = init_vulkan(
		window,
		device_extensions, device_extension_count,
		instance_extensions, instance_extension_count);
	init_imgui(vkal_info, window);
	
	// Descriptor Sets Setup

	// 1. Descriptor Set Layout Bindings:
	// - what kind of resources we want to use in the shaders
	// - what bindings (which ID) do they have? Will be used to address resources in shaders with
	//   eg 
	//     layout (set = 0, binding = 0) uniform UBO
	VkDescriptorSetLayoutBinding set_layout_global[] = {
		{
			0, // binding id ( matches with shader )
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			1, // number of resources with this layout binding
			VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT,
			0
		},
		{
			1, // binding ID
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			1,
			VK_SHADER_STAGE_FRAGMENT_BIT,
			0
		}
	};
    

	// 2. Descriptor Set Layouts:
	// - Creates layout from Desc Set Layout Bindings, 
	//   used later to actually allocate descriptor set(s) from the descriptor pool.
	VkDescriptorSetLayout descriptor_set_layout_global = vkal_create_descriptor_set_layout(set_layout_global, 2);
	//DescriptorSetLayout descriptor_set_layout_cubemap = vkal_create_descriptor_set_layout(set_layout_cubemap, 1);
	VkDescriptorSetLayout descriptor_set_layouts[] = {
		descriptor_set_layout_global
	}; // Order in this array matches the set number in shaders. We only have one set, in this example.
	uint32_t descriptor_set_layout_count = sizeof(descriptor_set_layouts) / sizeof(*descriptor_set_layouts);
    
	// 3. Allocate Descriptor Set(s)
	// allocate a descriptor set for each shader from a Descriptor Pool with the Layout defined by
	// Descriptor Set Layout(s).
	VkDescriptorSet * descriptor_set_scene = (VkDescriptorSet*)malloc(sizeof(VkDescriptorSet));
	vkal_allocate_descriptor_sets(vkal_info->descriptor_pool, descriptor_set_layouts, 1, &descriptor_set_scene);
	VkDescriptorSet * descriptor_set = (VkDescriptorSet*)malloc(sizeof(VkDescriptorSet));
	vkal_allocate_descriptor_sets(vkal_info->descriptor_pool, descriptor_set_layouts, 1, &descriptor_set);

	// Model shader
	uint8_t * vertex_byte_code = 0;
	int vertex_code_size;
	p.rfb("shaders\\scene_vert.spv", &vertex_byte_code, &vertex_code_size);
	uint8_t * fragment_byte_code = 0;
	int fragment_code_size;
	p.rfb("shaders\\scene_frag.spv", &fragment_byte_code, &fragment_code_size);
	ShaderStageSetup shader_setup = vkal_create_shaders(vertex_byte_code, vertex_code_size, fragment_byte_code, fragment_code_size);
	
    // Skybox shader
    p.rfb("shaders\\skybox_vert.spv", &vertex_byte_code, &vertex_code_size);
	p.rfb("shaders\\skybox_frag.spv", &fragment_byte_code, &fragment_code_size);
    ShaderStageSetup shader_setup_skybox = vkal_create_shaders(vertex_byte_code, vertex_code_size, fragment_byte_code, fragment_code_size);
    
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
	VkPipelineLayout pipeline_layout = vkal_create_pipeline_layout(descriptor_set_layouts, descriptor_set_layout_count, push_constant_ranges, 2);
	VkPipeline model_pipeline = vkal_create_graphics_pipeline(
                                                   shader_setup, 
                                                   VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL, 
                                                   VK_CULL_MODE_NONE, VK_POLYGON_MODE_FILL, 
                                                   VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FRONT_FACE_CLOCKWISE,
												   vkal_info->render_pass, pipeline_layout);
	// Another pipeline for rendering the skybox
	VkPipeline skybox_pipeline = vkal_create_graphics_pipeline(
                                                    shader_setup_skybox, 
                                                    VK_FALSE, VK_COMPARE_OP_LESS_OR_EQUAL, 
                                                    VK_CULL_MODE_BACK_BIT, VK_POLYGON_MODE_FILL, 
                                                    VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FRONT_FACE_CLOCKWISE,
													vkal_info->render_pass, pipeline_layout);
    
	
	// Uniform Buffers
	// TODO(Michael): Apparently the write descriptor set is important! The size of a uniform (or whatever data) has to be
	// constant if the same descriptor set should be used with different pipelines (= shaders).
	// If the layout also has to remain the same (= is it mat4 or vec4s or ...), I don't know!
	// So probably there should be distinct descriptor sets if the data is different, not only the type of descriptor sets (which wrap the data).
	UniformBuffer model_ubo  = vkal_create_uniform_buffer(sizeof(ModelUBO), 0);
	vkal_update_descriptor_set_uniform(descriptor_set_scene[0], model_ubo);
	UniformBuffer skybox_ubo = vkal_create_uniform_buffer(sizeof(SkyboxUBO), 0);
	vkal_update_descriptor_set_uniform(descriptor_set[0], skybox_ubo);

    // Camera init
	camera.pos = glm::vec3(0, 0, 10);
	camera.center = glm::vec3(0, 0, 0);
	camera.up = glm::vec3(0, -1, 0);

	// Initial Projection Matrix 
	glm::mat4 projection = glm::perspective(glm::radians(45.f), 1.f, 0.1f, 1000.0f);
 
	// Cubemap Textures:
	Image_t cubemap_posx = load_image("..\\assets\\textures\\mountains_cubemap\\posx.jpg");
	Image_t cubemap_negx = load_image("..\\assets\\textures\\mountains_cubemap\\negx.jpg");
	Image_t cubemap_posy = load_image("..\\assets\\textures\\mountains_cubemap\\posy.jpg");
	Image_t cubemap_negy = load_image("..\\assets\\textures\\mountains_cubemap\\negy.jpg");
	Image_t cubemap_posz = load_image("..\\assets\\textures\\mountains_cubemap\\posz.jpg");
	Image_t cubemap_negz = load_image("..\\assets\\textures\\mountains_cubemap\\negz.jpg");
	Image_t cubemap_array[] = {
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
		1,
		cubemap_data,
		cubemap_negx.width, cubemap_negx.height, cubemap_negx.channels,
		VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT, VK_IMAGE_VIEW_TYPE_CUBE,
		0, 1,
		0, 6,
		VK_FILTER_LINEAR, VK_FILTER_LINEAR);
	vkal_update_descriptor_set_texture(descriptor_set[0], cubemap_texture);
	vkal_update_descriptor_set_texture(descriptor_set_scene[0], cubemap_texture);

	// Models
	Model g_model = create_model_from_file("..\\assets\\obj\\kitten.obj");
	g_model.offset = vkal_vertex_buffer_add(g_model.vertices, g_model.vertex_count);
	g_model.pos = glm::vec3(0, 0, 0);
	glm::mat4 translate = glm::translate(glm::mat4(1.f), g_model.pos);
	g_model.model_matrix = translate;

	Model g_cube = build_skybox_model();
	g_cube.offset = vkal_vertex_buffer_add(g_cube.vertices, g_cube.vertex_count);
	g_cube.pos = glm::vec3(0, 0, 0);
	g_cube.model_matrix = translate;
    
    //Timer Setup
	double timer_frequency = glfwGetTimerFrequency();
	double timestep = 1.f / timer_frequency; // step in second
	double title_update_timer = 0;
	double dt = 0;
    
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
		ImGui::Begin("Versuch 4 - Cubemapping");                          // Create a window called "Hello, world!" and append into it.
		ImGui::Text("Select Texturing Mode");
		ImGui::RadioButton("Object Linear", (int*)&uniform_buffer.texture_mapping_type, 0);
		ImGui::RadioButton("Eye Linear", (int*)&uniform_buffer.texture_mapping_type, 1);
		ImGui::RadioButton("Cubemapping", (int*)&uniform_buffer.texture_mapping_type, 2);
		ImGui::End();
		ImGui::Render();
        
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
		vkal_update_uniform(&model_ubo, &uniform_buffer);
        
		uniform_buffer_skybox.projection = glm::perspective(glm::radians(45.f), float(width) / float(height), 0.1f, 1000.f);
		uniform_buffer_skybox.view = glm::lookAt(camera.pos, camera.center, camera.up);
		vkal_update_uniform(&skybox_ubo, &uniform_buffer_skybox);
        

		{
			/* Acquire next image from Swapchain */
			uint32_t image_id = vkal_get_image();

			/* Start recording */
			vkal_begin(image_id, vkal_info->command_buffers[image_id], vkal_info->render_pass);

			/* Bind descriptor set for Skybox and draw models */
			vkal_bind_descriptor_set(image_id, 0, &descriptor_set[0], 1, pipeline_layout);
			vkal_record_models_pc(image_id, skybox_pipeline, pipeline_layout, &g_cube, 1);
			/* Bind descriptor set for rest of the scene and draw models */
			vkal_bind_descriptor_set(image_id, 0, &descriptor_set_scene[0], 1, pipeline_layout);
			vkal_record_models_pc(image_id, model_pipeline, pipeline_layout, &g_model, 1);
			
			/* Record Imgui Draw Data and draw funcs into command buffer */
			ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), vkal_info->command_buffers[image_id]);
			
			/* Finish recording */
			vkal_end(vkal_info->command_buffers[image_id]);

			/* Submit to GPU */
			VkCommandBuffer command_buffers[] = { vkal_info->command_buffers[image_id] };
			vkal_queue_submit(command_buffers, 1);

			/* Present */
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

