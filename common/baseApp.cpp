#include "baseApp.hpp"

#include <iostream>

bool BaseApplication::init() {
	if (!glfwInit()) {
		std::cerr << "ERROR: Could not initialize GLFW\n";

		return false;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	window = glfwCreateWindow(window_width,
		window_height, title.c_str(), nullptr, nullptr);

	if (!window) {
		std::cerr << "ERROR: Could not create GLFW window\n";

		glfwTerminate();

		return false;
	}

	if (!centerWindow()) {
		std::cerr << "ERROR: Could not get window properties\n";

		glfwDestroyWindow(window);
		glfwTerminate();

		return false;
	}

	glfwSetWindowUserPointer(window, this);

	glfwMakeContextCurrent(window);
	
	if (!gl.load((GLADloadproc)glfwGetProcAddress)) {
		std::cerr << "ERROR: Could not initialize GLAD\n";

		glfwDestroyWindow(window);
		glfwTerminate();

		return false;
	}

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 450 core");

	if (!customInit()) {
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();

		return false;
	}

	return initialized = true;
}

void BaseApplication::destroy() {
	if (initialized) {
		customDestroy();

		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();

		glfwDestroyWindow(window);
		glfwTerminate();

		initialized = false;
	}
}

void BaseApplication::run() {
	double clock_now, clock_before = glfwGetTime(), delta_time;

	while (!glfwWindowShouldClose(window)) {
		bool fs = fullscreen;

		clock_now = glfwGetTime();
		delta_time = clock_now - clock_before;
		clock_before = clock_now;

		glfwPollEvents();

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		if (show_info) {
			ImGui::Begin(title.c_str());
			ImGui::Checkbox("Fullscreen", &fs);
			ImGui::Dummy(ImVec2(0.0f, 5.0f));
			ImGui::Separator();
			ImGui::Text("Delta Time: %.3f", delta_time);
			ImGui::End();
		}

		customLoop(delta_time);

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);

		setFullscreen(fs);
	}
}

bool BaseApplication::centerWindow() {
	GLFWmonitor* monitor = glfwGetPrimaryMonitor();

	if (!monitor)
		return false;

	GLFWvidmode const* mode = glfwGetVideoMode(monitor);

	if (!mode)
		return false;

	int monitor_x, monitor_y;
	glfwGetMonitorPos(monitor, &monitor_x, &monitor_y);

	glfwSetWindowPos(window, monitor_x + (mode->width - window_width) / 2,
		monitor_y + (mode->height - window_height) / 2);

	return true;
}

void BaseApplication::showInfo(bool show) { 
	show_info = show;
}

void BaseApplication::setWindowSize(int width, int height) {
	window_width = width;
	window_height = height;

	glfwSetWindowSize(window, width, height);
}

int BaseApplication::getWindowWidth() {
	return window_width;
}

int BaseApplication::getWindowHeight() {
	return window_height;
}

void BaseApplication::setFullscreen(bool fs) {
	if (fullscreen == fs)
		return;

	fullscreen = fs;

	int x, y;

	glfwGetWindowPos(window, &x, &y);

	GLFWmonitor* monitor = glfwGetPrimaryMonitor();

	if (!monitor) {
		std::cerr << "ERROR: Could not get primary monitor\n";
		abort();
	}

	GLFWvidmode const* mode = glfwGetVideoMode(monitor);

	if (!mode) {
		std::cerr << "ERROR: Could not get video mode\n";
		abort();
	}

	if (fullscreen)
		glfwSetWindowMonitor(window, monitor,
			0, 0, mode->width, mode->height, 0);
	else
		glfwSetWindowMonitor(window, nullptr,
			x + (mode->width - window_width) / 2,
			y + (mode->height - window_height) / 2,
			window_width, window_height, 0);
}

