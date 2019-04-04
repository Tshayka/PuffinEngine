#pragma once

#include "Item.hpp"

class Weapon : public Item {
public:
	Weapon(std::string name, std::string description, glm::vec3 position,  std::vector<std::shared_ptr<Actor>>& actors, ActorType actorType, ItemType itemType);
	virtual ~Weapon();

	void Init();
		
private:
};