#include "../common/baseApp.hpp"

#include <iostream>
#include <string>

#define WINDOW_WIDTH 1366
#define WINDOW_HEIGHT 768

void onKey(GLFWwindow* window, int key, int scancode, int action, int mods);
void onMouseScroll(GLFWwindow* window, double x_offset, double y_offset);
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

	void moveUp(bool state) {
		up = state;
	}

	void moveDown(bool state) {
		down = state;
	}

	void moveLeft(bool state) {
		left = state;
	}

	void moveRight(bool state) {
		right = state;
	}

	void moveZ(double y_offset) {
		forward = y_offset;
	}

	void setProjection(glm::mat4&& proj) {
		projection = proj;
	}

private:
	struct Vertex {
		glm::vec2 pos;
		glm::vec2 tex;
	};

	bool customInit() override {
		glfwSetKeyCallback(window, onKey);
		glfwSetScrollCallback(window, onMouseScroll);
		glfwSetWindowSizeCallback(window, windowResize);

		windowResize(window, window_width, window_height);

		if (!createProgram())
			return false;

		createTexture();
		createGeometry();

		glClearColor(0.10, 0.25, 0.15, 1.0);

		if (gl.checkErrors())
			return false;

		return true;
	}

	bool customLoop(double delta_time) override {
		if (gl.checkErrors())
			return false;

		buildGUI();

		glClear(GL_COLOR_BUFFER_BIT);

		glUseProgram(program_id);

		adjustTextureProperties();
		moveSquare(delta_time);

		glm::mat4 transform = 
			projection *
			glm::translate(glm::mat4(1.0), square_pos) *
			glm::rotate(glm::mat4(1.0),
				glm::radians(square_angle), glm::vec3(0.0f, 0.0f, 1.0f));

		glUniformMatrix4fv(u_transform_loc, 1,
			GL_FALSE, glm::value_ptr(transform));
		glUniform1f(u_angle_loc, glm::radians(texture_angle));
		glUniform4fv(u_color_loc, 1, glm::value_ptr(square_color));
		glUniform1i(u_has_color_loc, color_on);
		glUniform1i(u_has_texture_loc, texture_on);

		glBindVertexArray(vao_id);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

		return true;
	}

	void customDestroy() override {
		glDeleteProgram(program_id);
		glDeleteTextures(1, &texture_id);
		glDeleteBuffers(1, &ebo_id);
		glDeleteBuffers(1, &vbo_id);
		glDeleteVertexArrays(1, &vao_id);
	}

	void buildGUI() {
		using namespace ImGui;

		Begin("Square properties");

		Checkbox("Has texture", &texture_on);
		Checkbox("Has color", &color_on);
		ColorPicker4("Color", glm::value_ptr(square_color));
		Dummy(ImVec2(0.0f, 5.0f));

		Separator();
		Text("Set square position");
		Dummy(ImVec2(0.0f, 2.0f));
		SliderFloat("X", &square_pos.x, -1.0f, 1.0f);
		SliderFloat("Y", &square_pos.y, -1.0f, 1.0f);
		SliderFloat("Z", &square_pos.z, 0.0f, -10.0f);
		Dummy(ImVec2(0.0f, 5.0f));

		Separator();
		Text("Set square rotation");
		SliderFloat("Angle", &square_angle, -90.0f, 90.0f);
		Dummy(ImVec2(0.0f, 5.0f));

		Separator();
		Text("Use the W, A, S, D keys to move the square");
		Dummy(ImVec2(0.0f, 2.0f));
		SliderFloat("Velocity", &square_velocity, 1.0f, 10.0f);

		End();

		Begin("Texture properties");

		Text("Coordinates rotation");
		SliderFloat("Angle", &texture_angle, -90.0f, 90.0f);
		Dummy(ImVec2(0.0f, 5.0f));

		Separator();
		Text("Filtering");
		BeginGroup();
		RadioButton("Nearest", &texture_filtering, 0);
		SameLine();
		RadioButton("Linear", &texture_filtering, 1);
		EndGroup();
		Dummy(ImVec2(0.0f, 5.0f));

		Separator();
		Text("Wrapping");
		Dummy(ImVec2(0.0f, 2.0f));
		Text("S");
		BeginGroup();
		RadioButton("S Clamp to edge", &texture_wrap_s, 0);
		RadioButton("S Clamp to border", &texture_wrap_s, 1);
		RadioButton("S Mirrored repeat", &texture_wrap_s, 2);
		RadioButton("S Repeat", &texture_wrap_s, 3);
		RadioButton("S Mirror clamp to edge", &texture_wrap_s, 4);
		EndGroup();
		Dummy(ImVec2(0.0f, 2.0f));
		Text("T");
		BeginGroup();
		RadioButton("T Clamp to edge", &texture_wrap_t, 0);
		RadioButton("T Clamp to border", &texture_wrap_t, 1);
		RadioButton("T Mirrored repeat", &texture_wrap_t, 2);
		RadioButton("T Repeat", &texture_wrap_t, 3);
		RadioButton("T Mirror clamp to edge", &texture_wrap_t, 4);
		EndGroup();
		End();
	}

	void adjustTextureProperties() {
		switch (texture_filtering) {
			case 0:
				glTextureParameteri(texture_id, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				glTextureParameteri(texture_id, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
				break;

			case 1:
				glTextureParameteri(texture_id, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTextureParameteri(texture_id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				break;
		}

		switch (texture_wrap_s) {
#define CASE(id, param) \
	case id: \
		glTextureParameteri(texture_id, GL_TEXTURE_WRAP_S, param); \
		break
			
			CASE(0, GL_CLAMP_TO_EDGE);
			CASE(1, GL_CLAMP_TO_BORDER);
			CASE(2, GL_MIRRORED_REPEAT);
			CASE(3, GL_REPEAT);
			CASE(4, GL_MIRROR_CLAMP_TO_EDGE);

#undef CASE
		}

		switch (texture_wrap_t) {
#define CASE(id, param) \
	case id: \
		glTextureParameteri(texture_id, GL_TEXTURE_WRAP_T, param); \
		break
			
			CASE(0, GL_CLAMP_TO_EDGE);
			CASE(1, GL_CLAMP_TO_BORDER);
			CASE(2, GL_MIRRORED_REPEAT);
			CASE(3, GL_REPEAT);
			CASE(4, GL_MIRROR_CLAMP_TO_EDGE);

#undef CASE
		}
	}

	void moveSquare(double delta_time) {
		if (up)
			square_pos.y += delta_time * square_velocity;
		else if (down)
			square_pos.y -= delta_time * square_velocity;

		if (left)
			square_pos.x -= delta_time * square_velocity;
		else if (right)
			square_pos.x += delta_time * square_velocity;

		if (forward != 0.0) {
			square_pos.z += forward * 2 * delta_time * square_velocity;
			forward = 0.0;
		}
	}

	bool createProgram() {
		std::vector<ShaderInfo> shaders = {
			{
				GL_VERTEX_SHADER,
				"#version 450 core\n"
				""
				"layout (location = 0) in vec2 a_pos;"
				"layout (location = 1) in vec2 a_tex_coords;"
				""
				"uniform mat4 u_transform;"
				"uniform float u_angle;"
				""
				"out vec2 v_tex_coords;"
				""
				"void main() {"
				"	const mat2 rotate = mat2("
				"		vec2(cos(u_angle), sin(u_angle)),"
				"		vec2(-sin(u_angle), cos(u_angle)));"
				"	v_tex_coords = rotate * a_tex_coords;"
				"	gl_Position = u_transform * vec4(a_pos, 0.0, 1.0);"
				"}"
			},
			{
				GL_FRAGMENT_SHADER,
				"#version 450 core\n"
				""
				"in vec2 v_tex_coords;"
				""
				"uniform vec4 u_color;"
				"uniform bool u_has_color;"
				"layout (binding = 0) uniform sampler2D u_sampler;"
				"uniform bool u_has_texture;"
				""
				"out vec4 v_color;"
				""
				"void main() {"
				"	if (u_has_color && u_has_texture)"
				"		v_color = u_color * texture(u_sampler, v_tex_coords).rrra;"
				"	else if (u_has_color)"
				"		v_color = u_color;"
				"	else if (u_has_texture)"
				"		v_color = texture(u_sampler, v_tex_coords).rrra;"
				"	else"
				"		v_color = vec4(0.0);"
				"}"
			}
		};

		bool success;

		program_id = gl.createProgram(shaders, success);

		if (!success)
			return false;

		u_transform_loc = glGetUniformLocation(program_id, "u_transform");
		u_angle_loc = glGetUniformLocation(program_id, "u_angle");
		u_color_loc = glGetUniformLocation(program_id, "u_color");
		u_has_color_loc = glGetUniformLocation(program_id, "u_has_color");
		u_has_texture_loc = glGetUniformLocation(program_id, "u_has_texture");

		return true;
	}

	void createTexture() {
		std::vector<float> texture_data {
			0.0f, 1.0f,
			1.0f, 0.0f
		};

		glCreateTextures(GL_TEXTURE_2D, 1, &texture_id);
		glBindTextureUnit(0, texture_id);
		glTextureStorage2D(texture_id, 1, GL_R32F, 2, 2);
		glTextureSubImage2D(texture_id, 0, 0, 0, 2, 2,
			GL_RED, GL_FLOAT, texture_data.data());
	}

	void createGeometry() {
		std::vector<Vertex> vertices {
			{ { -1.0f, -1.0f }, { 0.0f, 0.0f } },
			{ { 1.0f, -1.0f }, { 5.0f, 0.0f } },
			{ { 1.0f, 1.0f }, { 5.0f, 5.0f } },
			{ { -1.0f, 1.0f }, { 0.0f, 5.0f } }
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
			4 * sizeof(float), (GLvoid*)offsetof(Vertex, pos));
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE,
			4 * sizeof(float), (GLvoid*)offsetof(Vertex, tex));
		glEnableVertexAttribArray(1);

		glCreateBuffers(1, &ebo_id);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_id);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER,
			indices.size() * sizeof(unsigned),
			indices.data(), GL_STATIC_DRAW);

		glBindVertexArray(0);
	}

	/// OpenGL handles
	GLuint vao_id;
	GLuint vbo_id;
	GLuint ebo_id;

	GLuint texture_id;

	GLuint program_id;
	GLuint u_transform_loc;
	GLuint u_angle_loc;
	GLuint u_color_loc;
	GLuint u_has_color_loc;
	GLuint u_has_texture_loc;

	/// Square properties
	glm::vec3 square_pos = glm::vec3(0.0f, 0.0f, -5.0f);
	float square_angle = 0.0f;
	float square_velocity = 6.0f;

	glm::vec4 square_color = glm::vec4(1.0f);
	bool color_on = true;
	bool texture_on = true;

	/// Texture properties
	int texture_filtering = 0;
	int texture_wrap_s = 3;
	int texture_wrap_t = 3;
	float texture_angle = 0.0f;

	/// Application state
	bool up = false;
	bool left = false;
	bool down = false;
	bool right = false;
	double forward = 0.0;

	glm::mat4 projection;
};

void onKey(GLFWwindow* window, int key, int scancode, int action, int mods) {
	auto app = static_cast<Application*>(glfwGetWindowUserPointer(window));

	switch (key) {
		case GLFW_KEY_W:
			app->moveUp(action != GLFW_RELEASE);
			break;

		case GLFW_KEY_A:
			app->moveLeft(action != GLFW_RELEASE);
			break;

		case GLFW_KEY_S:
			app->moveDown(action != GLFW_RELEASE);
			break;

		case GLFW_KEY_D:
			app->moveRight(action != GLFW_RELEASE);
			break;

		case GLFW_KEY_ESCAPE:
			app->close();
	}
}

void onMouseScroll(GLFWwindow* window, double x_offset, double y_offset) {
	auto app = static_cast<Application*>(glfwGetWindowUserPointer(window));

	app->moveZ(y_offset);
}

void windowResize(GLFWwindow* window, int width, int height) {
	auto app = static_cast<Application*>(glfwGetWindowUserPointer(window));

	app->setProjection(glm::perspective(
		glm::radians(60.0f), (float)width / height, 0.1f, 100.0f));

	glViewport(0, 0, width, height);
}

int main() {
	Application app("Hello, World", WINDOW_WIDTH, WINDOW_HEIGHT, true, true);

	if (app.init()) {
		app.run();
		app.destroy();
	}
}

