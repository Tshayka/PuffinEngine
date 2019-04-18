#include <fstream>
#include <iostream>

#include "TriggerArea.hpp"

// ------- Constructors and dectructors ------------- //

TriggerArea::TriggerArea(std::string name, std::string description, glm::vec3 position, ActorType type,  std::vector<std::shared_ptr<Actor>>& actors, TriggerAreaType triggerAreaType, glm::vec3 dimensions, std::shared_ptr<Actor>& mainCharacter) 
: Actor(name, description, position, type, actors) {
	// create save file
#if BUILD_ENABLE_VULKAN_DEBUG
	std::cout << "Trigger area created\n";
#endif
	this->triggerAreaType = triggerAreaType;
	this->dimensions = dimensions;
	this->mainCharacter = mainCharacter;

	CalculateBoundaryPoints();

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
		bool found = false;
		for (const auto& q : std::dynamic_pointer_cast<Character>(mainCharacter)->quests){
			if (q->id == 1) found = true;
		}

		if(!found) {
			std::shared_ptr<Quest<decltype(std::dynamic_pointer_cast<Character>(mainCharacter)->gold)>> tsk1 = std::make_shared<Quest<decltype(std::dynamic_pointer_cast<Character>(mainCharacter)->gold)>>(std::dynamic_pointer_cast<Character>(mainCharacter), 1500, "Get 1500 of gold", std::dynamic_pointer_cast<Character>(mainCharacter)->gold);
			std::dynamic_pointer_cast<Character>(mainCharacter)->quests.emplace_back(tsk1);
		}
	}

	if(triggerAreaType == TriggerAreaType::Info) {
		// print info to the screen
	}

	if(triggerAreaType == TriggerAreaType::Portal) {
		mainCharacter->SetPosition(glm::vec3(0.0f));
	}
}

void TriggerArea::CalculateBoundaryPoints(){
	currentAabb.max = glm::vec3(dimensions.x/2, dimensions.y, dimensions.z/2) + position;
	currentAabb.min = glm::vec3(-dimensions.x/2, 0.0f, -dimensions.z/2) + position;
}

glm::vec3 TriggerArea::CalculateSelectionIndicatorColor() {
	if (active) return glm::vec3(0.5f, 1.0f, 0.0f);
	return glm::vec3(0.5f, 0.5f, 0.5f);
}

void TriggerArea::CheckIfTriggered() {
	if(enginetool::ScenePart::Overlaps(mainCharacter->currentAabb, this->currentAabb) && active) {
		std::cout << mainCharacter->name << " collides with: " << this->name << "\n";
		Action();
	}
}

void TriggerArea::UpdatePosition(float dt) {
	if(movementGoal!=glm::vec3(0.0f,0.0f,0.0f) && !manualControl) Actor::CheckIfInTheDestination();

	movement.x = Approach(movementGoal.x, movement.x, dt * 500);
	movement.y = Approach(movementGoal.y, movement.y, dt * 500);
 	movement.z = Approach(movementGoal.z, movement.z, dt * 500);

	position += movement * dt;

	CheckIfTriggered();
}