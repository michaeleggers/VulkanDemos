
#include <imgui\imgui.h>
#include <imgui\imgui_impl_glfw.h>
#include <imgui\imgui_impl_vulkan.h>

#include "vkal_imgui.h"
#include "vkal.h"

void check_vk_result(VkResult err)
{
	if (err == 0) return;
	printf("VkResult %d\n", err);
	if (err < 0)
		abort();
}

void init_imgui(VkalInfo * vkal_info, GLFWwindow * window)
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