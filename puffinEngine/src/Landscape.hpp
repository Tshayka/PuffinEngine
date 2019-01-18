#pragma once

#include <glm/glm.hpp>

#include "Actor.hpp"

class Landscape : public Actor {
public:
	Landscape(std::string name, std::string description, glm::vec3 position, ActorType type);
	virtual ~Landscape();

	virtual glm::vec4 CalculateSelectionIndicatorColor() override;
	void UpdatePosition(float) override;
	void Init(unsigned int maxHealth, int currentHealth);
	void ResetPosition();
	
	unsigned int maxHealth;
	int currentHealth;

private:
	std::string albedoTexture;
};