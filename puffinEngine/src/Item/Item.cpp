#include <fstream>
#include <iostream>

#include "Item.hpp"

// ------- Constructors and dectructors ------------- //

Item::Item(std::string name, std::string description, glm::vec3 position, ActorType type,  std::vector<std::shared_ptr<Actor>>& actors, ItemType it) 
: Actor(name, description, position, type, actors), itemType(it) {
	// create save file
#if BUILD_ENABLE_VULKAN_DEBUG
	std::cout << "Item created\n";
#endif
}

Item::~Item() {
#if BUILD_ENABLE_VULKAN_DEBUG
	std::cout << "Item destroyed\n";
#endif
}

void Item::Init(const unsigned int value) {
    this->value = value;
}

unsigned int Item::GetValue() const {
	return value;
}

glm::vec3 Item::CalculateSelectionIndicatorColor(){
	return glm::vec3(0.0f, 0.5f, 1.0f);
}

void Item::UpdatePosition(float dt) {
	//if(movementGoal!=glm::vec3(0.0f,0.0f,0.0f) && !manualControl) Actor::CheckIfInTheDestination();

	groundLevel = DetectGroundLevel();
	CheckCollisions();
	
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

	UpdateAABB();
}
