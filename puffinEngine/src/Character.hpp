#pragma once

#include "Actor.hpp"

class Character : public Actor {
public:
	Character(std::string name, std::string description, glm::vec3 position);
	virtual ~Character();

    ActorType GetType() override;
	void Init(unsigned int maxHealth, int currentHealth, unsigned int gold);
    
	unsigned int maxHealth;
	int currentHealth;
	unsigned int gold;
    ActorType type = ActorType::Character;
};