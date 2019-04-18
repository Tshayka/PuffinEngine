#pragma once

#include "Actor.hpp"
#include "CharacterObserver/Quest.cpp"

class TriggerArea : public Actor {
public:
	TriggerArea(std::string name, 
				std::string description, 
				glm::vec3 position, 
				ActorType type,  
				std::vector<std::shared_ptr<Actor>>& actors, 
				TriggerAreaType triggerAreaType, 
				glm::vec3 dimensions, 
				std::shared_ptr<Actor>& mainCharacter);
	
	virtual ~TriggerArea();

	void Action();
	virtual glm::vec3 CalculateSelectionIndicatorColor() override;

	bool active = true; 

	uint32_t indexBaseAabb;
		
private:
	void UpdatePosition(float) override;
	void CalculateBoundaryPoints();
	void CheckIfTriggered();

	TriggerAreaType triggerAreaType;
	glm::vec3 dimensions;
	std::shared_ptr<Actor> mainCharacter;
};