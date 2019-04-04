#include <fstream>
#include <iostream>

#include "Weapon.hpp"

// ------- Constructors and dectructors ------------- //

Weapon::Weapon(std::string name, std::string description, glm::vec3 position,  std::vector<std::shared_ptr<Actor>>& actors, ActorType actorType, ItemType itemType) 
: Item(name, description, position, actorType, actors, itemType)  {
	// create save file
#if BUILD_ENABLE_VULKAN_DEBUG
	std::cout << "Weapon created\n";
#endif
}

Weapon::~Weapon() {
#if BUILD_ENABLE_VULKAN_DEBUG
	std::cout << "Weapon destroyed\n";
#endif
}

void Weapon::Init() {
}
