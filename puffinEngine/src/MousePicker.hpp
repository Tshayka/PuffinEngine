#pragma once

#include <glm/glm.hpp>

#include "Camera.hpp"

class MousePicker {
public:
	MousePicker();
	~MousePicker();

    glm::vec3 GetCurrentRay() const;
    void GetNormalisedDeviceCoordinates(const double& xpos, const double& ypos, const int& HEIGHT, const int& WIDTH) noexcept;
    void UpdateMousePicker(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix,std::shared_ptr<Camera> camera);

private:
    glm::vec3 currentRay;
    glm::vec2 mousePositionNormalized;
    glm::mat4 proj;
	glm::mat4 view;
    std::shared_ptr<Camera> currentCamera = nullptr;

    glm::vec3 CalculateMouseRay();
};