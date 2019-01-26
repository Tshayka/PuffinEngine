#pragma once

#include "Actor.hpp"

class Character : public Actor {
public:
	Character(std::string name, std::string description, glm::vec3 position, ActorType type);
	virtual ~Character();

	virtual glm::vec3 CalculateSelectionIndicatorColor() override;
	void Init(unsigned int maxHealth, int currentHealth, unsigned int gold);
	void StartJump();
	void EndJump();
    void UpdatePosition(float) override;

	unsigned int maxHealth;
	int currentHealth;
	unsigned int gold;
	bool onGround = false;
private:
	std::string albedoTexture = "puffinEngine/assets/textures/icons/cameraIcon.jpg";	
};