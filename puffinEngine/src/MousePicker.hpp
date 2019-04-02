#pragma once

#include <glm/glm.hpp>

#include "Camera.hpp"

class MousePicker {
public:
	MousePicker();
	~MousePicker();

    void DeInit();
    glm::vec3 GetRayDirection() const;
    glm::vec3 GetRayOrigin() const;
    void Init();
    void CalculateNormalisedDeviceCoordinates(const double& xpos, const double& ypos) noexcept;
    void UpdateMousePicker(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix,std::shared_ptr<Camera> camera);

    glm::vec3 hitPoint;
    float width, height;

private:
    glm::vec3 currentRay;
    glm::vec2 mousePositionNormalized;
    glm::mat4 proj;
	glm::mat4 view;
    std::shared_ptr<Camera> currentCamera = nullptr;

    glm::vec3 CalculateMouseRay();
};