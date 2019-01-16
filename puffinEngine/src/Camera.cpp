#include <iostream>

#include "Camera.hpp"
#include "ErrorCheck.hpp"


// ------- Constructors and dectructors ------------- //

Camera::Camera(std::string name, std::string description, glm::vec3 position, ActorType type) 
: Actor(name, description, position, type) {
#if BUILD_ENABLE_VULKAN_DEBUG
	std::cout << "Camera created\n";
#endif 
}

Camera::~Camera() {
#if BUILD_ENABLE_VULKAN_DEBUG
	std::cout << "Camera destroyed\n";
#endif 
}

// ---------------- Main functions ------------------ //

void Camera::Init(float move_x, float move_y, float move_z,
	float move_goal_x, float move_goal_y, float move_goal_z,
	float dir_x, float dir_y, float dir_z, 
	float up_x, float up_y, float up_z, 
	float fov, float cnear, float cfar, float horiz, float vert)
{
	movement = glm::vec3(move_x, move_y, move_z); 
	movementGoal = glm::vec3(move_goal_x, move_goal_y, move_goal_z);
	direction = glm::vec3(dir_x, dir_y, dir_z);
	up = glm::vec3(up_x, up_y, up_z);
	view = position + direction;
	
	FOV = fov;
	clippingNear = cnear;
	clippingFar = cfar;

	pitch = vert;
	yaw = horiz;
}

void Camera::UpdatePosition(float dt) {
	// Smooth movement and edge case in approach, without is movement is const
	movement.x = Approach(movementGoal.x, movement.x, dt * 80);
	movement.y = Approach(movementGoal.y, movement.y, dt * 80);
	movement.z = Approach(movementGoal.z, movement.z, dt * 80);

	direction = glm::vec3(cos(pitch) * sin(yaw), sin(pitch), cos(pitch) * cos(yaw));

	foward = direction;
	foward.y = 0.0f;
	foward = glm::normalize(foward);

	velocity = foward * movement.x + glm::cross(up, foward) * movement.z;
	velocity.y = movement.y;
	position += velocity * dt;
	view = position + direction; 
}

void Camera::ResetPosition(){
	Actor::ResetPosition();
	view = glm::vec3(0.0f, 0.0f, 0.0f); 
	up = glm::vec3(0.0f, 1.0f, 0.0f); 
	FOV = 60.0f;
	clippingNear = 0.001f;
	clippingFar = 1000.0f;
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