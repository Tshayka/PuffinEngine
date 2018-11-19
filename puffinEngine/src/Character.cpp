#include <fstream>
#include <iostream>

#include "Character.hpp"

// ------- Constructors and dectructors ------------- //

enum class BodySlots {
	LeftHand, RightHand, Head, Neck, Chest, Belt1, Belt2, Finger1, Finger2, Cloak, Shoes
};

Character::Character(std::string name, std::string description, glm::vec3 position) 
: Actor(name, description, position) {
	// create save file
	std::cout << "Character created\n";
}

Character::~Character() {
	std::cout << "Character destroyed\n";
}

void Character::Init(unsigned int maxHealth, int currentHealth, unsigned int gold) {
    this->maxHealth = maxHealth;
	this->currentHealth = currentHealth;
	this->gold = gold;
}

void Character::StartJump() {
	if (onGround) {
    	movementGoal.y = 30.0f;
		onGround = false;
	}
}

void Character::EndJump() {
    if(movementGoal.y > 15.0f)
		movementGoal.y = -10.0f;
}

void Character::UpdatePosition(float dt) {
	movement.x = Approach(movementGoal.x, movement.x, dt * 80);
	movement.y = Approach(movementGoal.y, movement.y, dt * 80);
 	movement.z = Approach(movementGoal.z, movement.z, dt * 80);

	movement += gravity * dt;
	position += movement * dt;

	//std::cout << onGround << " " << position.y << " " << movementGoal.y << std::endl;

	if (position.y < groundLevel){
		position.y = groundLevel;
		movementGoal.y = 0.0f;
		onGround = true;
	}
}


// void Camera::UpdatePosition(float dt) {
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