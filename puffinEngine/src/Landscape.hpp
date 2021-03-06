#pragma once

#include <glm/glm.hpp>

#include "Actor.hpp"

class Landscape : public Actor {
public:
	Landscape(std::string name, std::string description, glm::vec3 position, ActorType type,  std::vector<std::shared_ptr<Actor>>& actors);
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
	Sea(std::string name, std::string description, glm::vec3 position, ActorType type,  std::vector<std::shared_ptr<Actor>>& actors);	
	virtual ~Sea();

	void CreateMesh();
	std::vector<uint32_t> indices;// TODO remove in future
	std::vector<enginetool::VertexLayout> vertices; // TODO remove in future

private:

};

class Cloud : public Landscape {
public:
	Cloud(std::string name, std::string description, glm::vec3 position, ActorType type,  std::vector<std::shared_ptr<Actor>>& actors);	
	virtual ~Cloud();

private:

};