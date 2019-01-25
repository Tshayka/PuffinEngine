#pragma once

#include <glm/glm.hpp>

#include "Actor.hpp"

class Landscape : public Actor {
public:
	Landscape(std::string name, std::string description, glm::vec3 position, ActorType type);
	virtual ~Landscape();

	virtual glm::vec3 CalculateSelectionIndicatorColor() override;
	void UpdatePosition(float) override;
	void Init(unsigned int maxHealth, int currentHealth);
	void ResetPosition();
	
	unsigned int maxHealth;
	int currentHealth;

private:
	std::string albedoTexture;
};

class Sea : public Landscape {
public:
	Sea(std::string name, std::string description, glm::vec3 position, ActorType type);	
	virtual ~Sea();

	void CreateMesh();

private:

};

class Cloud : public Landscape {
public:
	Cloud(std::string name, std::string description, glm::vec3 position, ActorType type);	
	virtual ~Cloud();

private:

};