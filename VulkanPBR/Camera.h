#pragma once

#include "glm/glm.hpp"

class Camera
{
public:
	void Init(glm::vec3 inTargetPosition, float inDistanceFromTarget, glm::vec3 inViewDirection);
	void Update(glm::vec3 inTargetPosition, float inDistanceFromTarget, glm::vec3 inViewDirection);
	void RoundRotate(float inDeltaTime, glm::vec3 inTargetPosition, float inDistanceFromTarget, float inRotateSpeed);

	float m_distanceFromTarget;
	float m_rotateAngle;
	glm::vec3 m_initialViewDirection;
	glm::vec3 m_viewDirection;
	glm::vec3 m_position;
	glm::mat4 m_viewMatrix;
};
