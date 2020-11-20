#ifndef CAMERA_H
#define CAMERA_H

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/ext.hpp>

struct Camera
{
	glm::vec3 pos;
	glm::vec3 center;
	glm::vec3 up;
	glm::vec3 right;
};


void rotate_camera(Camera * camera, float dt, float up_angle, float right_angle);
void dolly_camera(Camera * camera, float dt, double translate);

#endif