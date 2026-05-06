#include "Camera.h"

#include "glm/ext.hpp"

void Camera::Init(glm::vec3 inTargetPosition, float inDistanceFromTarget, glm::vec3 inViewDirection)
{
	inViewDirection = glm::normalize(inViewDirection);
	m_initialViewDirection = inViewDirection;
	m_distanceFromTarget = inDistanceFromTarget;
	m_rotateAngle = 0.0;
	Update(inTargetPosition, inDistanceFromTarget, inViewDirection);
}

void Camera::Update(glm::vec3 inTargetPosition, float inDistanceFromTarget, glm::vec3 inViewDirection)
{
	glm::vec3 cameraPosition = inTargetPosition - inViewDirection * inDistanceFromTarget;
	glm::vec3 rightDirection = glm::cross(inViewDirection, glm::vec3(0.0f, 1.0f, 0.0f));
	glm::vec3 upDirection = glm::normalize(glm::cross(rightDirection, inViewDirection));
	m_position = cameraPosition;
	m_viewDirection = inViewDirection;
	m_viewMatrix = glm::lookAt(cameraPosition, inTargetPosition, upDirection);
}

void Camera::RoundRotate(float inDeltaTime, glm::vec3 inTargetPosition, float inDistanceFromTarget, float inRotateSpeed)
{
	float newAngle = m_rotateAngle + inRotateSpeed * inDeltaTime;
	m_rotateAngle = newAngle;
	glm::mat4 rotateMatrix;
	rotateMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(newAngle), glm::vec3(0.0f, 1.0f, 0.0f));
	glm::vec3 inserseInitialViewDirection = -m_initialViewDirection;
	glm::vec3 inverseViewDirection = glm::vec3(rotateMatrix * glm::vec4(inserseInitialViewDirection, 0.0f));
	inverseViewDirection = glm::normalize(inverseViewDirection);

	glm::vec3 viewDirection = -inverseViewDirection;
	Update(inTargetPosition, inDistanceFromTarget, viewDirection);
}