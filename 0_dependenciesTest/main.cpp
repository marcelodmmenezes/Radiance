#include <imgui/imgui.h>
#include <imgui/examples/imgui_impl_glfw.h>
#include <imgui/examples/imgui_impl_opengl3.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <glad/glad.h>

#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include <cstdlib>
#include <iostream>

int main() {
	std::cout << "Running dependencies test...\n";

	if (!glfwInit()) {
		std::cerr << "ERROR: Could not initialize GLFW\n";

		return EXIT_FAILURE;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window =
		glfwCreateWindow(800, 600, "Dependencies Test", nullptr, nullptr);

	if (!window) {
		std::cerr << "ERROR: Could not create GLFW window\n";

		glfwTerminate();

		return EXIT_FAILURE;
	}

	glfwMakeContextCurrent(window);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cerr << "ERROR: Could not initialize GLAD\n";

		glfwDestroyWindow(window);
		glfwTerminate();

		return EXIT_FAILURE;
	}

	char const* glsl_version = "#version 450 core";
	glm::vec4 clear_c = { 0.2, 0.4, 0.25, 1.0 };

	glViewport(0, 0, 800, 600);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGui::StyleColorsDark();

	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init(glsl_version);

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		glClearColor(clear_c.r, clear_c.g, clear_c.b, clear_c.a);
		glClear(GL_COLOR_BUFFER_BIT);

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		{
			ImGui::Begin("ImGui Test");

			ImGui::ColorEdit4("Background Color", glm::value_ptr(clear_c));

			if (ImGui::Button("Close Dependencies Test"))
				glfwSetWindowShouldClose(window, 1);

			ImGui::End();
		}

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);
	}

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(window);
	glfwTerminate();

	std::cout << "Testing image loading...\n";

	int width, height, channels;

	stbi_set_flip_vertically_on_load(true);

	unsigned char* image = stbi_load("../res/checkers.png",
		&width, &height, &channels, STBI_rgb_alpha);

	if (!image) {
		std::cerr << "ERROR: Could not load res/checkers.png\n";
		return EXIT_FAILURE;
	}

	stbi_image_free(image);

	std::cout << "\nSuccess\n\n";

	return EXIT_SUCCESS;
}

