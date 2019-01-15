#pragma once

#include <glm/glm.hpp>

#include "Actor.hpp"

class Landscape : public Actor {
public:
	Landscape(std::string name, std::string description, glm::vec3 position, ActorType type);
	virtual ~Landscape();

	void UpdatePosition(float) override;

	void Init();
	void ResetPosition();
	


private:
	std::string albedoTexture;
};