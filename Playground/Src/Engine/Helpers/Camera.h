#pragma once

#include "glm/glm.hpp"
#include "glm/gtx/transform.hpp"
#include "glm/gtx/quaternion.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

// Based on https://github.com/hmazhar/moderngl_camera

enum class CameraDirection
{
    UP,
    DOWN,
    LEFT,
    RIGHT,
    FORWARD,
    BACK
};

class Camera
{
public:
    static Camera& getInstance()
    {
        static Camera instance;
        return instance;
    }

    ~Camera();

    void        Reset();
    void        Update(float dt);

    void        Move(CameraDirection dir);
    void        ChangePitch(float degrees);
    void        ChangeYaw(float degrees);

    void        Move2D(int x, int y);               // Change the Yaw-Pitch of the camera based on the 2D movement of the Mouse!

    int         m_iViewportX;
    int         m_iViewportY;

    int         m_iWindowWidth;
    int         m_iWindowHeight;

    float       m_fAspect;
    float       m_fFOV;
    float       m_fNearClip;
    float       m_fFarClip;             

    float       m_fCameraScale;
    float       m_fCameraYaw;
    float       m_fCameraPitch;

    float       m_fMaxPitchRate;
    float       m_fMaxYawRate;
    bool        m_bMoveCamera;

    glm::vec3   m_vecCameraPosition;
    glm::vec3   m_vecCameraPositionDelta;
    glm::vec3   m_vecCameraLookAt;
    glm::vec3   m_vecCameraDirection;
    glm::vec3   m_vecCameraUp;
                                              
    glm::vec3   m_vecMousePosition;

    glm::mat4   m_matProjection;
    glm::mat4   m_matView;
    glm::mat4   m_matModel;
    glm::mat4   m_matMVP;

private:
    Camera();
    Camera(const Camera&);
};

