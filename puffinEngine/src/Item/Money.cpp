#include <fstream>
#include <iostream>

#include "Money.hpp"

// ------- Constructors and dectructors ------------- //

Money::Money(std::string name, std::string description, glm::vec3 position,  std::vector<std::shared_ptr<Actor>>& actors, ActorType actorType, ItemType itemType) 
: Item(name, description, position, actorType, actors, itemType)  {
	// create save file
#if BUILD_ENABLE_VULKAN_DEBUG
	std::cout << "Money created\n";
#endif
}

Money::~Money() {
#if BUILD_ENABLE_VULKAN_DEBUG
	std::cout << "Money destroyed\n";
#endif
}

void Money::Init() {
}