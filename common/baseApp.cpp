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
	glfwWindowHint(GLFW_SAMPLES, 4);

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

	setFullscreen(fullscreen);

	glfwMakeContextCurrent(window);
	
	if (!gl.load((GLADloadproc)glfwGetProcAddress)) {
		std::cerr << "ERROR: Could not initialize GLAD\n";

		glfwDestroyWindow(window);
		glfwTerminate();

		return false;
	}

	glfwSwapInterval(0); // Vsync
	glfwSwapInterval(1); // Disabling and enabling fixes glfw stuttering

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
		bool vs = vsync;
		bool wireframe = gl_wireframe;
		float line_width = gl_line_width;

		clock_now = glfwGetTime();
		delta_time = clock_now - clock_before;
		clock_before = clock_now;

		glfwPollEvents();

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		if (show_info) {
			ImGui::Begin(title.c_str());

			if (ImGui::Button("Close"))
				close();

			ImGui::Checkbox("Fullscreen", &fs);
			ImGui::Checkbox("V Sync", &vs);
			ImGui::Checkbox("Wireframe", &wireframe);
			ImGui::SliderFloat("Line width", &line_width, 0.5f, 10.0f);
			ImGui::Dummy(ImVec2(0.0f, 5.0f));

			ImGui::Separator();

			ImGui::Text("Delta Time: %.3f", delta_time);

			ImGui::End();
		}

		if (!customLoop(delta_time))
			close();

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);

		if (fs != fullscreen)
			setFullscreen(fs);

		if (vs != vsync) {
			glfwSwapInterval(vs);
			vsync = vs;
		}

		if (wireframe != gl_wireframe) {
			gl_wireframe = wireframe;
			gl.setWireframe(gl_wireframe);
		}

		if (line_width != gl_line_width) {
			gl_line_width = line_width;
			gl.setLineWidth(gl_line_width);
		}
	}
}

void BaseApplication::close() {
	glfwSetWindowShouldClose(window, 1);
}

void BaseApplication::showInfo(bool show) { 
	show_info = show;
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

