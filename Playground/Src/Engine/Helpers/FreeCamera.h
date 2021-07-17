#pragma once

#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtc/matrix_transform.hpp"

enum class FreeCameraMovement
{
	FORWARD,
	BACK,
	LEFT,
	RIGHT
};

class FreeCamera
{
public:
	static FreeCamera& getInstance()
	{
		static FreeCamera camera;
		return camera;
	}

	~FreeCamera();

	void			Update(float dt);
	void			ProcessKeyDown(FreeCameraMovement mov);
	void			ProcessMouseMove(float xOffset, float yOffset, bool bConstraintPitch = true);
	void			ProcessMouseScroll(double yOffset);
public:
	glm::vec3		m_vecPosition;
	glm::mat4		m_matView;
	glm::mat4		m_matProjection;
	
private:
	FreeCamera();

	FreeCamera(const FreeCamera&);
	void operator=(const FreeCamera&);

	glm::vec3		m_vecForward;
	glm::vec3		m_vecUp;
	glm::vec3		m_vecRight;
	glm::vec3		m_vecAngle;

	float			m_fYaw;
	float			m_fPitch;

	float			m_fSpeed;
	float			m_fSensitivity;
	float			m_fFOV;
};

