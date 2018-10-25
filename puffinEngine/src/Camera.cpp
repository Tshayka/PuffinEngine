#include <iostream>

#include "Camera.h"

// ------- Constructors and dectructors ------------- //

Camera::Camera()
{
#if BUILD_ENABLE_VULKAN_DEBUG
	std::cout << "Camera created\n";
#endif 
}

Camera::~Camera()
{
#if BUILD_ENABLE_VULKAN_DEBUG
	std::cout << "Camera removed\n";
#endif 
}

// ---------------- Main functions ------------------ //

void Camera::InitCamera(float pos_x, float pos_y, float pos_z, 
	float move_x, float move_y, float move_z,
	float move_goal_x, float move_goal_y, float move_goal_z,
	float dir_x, float dir_y, float dir_z, float up_x, float up_y, float up_z, float fov,
	float cnear, float cfar, float horiz, float vert)
{
	cam_pos = glm::vec3(pos_x, pos_y, pos_z);
	cam_movement = glm::vec3(move_x, move_y, move_z); 
	cam_movement_goal = glm::vec3(move_goal_x, move_goal_y, move_goal_z);
	cam_dir = glm::vec3(dir_x, dir_y, dir_z);
	cam_up = glm::vec3(up_x, up_y, up_z);
	cam_view = cam_pos + cam_dir;
	
	cam_FOV = fov;
	cam_clip_near = cnear;
	cam_clip_far = cfar;

	pitch = vert ;
	yaw = horiz;
}

void Camera::UpdateCamera(float dt)
{
	cam_dir = glm::vec3(cos(pitch) * sin(yaw), sin(pitch), cos(pitch) * cos(yaw));

	// Smooth movement and edge case in approach, without is movement is const
	cam_movement.x = CamApproach(cam_movement_goal.x, cam_movement.x, dt * 80);
	cam_movement.y = CamApproach(cam_movement_goal.y, cam_movement.y, dt * 80);
	cam_movement.z = CamApproach(cam_movement_goal.z, cam_movement.z, dt * 80);

	foward = cam_dir;
	foward.y = 0.0f;
	glm::normalize(foward);

	cam_veloc = foward * cam_movement.x + glm::cross(cam_up, foward) * cam_movement.z;
	cam_veloc.y = cam_movement.y;
	cam_pos += cam_veloc * dt;
	cam_view = cam_pos + cam_dir; 
}

void Camera::DollyCamera(float cam_velocity_goal) // moving directly fovard or backward
{
	cam_movement_goal.x = cam_velocity_goal;
}

void Camera::TruckCamera(float cam_velocity_goal) 
{
	cam_movement_goal.z = cam_velocity_goal;
}

void Camera::PedestalCamera(float cam_velocity_goal) // moving directly up or down
{
	cam_movement_goal.y = cam_velocity_goal;
}

float Camera::CamApproach(float camera_goal, float camera_current, float dt)
{
	float flDifference = camera_goal - camera_current;

	if (flDifference > dt) return camera_current + dt;
	if (flDifference < -dt) return camera_current - dt;

	return camera_goal;
}

void Camera::ResetPosition(){
	cam_pos = glm::vec3(3.0f, 3.0f, 3.0f);
	cam_movement = glm::vec3(0.0f, 0.0f, 0.0f);
	cam_movement_goal = glm::vec3(0.0f, 0.0f, 0.0f); 
	cam_view = glm::vec3(0.0f, 0.0f, 0.0f); 
	cam_up = glm::vec3(0.0f, 1.0f, 0.0f); 

	cam_FOV = 60.0f;
	cam_clip_near = 0.001f;
	cam_clip_far = 1000.0f;
}

void Camera::MouseMove(double xpos, double ypos, int HEIGHT, int WIDTH, float sensitivity)
{
	yaw = sensitivity  * float(WIDTH /2 - xpos);
	pitch = sensitivity * float(HEIGHT / 2 - ypos);
}

void Camera::GamepadMove(double xpos, double ypos, int HEIGHT, int WIDTH, float sensitivity)
{
	yaw += sensitivity  * float(xpos);
	pitch += sensitivity/2.0f * float(ypos);
}