#pragma once

#include "Actor.hpp"

class Character : public Actor {
public:
	Character(std::string name, std::string description, glm::vec3 position, ActorType type);
	virtual ~Character();

	virtual glm::vec4 CalculateSelectionIndicatorColor() override;
	void Init(unsigned int maxHealth, int currentHealth, unsigned int gold);
	void StartJump();
	void EndJump();
    void UpdatePosition(float) override;

	unsigned int maxHealth;
	int currentHealth;
	unsigned int gold;
	bool onGround = false;

	glm::vec3 gravity = glm::vec3(0.0f, -10.0f, 0.0f);
	float closestPointBelow;

private:
	std::string albedoTexture = "puffinEngine/assets/textures/icons/cameraIcon.jpg";	
};