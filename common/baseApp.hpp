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
	BaseApplication(
		std::string const& title,
		int window_width,
		int window_height,
		bool show_info,
		bool fullscreen)
		:
		title{ title },
		window_width{ window_width },
		window_height{ window_height },
		show_info{ show_info },
		fullscreen{ fullscreen } {}

	virtual ~BaseApplication() {}

	bool init();
	void destroy();
	void run();

	void showInfo(bool show);

	bool centerWindow();
	void setWindowSize(int width, int height);
	int getWindowWidth();
	int getWindowHeight();

	void setFullscreen(bool fs);

protected:
	virtual bool customInit() = 0;
	virtual bool customLoop(double delta_time) = 0;
	virtual void customDestroy() = 0;

	bool initialized = false;

	GLFWwindow* window;
	std::string title;
	int window_width;
	int window_height;

	bool show_info;
	bool fullscreen;

	OpenGLContext gl;
};

#endif // BASE_APP_HPP

