#pragma once

#include <glm/glm.hpp>

//const std::string model_path = "models/cube.obj";
//const std::string texture_path = "textures/cube.jpg";

class Light
{
public:
	virtual ~Light() {};

	virtual void InitLight(float, float, float, float, float, float, float) = 0;
	virtual void DrawLight() = 0;
	void LoadModel();
	glm::vec3 GetColor() const;
	glm::vec4 GetPosition() const;
	void SetColor(glm::vec3);
	void SetPosition(glm::vec4);

	glm::vec4 light_position;
	glm::vec3 light_color;
};

class SkyMap : public Light
{
public:
	SkyMap();
	~SkyMap();

	virtual void InitLight(float, float, float, float, float, float, float);
	virtual void DrawLight();
};

class SphereLight : public Light
{
public:

	SphereLight();
	~SphereLight();

	virtual void InitLight(float, float, float, float, float, float, float);
	virtual void DrawLight();

};