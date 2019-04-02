#pragma once

#include "Actor.hpp"

class TriggerArea : public Actor {
public:
	TriggerArea(std::string name, std::string description, glm::vec3 position, ActorType type,  std::vector<std::shared_ptr<Actor>>& actors);
	virtual ~TriggerArea();
		
private:
	void UpdatePosition(float) override;
};