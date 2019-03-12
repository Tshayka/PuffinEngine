#include <fstream>
#include <iostream>

#include "MainCharacter.hpp"

// ------- Constructors and dectructors ------------- //

MainCharacter::MainCharacter(std::string name, std::string description, glm::vec3 position, ActorType type,  std::vector<std::shared_ptr<Actor>>& actors ) 
: Character(name, description, position, type, actors) {
	// create save file
#if BUILD_ENABLE_VULKAN_DEBUG
	std::cout << "MainCharacter created\n";
#endif
}

MainCharacter::~MainCharacter() {
#if BUILD_ENABLE_VULKAN_DEBUG
	std::cout << "MainCharacter destroyed\n";
#endif
}