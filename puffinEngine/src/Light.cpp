#include <iostream>
#include "Light.hpp"

// ------- Constructors and dectructors ------------- //

Light::Light(std::string name, std::string description, glm::vec3 position) 
: Actor(name, description, position) {
	lightPosition = glm::vec4(glm::vec3(position), alpha);
	std::cout << "Light created\n";
}

Light::~Light() {
	std::cout << "Light destroyed\n";
}

SphereLight::SphereLight(std::string name, std::string description, glm::vec3 position) 
: Light(name, description, position) {
	std::cout << "Sphere light created\n";
}

SphereLight::~SphereLight() {
	std::cout << "Sphere light destroyed\n";
}

// --------------- Setters and getters -------------- //

ActorType SphereLight::GetType() {
    return type;
}

glm::vec3 Light::GetLightColor() const {
	return lightColor;
}

glm::vec4 Light::GetLightPosition() const {
	return lightPosition;
}

void Light::SetLightColor(glm::vec3 lightColor) {
	this->lightColor = lightColor;
}
void Light::SetLightPosition(glm::vec4 lightPosition) {
	this->lightPosition = lightPosition;
}