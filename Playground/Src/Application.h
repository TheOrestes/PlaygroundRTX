#pragma once

#include "PlaygroundPCH.h"

#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"

class IRenderer;

class Application
{
public:
	Application() {};
	Application(const std::string& _title);
		
	~Application();

	bool			Initialize();
	void			Run();
	void			Shutdown();

	//-- EVENTS
	static void		EventWindowClosedCallback(GLFWwindow* pWindow);
	static void		EventWindowResizedCallback(GLFWwindow* pWindow, int width, int height);
	static void		EventKeyHandlerCallback(GLFWwindow* pWindow, int key, int scancode, int action, int mods);
	static void		EventMousePositionCallback(GLFWwindow* pWindow, double xPos, double yPos);
	static void		EventMouseButtonCallback(GLFWwindow* pWindow, int button, int action, int mods);
	static void		EventMouseScrollCallback(GLFWwindow* pWindow, double xOffset, double yOffset);

private:
	GLFWwindow*		m_pWindow;
	IRenderer*		m_pRenderer;

	uint16_t		m_uiWindowWidth;
	uint16_t		m_uiWindowHeight;
	std::string		m_strWindowTitle;

	float			m_fDelta;

	void			MainLoop();
};
