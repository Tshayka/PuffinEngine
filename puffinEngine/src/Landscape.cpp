#include <iostream>

#include "Landscape.hpp"
#include "ErrorCheck.hpp"


// ------- Constructors and dectructors ------------- //

Landscape::Landscape(std::string name, std::string description, glm::vec3 position, ActorType type) 
: Actor(name, description, position, type) {
#if BUILD_ENABLE_VULKAN_DEBUG
	// create save file
	std::cout << "Character created\n";
#endif 
}

Landscape::~Landscape() {
#if BUILD_ENABLE_VULKAN_DEBUG
	std::cout << "Landscape destroyed\n";
#endif 
}

// ---------------- Main functions ------------------ //

void Landscape::Init() {

}

void Landscape::UpdatePosition(float dt) {
	movement.x = Approach(movementGoal.x, movement.x, dt * 80);
	movement.y = Approach(movementGoal.y, movement.y, dt * 80);
 	movement.z = Approach(movementGoal.z, movement.z, dt * 80);

	position += movement * dt;

	Actor::UpdateAABB();
}


// void Camera::UpdatePosition(float dt)
// {
// 	direction = glm::vec3(cos(pitch) * sin(yaw), sin(pitch), cos(pitch) * cos(yaw));

// 	// Smooth movement and edge case in approach, without is movement is const
// 	movement.x = Approach(movementGoal.x, movement.x, dt * 80);
// 	movement.y = Approach(movementGoal.y, movement.y, dt * 80);
// 	movement.z = Approach(movementGoal.z, movement.z, dt * 80);

// 	foward = direction;
// 	foward.y = 0.0f;
// 	glm::normalize(foward);

// 	velocity = foward * movement.x + glm::cross(up, foward) * movement.z;
// 	velocity.y = movement.y;
// 	position += velocity * dt;
// 	view = position + direction; 
// }

void Landscape::ResetPosition(){
	Actor::ResetPosition();
}
