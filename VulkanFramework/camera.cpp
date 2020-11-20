#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include "camera.h"

void rotate_camera(Camera * camera, float dt, float up_angle, float right_angle)
{
	glm::vec3 camera_dir = glm::normalize(camera->center - camera->pos);
	camera->right = glm::normalize(glm::cross(camera->up, camera_dir));
	camera->up = glm::vec3(0, -1, 0);

	glm::mat3 rotate_up = glm::rotate(glm::mat4(1.f), glm::radians(up_angle), camera->up);
	glm::mat3 rotate_right = glm::rotate(glm::mat4(1.f), glm::radians(right_angle), camera->right);
	camera->pos = rotate_up * rotate_right * camera->pos;
}

void dolly_camera(Camera * camera, float dt, double translate)
{
	glm::vec3 view_dir = normalize(camera->center - camera->pos);
	camera->pos = dt * (float)translate*view_dir + camera->pos;
}
