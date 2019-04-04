#pragma once

#include "Character.hpp"

class MainCharacter : public Character {
public:
	MainCharacter(std::string name, std::string description, glm::vec3 position, ActorType type,  std::vector<std::shared_ptr<Actor>>& actors);
	virtual ~MainCharacter();

	enginetool::ScenePart* assignedMesh;

private:
};