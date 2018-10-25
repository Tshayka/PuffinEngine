#pragma once

#include <glm/glm.hpp>

class Camera
{
public:
	Camera();
	~Camera();

	void InitCamera(float, float, float, float, float, float, float, float, float, float, float, float, float, float, float, float, float, float, float, float);
	void DollyCamera(float);
	void GamepadMove(double, double, int, int, float);
	void MouseMove(double, double, int, int, float);
	void PedestalCamera(float);
	void ResetPosition();
	void TruckCamera(float);
	void UpdateCamera(float);

	glm::vec3 cam_pos;
	glm::vec3 cam_up;
	glm::vec3 cam_view;

	float cam_FOV;
	float cam_clip_near;
	float cam_clip_far;

private:
	float CamApproach(float, float, float);

	glm::vec3 cam_movement;
	glm::vec3 cam_movement_goal;
	glm::vec3 cam_veloc;
	glm::vec3 cam_dir;
	glm::vec3 foward;
	
	float pitch;
	float yaw;
};