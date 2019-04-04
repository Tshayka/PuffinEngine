#include <fstream>
#include <iostream>

#include "TriggerArea.hpp"

// ------- Constructors and dectructors ------------- //

TriggerArea::TriggerArea(std::string name, std::string description, glm::vec3 position, ActorType type,  std::vector<std::shared_ptr<Actor>>& actors, TriggerAreaType triggerAreaType) 
: Actor(name, description, position, type, actors) {
	// create save file
#if BUILD_ENABLE_VULKAN_DEBUG
	std::cout << "Trigger area created\n";
#endif
	this->triggerAreaType = triggerAreaType;

	// create position observer

}

TriggerArea::~TriggerArea() {
#if BUILD_ENABLE_VULKAN_DEBUG
	std::cout << "Trigger area destroyed\n";
#endif
	// destroy position observer

}

void TriggerArea::Action() {
	if(triggerAreaType == TriggerAreaType::GiveQuest) {
		// give quest to character
	}

	if(triggerAreaType == TriggerAreaType::Info) {
		// print info to the screen
	}

}

glm::vec3 TriggerArea::CalculateSelectionIndicatorColor() {
	if (activated) return glm::vec3(1.0f, 0.0f, 0.0f);
	return glm::vec3(0.0f, 1.0f, 0.0f);
}

void TriggerArea::UpdatePosition(float dt) {
	if(movementGoal!=glm::vec3(0.0f,0.0f,0.0f) && !manualControl) Actor::CheckIfInTheDestination();

	movement.x = Approach(movementGoal.x, movement.x, dt * 500);
	movement.y = Approach(movementGoal.y, movement.y, dt * 500);
 	movement.z = Approach(movementGoal.z, movement.z, dt * 500);

	position += movement * dt;
}