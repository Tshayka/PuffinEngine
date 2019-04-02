#include <fstream>
#include <iostream>

#include "TriggerArea.hpp"

// ------- Constructors and dectructors ------------- //

TriggerArea::TriggerArea(std::string name, std::string description, glm::vec3 position, ActorType type,  std::vector<std::shared_ptr<Actor>>& actors) 
: Actor(name, description, position, type, actors) {
	// create save file
#if BUILD_ENABLE_VULKAN_DEBUG
	std::cout << "Trigger area created\n";
#endif
}

TriggerArea::~TriggerArea() {
#if BUILD_ENABLE_VULKAN_DEBUG
	std::cout << "Trigger area destroyed\n";
#endif
}