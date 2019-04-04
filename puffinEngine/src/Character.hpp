#pragma once

#include "Actor.hpp"
#include "Item/Item.hpp"

enum class BodySlots {
	LeftHand, RightHand, Head, Neck, Chest, Belt1, Belt2, Finger1, Finger2, Cloak, Shoes
};

class Observer {
    public:
    virtual void Update(unsigned int value) = 0;
    std::string description;
};

class Character : public Actor {
public:
	Character(std::string name, std::string description, glm::vec3 position, ActorType type,  std::vector<std::shared_ptr<Actor>>& actors);
	virtual ~Character();

	void Attach(Observer *observer);
	virtual glm::vec3 CalculateSelectionIndicatorColor() override;
	void FindCollidingItem();
	void Init(unsigned int maxHealth, int currentHealth, unsigned int gold);
	void Notify();

	unsigned int maxHealth;
	int currentHealth;
	unsigned int gold;
	bool onGround = false;

	std::vector<Item> inventory;


		
private:
	void AddToInventory(Item* item);
	void AddGold(unsigned int amount);
	bool CheckItem(Item* item);
	void UpdatePosition(float) override;

	std::string albedoTexture;
	std::vector<Observer*> obervatorsToUpdate;	
};