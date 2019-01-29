#include <fstream>
#include <iostream>

#include "MainCharacter.hpp"

// ------- Constructors and dectructors ------------- //

MainCharacter::MainCharacter(std::string name, std::string description, glm::vec3 position, ActorType type) 
: Character(name, description, position, type) {
	// create save file
	std::cout << "MainCharacter created\n";
}

MainCharacter::~MainCharacter() {
	std::cout << "MainCharacter destroyed\n";
}