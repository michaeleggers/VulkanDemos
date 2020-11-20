#ifndef VKAL_IMGUI_H
#define VKAL_IMGUI_H

#include <imgui\imgui.h>
#include <imgui\imgui_impl_glfw.h>
#include <imgui\imgui_impl_vulkan.h>

#include "vkal.h"

void check_vk_result(VkResult err);
void init_imgui(VkalInfo * vkal_info, GLFWwindow * window);

#endif