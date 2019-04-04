#pragma once

#include "../Actor.hpp"

class Item : public Actor {
public:
	Item(std::string name, std::string description, glm::vec3 position, ActorType type,  std::vector<std::shared_ptr<Actor>>& actors, ItemType itemType);
	virtual ~Item();

	virtual glm::vec3 CalculateSelectionIndicatorColor() override;
    unsigned int GetValue() const;
	void Init(const unsigned int value);

    ItemType itemType;
	bool onGround = false;
		
private:
	void UpdatePosition(float) override;

    unsigned int value;

	std::string albedoTexture;	
};