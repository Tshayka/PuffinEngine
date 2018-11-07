#include <iostream>
#include "Light.hpp"
#include "ErrorCheck.hpp"

// ------- Constructors and dectructors ------------- //

Light::Light(std::string name, std::string description, glm::vec3 position) 
: Actor(name, description, position) {
#if BUILD_ENABLE_VULKAN_DEBUG
	std::cout << "Light created\n";
#endif
}

Light::~Light() {
#if BUILD_ENABLE_VULKAN_DEBUG
	std::cout << "Light destroyed\n";
#endif
}

SphereLight::SphereLight(std::string name, std::string description, glm::vec3 position) 
: Light(name, description, position) {
#if BUILD_ENABLE_VULKAN_DEBUG
	std::cout << "Sphere light created\n";
#endif
}

SphereLight::~SphereLight() {
#if BUILD_ENABLE_VULKAN_DEBUG
	std::cout << "Sphere light destroyed\n";
#endif
}

// --------------- Setters and getters -------------- //

ActorType SphereLight::GetType() {
    return type;
}

glm::vec3 Light::GetLightColor() const {
	return lightColor;
}

void Light::SetLightColor(glm::vec3 lightColor) {
	this->lightColor = lightColor;
}
