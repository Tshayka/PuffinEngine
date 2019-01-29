#pragma once

#include "Actor.hpp"

class Character : public Actor {
public:
	Character(std::string name, std::string description, glm::vec3 position, ActorType type);
	virtual ~Character();

	enum class BodySlots {
		LeftHand, RightHand, Head, Neck, Chest, Belt1, Belt2, Finger1, Finger2, Cloak, Shoes
	};

	virtual glm::vec3 CalculateSelectionIndicatorColor() override;
	void Init(unsigned int maxHealth, int currentHealth, unsigned int gold);

	unsigned int maxHealth;
	int currentHealth;
	unsigned int gold;
	bool onGround = false;

private:
	void UpdatePosition(float) override;

	std::string albedoTexture;	
};