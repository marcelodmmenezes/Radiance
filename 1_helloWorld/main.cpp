#include "../common/baseApp.hpp"

#include <iostream>
#include <string>

#define WINDOW_WIDTH 1366
#define WINDOW_HEIGHT 768

void onKey(GLFWwindow* window, int key, int scancode, int action, int mods);
void windowResize(GLFWwindow* window, int width, int height);

class Application : public BaseApplication {
public:
	Application(
		std::string const& title = "Application",
		int window_width = 800,
		int window_height = 600,
		bool show_info = true,
		bool fullscreen = false)
		:
		BaseApplication(
			title,
			window_width,
			window_height,
			show_info,
			fullscreen) {}

	~Application() {}

	void up(bool state) {
		square_up = state;
	}

	void down(bool state) {
		square_down = state;
	}

	void left(bool state) {
		square_left = state;
	}

	void right(bool state) {
		square_right = state;
	}

	void setProjection(glm::mat4&& proj) {
		projection = proj;
	}

private:
	struct Vertex {
		glm::vec2 pos;
	};

	bool customInit() override {
		glfwSetWindowSizeCallback(window, windowResize);
		glfwSetKeyCallback(window, onKey);

		windowResize(window, window_width, window_height);

		std::vector<ShaderInfo> shaders = {
			{
				GL_VERTEX_SHADER,
				"#version 450 core\n"
				""
				"layout (location = 0) in vec2 a_pos;"
				""
				"uniform mat4 u_transform;"
				""
				"void main() {"
				"	gl_Position = u_transform * vec4(a_pos, 0.0, 1.0);"
				"}"
			},
			{
				GL_FRAGMENT_SHADER,
				"#version 450 core\n"
				""
				"uniform vec4 u_color;"
				""
				"out vec4 v_color;"
				""
				"void main() {"
				"	v_color = u_color;"
				"}"
			}
		};

		bool success;

		program_id = gl.createProgram(shaders, success);

		if (!success)
			return false;

		u_transform_loc = glGetUniformLocation(program_id, "u_transform");
		u_color_loc = glGetUniformLocation(program_id, "u_color");

		std::vector<Vertex> vertices {
			{ { -1.0f, -1.0f } },
			{ { 1.0f, -1.0f } },
			{ { 1.0f, 1.0f } },
			{ { -1.0f, 1.0f } }
		};

		std::vector<unsigned> indices {
			0, 1, 2, 0, 2, 3
		};

		glCreateVertexArrays(1, &vao_id);
		glBindVertexArray(vao_id);

		glCreateBuffers(1, &vbo_id);

		glBindBuffer(GL_ARRAY_BUFFER, vbo_id);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex),
			vertices.data(), GL_STATIC_DRAW);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE,
			0, (GLvoid*)offsetof(Vertex, pos));
		glEnableVertexAttribArray(0);

		glCreateBuffers(1, &ebo_id);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_id);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER,
			indices.size() * sizeof(unsigned),
			indices.data(), GL_STATIC_DRAW);

		glBindVertexArray(0);

		glClearColor(0.10, 0.25, 0.15, 1.0);

		if (gl.checkErrors())
			return false;

		return true;
	}

	bool customLoop(double delta_time) override {
		if (gl.checkErrors())
			return false;

		{
			ImGui::Begin("Square properties");
			ImGui::ColorPicker4("Color", glm::value_ptr(square_color));
			ImGui::Dummy(ImVec2(0.0f, 5.0f));
			ImGui::Separator();
			ImGui::Text("Set square position");
			ImGui::Dummy(ImVec2(0.0f, 2.0f));
			ImGui::SliderFloat("X", &square_pos.x, -1.0f, 1.0f);
			ImGui::SliderFloat("Y", &square_pos.y, -1.0f, 1.0f);
			ImGui::SliderFloat("Z", &square_pos.z, 0.0f, -10.0f);
			ImGui::Dummy(ImVec2(0.0f, 5.0f));
			ImGui::Separator();
			ImGui::Text("Use the W, A, S, D keys to move the square");
			ImGui::Dummy(ImVec2(0.0f, 2.0f));
			ImGui::SliderFloat("Velocity", &square_velocity, 1.0f, 10.0f);
			ImGui::End();
		}

		glClear(GL_COLOR_BUFFER_BIT);

		glUseProgram(program_id);

		if (square_up)
			square_pos.y += delta_time * square_velocity;
		else if (square_down)
			square_pos.y -= delta_time * square_velocity;

		if (square_left)
			square_pos.x -= delta_time * square_velocity;
		else if (square_right)
			square_pos.x += delta_time * square_velocity;

		glm::mat4 transform = 
			projection * glm::translate(glm::mat4(1.0), square_pos);

		glUniformMatrix4fv(u_transform_loc, 1,
			GL_FALSE, glm::value_ptr(transform));
		glUniform4fv(u_color_loc, 1, glm::value_ptr(square_color));

		glBindVertexArray(vao_id);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

		return true;
	}

	void customDestroy() override {
		glDeleteProgram(program_id);
		glDeleteBuffers(1, &ebo_id);
		glDeleteBuffers(1, &vbo_id);
		glDeleteVertexArrays(1, &vao_id);
	}

	GLuint vao_id;
	GLuint vbo_id;
	GLuint ebo_id;

	bool square_up = false;
	bool square_left = false;
	bool square_down = false;
	bool square_right = false;
	glm::vec3 square_pos = glm::vec3(0.0f, 0.0f, -5.0f);
	float square_velocity = 6.0f;
	glm::vec4 square_color = glm::vec4(1.0f);

	GLuint program_id;
	GLuint u_transform_loc;
	GLuint u_color_loc;

	glm::mat4 projection;
};

void onKey(GLFWwindow* window, int key, int scancode, int action, int mods) {
	auto app = static_cast<Application*>(glfwGetWindowUserPointer(window));

	switch (key) {
		case GLFW_KEY_W:
			app->up(action != GLFW_RELEASE);
			break;

		case GLFW_KEY_A:
			app->left(action != GLFW_RELEASE);
			break;

		case GLFW_KEY_S:
			app->down(action != GLFW_RELEASE);
			break;

		case GLFW_KEY_D:
			app->right(action != GLFW_RELEASE);
			break;
	}
}

void windowResize(GLFWwindow* window, int width, int height) {
	auto app = static_cast<Application*>(glfwGetWindowUserPointer(window));

	app->setProjection(glm::perspective(
		glm::radians(60.0f), (float)width / height, 0.1f, 100.0f));

	glViewport(0, 0, width, height);
}

int main() {
	Application app("Hello, World", WINDOW_WIDTH, WINDOW_HEIGHT);

	if (app.init()) {
		app.run();
		app.destroy();
	}
}

