#ifndef BASE_APP_HPP
#define BASE_APP_HPP

#include "glContext.hpp"

#include <imgui/imgui.h>
#include <imgui/examples/imgui_impl_glfw.h>
#include <imgui/examples/imgui_impl_opengl3.h>

#include <GLFW/glfw3.h>

#include <string>

class BaseApplication {
public:
	BaseApplication() {}
	virtual ~BaseApplication() {}

	bool init(
		std::string const& title = "BaseApplication",
		int window_width = 800,
		int window_height = 600,
		bool show_info = true,
		bool fullscreen = false);

	void destroy();
	void run();
	void showInfo(bool show);

	virtual bool customInit() = 0;
	virtual bool customLoop(double delta_time) = 0;
	virtual void customDestroy() = 0;

protected:
	bool initialized = false;

	GLFWwindow* window;
	std::string title;

	bool show_info;
	bool fullscreen;

	OpenGLContext gl;
};

#endif // BASE_APP_HPP

