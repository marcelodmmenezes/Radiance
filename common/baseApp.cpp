#include "baseApp.hpp"

#include <iostream>

bool BaseApplication::init(
	std::string const& title,
	int window_width,
	int window_height,
	bool show_info,
	bool fullscreen) {

	this->title = title;
	this->show_info = show_info;
	this->fullscreen = fullscreen;

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
		clock_now = glfwGetTime();
		delta_time = clock_now - clock_before;
		clock_before = clock_now;

		glfwPollEvents();

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		if (show_info) {
			ImGui::Begin(title.c_str());
			ImGui::Checkbox("Fullscreen", &fullscreen);
			ImGui::Text("Delta Time: %f", delta_time);
			ImGui::End();
		}

		customLoop(delta_time);

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);
	}
}

void BaseApplication::showInfo(bool show) { 
	show_info = show;
}

