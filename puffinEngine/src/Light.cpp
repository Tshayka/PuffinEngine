#include <iostream>
#include "Light.hpp"


// ------- Constructors and dectructors ------------- //

SkyMap::SkyMap()
{
	std::cout << "Skymap created\n";
}

SkyMap::~SkyMap()
{
	std::cout << "Skymap removed\n";
}

SphereLight::SphereLight()
{ 
	std::cout << "Sphere Light created\n";
}

SphereLight::~SphereLight()
{
	std::cout << "Sphere Light removed\n";
}

// --------------- Setters and getters -------------- //

glm::vec3 Light::GetColor() const
{
	return light_color;
}

glm::vec4 Light::GetPosition() const
{
	return light_position;
}

void Light::SetColor(glm::vec3 light_color) 
{
	this->light_color = light_color;
}

void Light::SetPosition(glm::vec4 light_position)
{
	this->light_position = light_position;
}

// ---------------- Main functions ------------------ //

void SkyMap::InitLight(float pos_x, float pos_y, float pos_z, float alpha, float r, float g, float b)
{
	light_position = glm::vec4(pos_x, pos_y, pos_z, alpha);
	light_color = glm::vec3(r, g, b);
}

void SphereLight::InitLight(float pos_x, float pos_y, float pos_z, float alpha, float r, float g, float b)
{
	light_position = glm::vec4(pos_x, pos_y, pos_z, alpha);
	light_color = glm::vec3(r, g, b);
}

void SkyMap::DrawLight()
{

}

void SphereLight::DrawLight()
{

}

void Light::LoadModel()
{
}