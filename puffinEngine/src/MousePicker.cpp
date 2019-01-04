#include <iostream>

#include "MousePicker.hpp"
#include "ErrorCheck.hpp"


// ------- Constructors and dectructors ------------- //

MousePicker::MousePicker() {
#if BUILD_ENABLE_VULKAN_DEBUG
	std::cout << "MousePicker created\n";
#endif
}

MousePicker::~MousePicker() {
#if BUILD_ENABLE_VULKAN_DEBUG
	std::cout << "MousePicker destroyed\n";
    currentCamera = nullptr;
#endif 
}

// ---------------- Main functions ------------------ //

glm::vec3 MousePicker::GetCurrentRay() const {
	return currentRay;
}

void MousePicker::UpdateMousePicker(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix, std::shared_ptr<Camera> camera) {
    view = viewMatrix;
    proj = projectionMatrix;
    currentCamera = camera;
    currentRay = CalculateMouseRay();
}

glm::vec3 MousePicker::CalculateMouseRay(){
    glm::vec4 mouseClipSpace = glm::vec4(mousePositionNormalized.x, mousePositionNormalized.y, -1.0f, 1.0f);
    glm::mat4 invertedProjection = glm::inverse(proj);
	glm::vec4 eyeCoords = invertedProjection * mouseClipSpace;
	glm::vec4 mouseEyeSpace = glm::vec4(eyeCoords.x, eyeCoords.y, -1.0f, 0.0f);
	glm::mat4 invertedView = glm::inverse(view);
	glm::vec4 rayWorld = invertedView * mouseEyeSpace;
	glm::vec3 mouseRay = glm::normalize(glm::vec3(rayWorld.x, rayWorld.y, rayWorld.z));
    std::cout << mouseRay.x << " " << mouseRay.y << " " << mouseRay.z << "\n";
	return mouseRay;
}

void MousePicker::GetNormalisedDeviceCoordinates(const double& xpos, const double& ypos, const int& HEIGHT, const int& WIDTH) noexcept {
    float x = (2.0f * xpos) / WIDTH - 1.0f;
	float y = 1.0f - (2.0f * ypos) / HEIGHT;
	mousePositionNormalized = glm::vec2(x, y);
}