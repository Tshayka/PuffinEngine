#include <fstream>
#include <iostream>

#include "headers/MainCharacter.hpp"

// ------- Constructors and dectructors ------------- //

MainCharacter::MainCharacter(std::string name, std::string description, glm::vec3 position, ActorType type,  std::vector<std::shared_ptr<Actor>>& actors ) 
: Character(name, description, position, type, actors) {
	// create save file
	std::cout << "MainCharacter created\n";
}

MainCharacter::~MainCharacter() {
	std::cout << "MainCharacter destroyed\n";
}