#include "PlaygroundPCH.h"
#include "FreeCamera.h"

#include "Engine/Helpers/Utility.h"

//---------------------------------------------------------------------------------------------------------------------
FreeCamera::FreeCamera()
{
	m_fFOV = 45.0f;
	m_fPitch = 0.0f;						// camera pitch angle
	m_fYaw = -90.0f;						// camera yaw angle
	m_fSensitivity = 0.05f;					// camera pan sensitivity
	m_fSpeed = 1.0f;						// camera movement speed
	m_vecPosition = glm::vec3(0, 5, 25);	// camera position
	m_vecForward = glm::vec3(0, 5, 26);		// camera forward
	m_vecRight = glm::vec3(1, 0, 0);		// camera right
	m_vecUp = glm::vec3(0, 1, 0);			// camera up
}

//---------------------------------------------------------------------------------------------------------------------
FreeCamera::~FreeCamera()
{

}

//---------------------------------------------------------------------------------------------------------------------
void FreeCamera::Update(float dt)
{
	// calculate new lookAt vector based on updated Yaw & Pitch values!
	m_vecForward.x = glm::cos(glm::radians(m_fYaw)) * glm::cos(glm::radians(m_fPitch));
	m_vecForward.y = glm::sin(glm::radians(m_fPitch));
	m_vecForward.z = glm::sin(glm::radians(m_fYaw)) * glm::cos(glm::radians(m_fPitch));

	m_vecForward = glm::normalize(m_vecForward);

	// calculate new Up & Right vector!
	m_vecRight = glm::normalize(glm::cross(m_vecForward, m_vecUp));
	m_vecUp = glm::normalize(glm::cross(m_vecRight, m_vecForward));

	// Update View & Projection matrices!
	m_matView = glm::lookAt(m_vecPosition, m_vecPosition + m_vecForward, m_vecUp);
	m_matProjection = glm::perspective(m_fFOV, Helper::App::WINDOW_WIDTH / Helper::App::WINDOW_HEIGHT, 0.1f, 1000.0f);
}

//---------------------------------------------------------------------------------------------------------------------
void FreeCamera::ProcessKeyDown(FreeCameraMovement mov)
{
	switch (mov)
	{
		case FreeCameraMovement::FORWARD:
		{
			m_vecPosition += m_vecForward * m_fSpeed;
			break;
		}
			
		case FreeCameraMovement::BACK:
		{
			m_vecPosition -= m_vecForward * m_fSpeed;
			break;
		}
			
		case FreeCameraMovement::LEFT:
		{
			m_vecPosition -= m_vecRight * m_fSpeed;
			break;
		}
			
		case FreeCameraMovement::RIGHT:
		{
			m_vecPosition += m_vecRight * m_fSpeed;
			break;
		}
	}
}

//---------------------------------------------------------------------------------------------------------------------
void FreeCamera::ProcessMouseMove(float xOffset, float yOffset, bool bConstraintPitch /*= true*/)
{
	xOffset *= m_fSensitivity;
	yOffset *= m_fSensitivity;

	m_fYaw += xOffset;
	m_fPitch += yOffset;

	if (bConstraintPitch)
	{
		if (m_fPitch > 89.0f)
			m_fPitch = 89.0f;

		if (m_fPitch < -89.0f)
			m_fPitch = -89.0f;
	}
}

//---------------------------------------------------------------------------------------------------------------------
void FreeCamera::ProcessMouseScroll(double yOffset)
{
	m_fFOV -= 0.05f * (float)yOffset;
	
	if (m_fFOV < 1.0f)
		m_fFOV = 1.0f;

	if (m_fFOV > 45.0f)
		m_fFOV = 45.0f;
}




