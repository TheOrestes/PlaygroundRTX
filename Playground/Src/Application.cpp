
#include "PlaygroundPCH.h"
#include "PlaygroundHeaders.h"
#include "Application.h"

#include "Engine/Helpers/Utility.h"
#include "Engine/Renderer/IRenderer.h"
#include "Engine/Renderer/VulkanRenderer.h"
#include "Engine/Renderer/RTXRenderer.h"
#include "Engine/Helpers/Camera.h"


//---------------------------------------------------------------------------------------------------------------------
Application::Application(const std::string& _title)
    :   m_strWindowTitle(_title)
{
    m_pWindow = nullptr;
    m_pRenderer = nullptr;

    m_uiWindowWidth = Helper::App::WINDOW_WIDTH;
    m_uiWindowHeight = Helper::App::WINDOW_HEIGHT;

    m_fDelta = 0.0f;
}

//---------------------------------------------------------------------------------------------------------------------
Application::~Application()
{
    Shutdown();
}

//---------------------------------------------------------------------------------------------------------------------
bool Application::Initialize()
{
    // Initialize GLFW
    if (!glfwInit())
    {
        LOG_CRITICAL("GLFW Initialization failed!");
        return false;
    }

    // GLFW Window parameters/settings
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    // Create Window
    m_pWindow = glfwCreateWindow(m_uiWindowWidth, m_uiWindowHeight, m_strWindowTitle.c_str(), nullptr, nullptr);
    
    // check if creation successful or not? 
    if (!m_pWindow)
    {
        LOG_CRITICAL("{0} Window of size ({1}, {2}) creation Failed!", m_strWindowTitle, m_uiWindowWidth, m_uiWindowHeight);
        return false;
    }
    else
    {
        LOG_INFO("{0} Window of size ({1}, {2}) created successfully!", m_strWindowTitle, m_uiWindowWidth, m_uiWindowHeight);
    }

    // Set 
    glfwSetWindowUserPointer(m_pWindow, this);

    // Register Events! 
    glfwSetWindowCloseCallback(m_pWindow, EventWindowClosedCallback);
    glfwSetWindowSizeCallback(m_pWindow, EventWindowResizedCallback);
    glfwSetKeyCallback(m_pWindow, EventKeyHandlerCallback);
    glfwSetCursorPosCallback(m_pWindow, EventMousePositionCallback);
    glfwSetMouseButtonCallback(m_pWindow, EventMouseButtonCallback);
    glfwSetScrollCallback(m_pWindow, EventMouseScrollCallback);

    // Create Vulkan Renderer
    m_pRenderer = new RTXRenderer();
    m_pRenderer->Initialize(m_pWindow);

    return true;
}

//---------------------------------------------------------------------------------------------------------------------
void Application::Run()
{
    if (Initialize())
    {
        MainLoop();
    }
}

//---------------------------------------------------------------------------------------------------------------------
void Application::MainLoop()
{
    float lastTime = 0.0f;

    while (!glfwWindowShouldClose(m_pWindow))
    {
        glfwPollEvents();

        float currTime = glfwGetTime();
        m_fDelta = currTime - lastTime;
        lastTime = currTime;

        Camera::getInstance().Update(m_fDelta);
        m_pRenderer->Update(m_fDelta);
        
        m_pRenderer->Render();
    }
}

//---------------------------------------------------------------------------------------------------------------------
void Application::Shutdown()
{
    glfwDestroyWindow(m_pWindow);

    m_pRenderer->Cleanup();
    SAFE_DELETE(m_pRenderer);
}

//---------------------------------------------------------------------------------------------------------------------
void Application::EventWindowClosedCallback(GLFWwindow* pWindow)
{
    LOG_DEBUG("Window Closed");
}

//---------------------------------------------------------------------------------------------------------------------
void Application::EventWindowResizedCallback(GLFWwindow* pWindow, int width, int height)
{
    LOG_DEBUG("Window Resized ({0},{1})", width, height);
}

//---------------------------------------------------------------------------------------------------------------------
void Application::EventKeyHandlerCallback(GLFWwindow* pWindow, int key, int scancode, int action, int mods)
{
    // Application close!
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(pWindow, true);
    }

    // Camera controls..
    if (key == GLFW_KEY_W && (action == GLFW_REPEAT || GLFW_PRESS))
    {
        Camera::getInstance().Move(CameraDirection::FORWARD);
    }

    if (key == GLFW_KEY_S && (action == GLFW_REPEAT || GLFW_PRESS))
    {
        Camera::getInstance().Move(CameraDirection::BACK);
    }

    if (key == GLFW_KEY_A && (action == GLFW_REPEAT || GLFW_PRESS))
    {
        Camera::getInstance().Move(CameraDirection::LEFT);
    }

    if (key == GLFW_KEY_D && (action == GLFW_REPEAT || GLFW_PRESS))
    {
        Camera::getInstance().Move(CameraDirection::RIGHT);
    }
}

//float lastX = Helper::App::WINDOW_WIDTH / 2.0f;
//float lastY = Helper::App::WINDOW_HEIGHT / 2.0f;
//---------------------------------------------------------------------------------------------------------------------
void Application::EventMousePositionCallback(GLFWwindow* pWindow, double xPos, double yPos)
{
    //float xOffset = xPos - lastX;
    //float yOffset = lastY - yPos;
    //
    //lastX = xPos;
    //lastY = yPos;

    // Rotate only when RIGHT CLICK is down!
    if (glfwGetMouseButton(pWindow, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)
    {
        Camera::getInstance().Move2D(xPos, yPos);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void Application::EventMouseButtonCallback(GLFWwindow* pWindow, int button, int action, int mods)
{
}

//---------------------------------------------------------------------------------------------------------------------
void Application::EventMouseScrollCallback(GLFWwindow* pWindow, double xOffset, double yOffset)
{
    //FreeCamera::getInstance().ProcessMouseScroll(yOffset);
}
