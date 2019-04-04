#pragma once

#include "Item.hpp"

class Money : public Item {
public:
	Money(std::string name, std::string description, glm::vec3 position,  std::vector<std::shared_ptr<Actor>>& actors, ActorType actorType, ItemType itemType);
	virtual ~Money();

	void Init();
		
private:
};