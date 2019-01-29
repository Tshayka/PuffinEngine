#include <fstream>
#include <iostream>

#include "Character.hpp"

// ------- Constructors and dectructors ------------- //

Character::Character(std::string name, std::string description, glm::vec3 position, ActorType type) 
: Actor(name, description, position, type) {
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

glm::vec3 Character::CalculateSelectionIndicatorColor(){
	float perentOfMax = currentHealth / maxHealth;
	return glm::vec3(2.0f * (1.0f - perentOfMax), 2.0f * perentOfMax, 0.0f);
}

void Character::UpdatePosition(float dt) {
	//if(movementGoal!=glm::vec3(0.0f,0.0f,0.0f) && !manualControl) Actor::CheckIfInTheDestination();
	
	movement.x = Approach(movementGoal.x, movement.x, dt * 1000);
	movement.y = Approach(movementGoal.y, movement.y, dt * 1000);
 	movement.z = Approach(movementGoal.z, movement.z, dt * 1000);

	velocity = movement;
	position += velocity * dt;

	if (position.y <= groundLevel) {
		position.y = groundLevel;
		if (inAir) {
			inAir = false;
			SetState(ActorState::Idle);
		}
	}
	else SetState(ActorState::Fall);
		
	if (inAir) movementGoal.y -= 100.0f;

	Actor::UpdateAABB();
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
