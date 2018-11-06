#include <fstream>
#include <iostream>

#include "Character.hpp"

// ------- Constructors and dectructors ------------- //

enum class BodySlots {
	LeftHand, RightHand, Head, Neck, Chest, Belt1, Belt2, Finger1, Finger2, Cloak, Shoes
};

Character::Character(std::string name, std::string description, glm::vec3 position) 
: Actor(name, description, position) {
	// create save file
	std::cout << "Character created\n";
}

Character::~Character() {
	std::cout << "Character destroyed\n";
}

ActorType Character::GetType() {
    return type;
}

void Character::Init(unsigned int maxHealth, int currentHealth, unsigned int gold) {
    this->maxHealth = maxHealth;
	this->currentHealth = currentHealth;
	this->gold = gold;
}
