#include "PlaygroundPCH.h"
#include "Engine/Helpers/Utility.h"
#include "Camera.h"

//---------------------------------------------------------------------------------------------------------------------
Camera::Camera()
{
    m_bMoveCamera = true;
    
    m_vecCameraUp = glm::vec3(0,1,0);
    m_fFOV = 45.0f;
    m_fAspect = Helper::App::WINDOW_WIDTH / Helper::App::WINDOW_HEIGHT;
    m_fNearClip = 0.01f;
    m_fFarClip = 10000.0f;
    m_vecCameraPositionDelta = glm::vec3(0);
    m_fCameraScale = 0.5f;
    m_fMaxPitchRate = 1.0f;
    m_fMaxYawRate = 1.0f;

    m_vecCameraPosition = glm::vec3(0,5,25);
    m_vecCameraLookAt = glm::vec3(0);
}

//---------------------------------------------------------------------------------------------------------------------
Camera::~Camera()
{
}

//---------------------------------------------------------------------------------------------------------------------
void Camera::Reset()
{
    m_vecCameraUp = glm::vec3(0,1,0);
}

//---------------------------------------------------------------------------------------------------------------------
void Camera::Update(float dt)
{
    m_vecCameraDirection = glm::normalize(m_vecCameraLookAt - m_vecCameraPosition);

    m_matProjection = glm::perspective(m_fFOV, m_fAspect, m_fNearClip, m_fFarClip);

    // determine axis for pitch rotation
    glm::vec3 pitchAxis = glm::cross(m_vecCameraDirection, m_vecCameraUp);

    // compute quaternion for pitch based on camera pitch angle
    glm::quat pitchQuat = glm::angleAxis(m_fCameraPitch, pitchAxis);

    // compute quaternion for yaw based on yaw angle & camera up vector
    glm::quat yawQuat = glm::angleAxis(m_fCameraYaw, m_vecCameraUp);

    // Add the two quaternions
    glm::quat temp = glm::normalize(glm::cross(pitchQuat, yawQuat));

    // update the direction from quaternion
    m_vecCameraDirection = glm::rotate(temp, m_vecCameraDirection);

    // Add the camera delta
    m_vecCameraPosition += m_vecCameraPositionDelta;

    // Set the lookAt to be at the front of camera
    m_vecCameraLookAt = m_vecCameraPosition + m_vecCameraDirection * 1.0f;

    // Damping for smooth camera
    m_fCameraYaw *= dt;
    m_fCameraPitch *= dt;
    m_vecCameraPositionDelta = m_vecCameraPositionDelta * dt;

    // Compute MVP
    m_matView = glm::lookAt(m_vecCameraPosition, m_vecCameraLookAt, m_vecCameraUp);
    m_matModel = glm::mat4(1);
    m_matMVP = m_matProjection * m_matView * m_matModel;
}

//---------------------------------------------------------------------------------------------------------------------
void Camera::Move(CameraDirection dir)
{
    switch (dir)
    {
        case CameraDirection::UP:
        {
            m_vecCameraPositionDelta += m_vecCameraUp * m_fCameraScale;
            break;
        }
            
        case CameraDirection::DOWN:
        {
            m_vecCameraPositionDelta -= m_vecCameraUp * m_fCameraScale;
            break;
        }
            
        case CameraDirection::LEFT:
        {
            m_vecCameraPositionDelta -= glm::normalize(glm::cross(m_vecCameraDirection, m_vecCameraUp)) * m_fCameraScale;
            break;
        }
            
        case CameraDirection::RIGHT:
        {
            m_vecCameraPositionDelta += glm::normalize(glm::cross(m_vecCameraDirection, m_vecCameraUp)) * m_fCameraScale;
            break;
        }

        case CameraDirection::FORWARD:
        {
            m_vecCameraPositionDelta += m_vecCameraDirection * m_fCameraScale;        
            break;
        }
            
        case CameraDirection::BACK:
        {
            m_vecCameraPositionDelta -= m_vecCameraDirection * m_fCameraScale;
            break;
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void Camera::ChangePitch(float degrees)
{
    //Check bounds with the max pitch rate so that we aren't moving too fast
    if (degrees < -m_fMaxPitchRate) 
    {
        degrees = -m_fMaxPitchRate;
    }
    else if (degrees > m_fMaxPitchRate) 
    {
        degrees = m_fMaxPitchRate;
    }

    m_fCameraPitch += degrees;

    //Check bounds for the camera pitch
    if (m_fCameraPitch > 360.0f) 
    {
        m_fCameraPitch -= 360.0f;
    }
    else if (m_fCameraPitch < -360.0f)
    {
        m_fCameraPitch += 360.0f;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void Camera::ChangeYaw(float degrees)
{
    //Check bounds with the max heading rate so that we aren't moving too fast
    if (degrees < -m_fMaxYawRate) 
    {
        degrees = -m_fMaxYawRate;
    }
    else if (degrees > m_fMaxYawRate)
    {
        degrees = m_fMaxYawRate;
    }
    //This controls how the heading is changed if the camera is pointed straight up or down
    //The heading delta direction changes
    if (m_fCameraPitch > 90 && m_fCameraPitch < 270 || (m_fCameraPitch < -90 && m_fCameraPitch > -270))
    {
        m_fCameraYaw -= degrees;
    }
    else 
    {
        m_fCameraYaw += degrees;
    }
    //Check bounds for the camera heading
    if (m_fCameraYaw > 360.0f)
    {
        m_fCameraYaw -= 360.0f;
    }
    else if (m_fCameraYaw < -360.0f)
    {
        m_fCameraYaw += 360.0f;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void Camera::Move2D(int x, int y)
{
    //compute the mouse delta from the previous mouse position
    glm::vec3 mouse_delta = m_vecMousePosition - glm::vec3(x, y, 0);

    //if the camera is moving, meaning that the mouse was clicked and dragged, change the pitch and heading
    if (m_bMoveCamera) 
    {
        ChangeYaw(.008f * mouse_delta.x);
        ChangePitch(.008f * mouse_delta.y);
    }

    m_vecMousePosition = glm::vec3(x, y, 0);
}
