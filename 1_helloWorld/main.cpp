#include "../common/baseApp.hpp"

#include <iostream>
#include <string>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

void windowResize(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);
}

class Application : public BaseApplication {
public:
	bool customInit() override {
		glfwSetWindowSizeCallback(window, windowResize);

		std::vector<ShaderInfo> shaders = {
			{
				GL_VERTEX_SHADER,
				"#version 450 core\n"
				""
				"layout (location = 0) in vec3 a_pos;"
				""
				"uniform mat4 u_transform;"
				""
				"void main() {"
				"	gl_Position = u_transform * vec4(a_pos, 1.0);"
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
			{ { -0.5f, -0.5f, 0.0f } },
			{ { 0.5f, -0.5f, 0.0f } },
			{ { 0.0f, 0.5f, 0.0f } }
		};

		std::vector<unsigned> indices {
			0, 1, 2
		};

		glCreateVertexArrays(1, &vao_id);
		glBindVertexArray(vao_id);

		glCreateBuffers(1, &vbo_id);

		glBindBuffer(GL_ARRAY_BUFFER, vbo_id);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex),
			vertices.data(), GL_STATIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
			0, (GLvoid*)offsetof(Vertex, pos));
		glEnableVertexAttribArray(0);

		glCreateBuffers(1, &ebo_id);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_id);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER,
			indices.size() * sizeof(unsigned),
			indices.data(), GL_STATIC_DRAW);

		glBindVertexArray(0);

		glClearColor(0.0, 0.2, 0.0, 1.0);
		glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);

		if (gl.checkErrors())
			return false;

		return true;
	}

	bool customLoop(double delta_time) override {
		if (gl.checkErrors())
			return false;

		{
			ImGui::Begin("Triangle properties");
			ImGui::ColorPicker4("Color", glm::value_ptr(color));
			ImGui::End();
		}

		glClear(GL_COLOR_BUFFER_BIT);

		glUseProgram(program_id);

		glUniformMatrix4fv(u_transform_loc, 1,
			GL_FALSE, glm::value_ptr(transform));
		glUniform4fv(u_color_loc, 1, glm::value_ptr(color));

		glBindVertexArray(vao_id);
		glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, nullptr);

		return true;
	}

	void customDestroy() override {
		glDeleteProgram(program_id);
		glDeleteBuffers(1, &ebo_id);
		glDeleteBuffers(1, &vbo_id);
		glDeleteVertexArrays(1, &vao_id);
	}

private:
	struct Vertex {
		glm::vec3 pos;
	};

	GLuint vao_id;
	GLuint vbo_id;
	GLuint ebo_id;
	glm::mat4 transform = glm::mat4(1.0);
	glm::vec4 color = glm::vec4(1.0);

	GLuint program_id;
	GLuint u_transform_loc;
	GLuint u_color_loc;
};

int main() {
	Application app;

	if (app.init("Hello, World", WINDOW_WIDTH, WINDOW_HEIGHT)) {
		app.run();
		app.destroy();
	}
}

