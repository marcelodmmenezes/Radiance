#include "../common/baseApp.hpp"
#include "../common/flyThroughCamera.hpp"
#include "../common/objParser.hpp"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#define WINDOW_WIDTH 1366
#define WINDOW_HEIGHT 768

#define N_LIGHTING_MODELS 8

void onKey(GLFWwindow* window, int key, int scancode, int action, int mods);
void onMouseMove(GLFWwindow* window, double xpos, double ypos);
void onMouseButton(GLFWwindow* window, int button, int action, int mods);
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
			fullscreen) {

		dir_light.direction = glm::vec3(-1.0f, -1.0f, -1.0f);
		dir_light.color = glm::vec3(1.0f, 1.0f, 1.0f);

		camera = FlyThroughCamera(glm::vec3(-3.0f, 3.0f, 3.0f), -45.0f, -30.0f);
	}

	~Application() {}

	void moveForward(bool state) {
		forward = state;
	}

	void moveBackward(bool state) {
		backward = state;
	}

	void moveLeft(bool state) {
		left = state;
	}

	void moveRight(bool state) {
		right = state;
	}

	void moveUp(bool state) {
		up = state;
	}

	void moveDown(bool state) {
		down = state;
	}

	void updateMousePos(double x, double y) {
		mouse_x = x;
		mouse_y = y;
	}

	void mouseGrab(bool state) {
		mouse_grab = state;
	}

	void fastCamera(bool fast) {
		camera.fast(fast);
	}

	void setProjection(glm::mat4&& proj) {
		projection = proj;
	}

private:
	struct Program {
		GLuint id;

		GLint u_transform_loc;
		GLint u_nor_transform_loc;

		GLint u_dir_light_direction_loc;
		GLint u_dir_light_color_loc;

		GLint u_wrap_value_loc;
	};

	struct DirectionalLight {
		glm::vec3 direction;
		glm::vec3 color;
	};

	bool customInit() override {
		glfwSetKeyCallback(window, onKey);
		glfwSetCursorPosCallback(window, onMouseMove);
		glfwSetMouseButtonCallback(window, onMouseButton);
		glfwSetWindowSizeCallback(window, windowResize);

		windowResize(window, window_width, window_height);

		if (!createProgram())
			return false;

		createTexture();

		if (!createGeometry())
			return false;

		glClearColor(0.10, 0.25, 0.15, 1.0);

		gl.enable(GL_DEPTH_TEST);

		if (gl.checkErrors(__FILE__, __LINE__))
			return false;

		return true;
	}

	bool customLoop(double delta_time) override {
		if (gl.checkErrors(__FILE__, __LINE__))
			return false;

		buildGUI();
		updateCamera(delta_time);

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glUseProgram(programs[current_program].id);

		glm::mat4 transform(1.0f);

		if (programs[current_program].u_transform_loc != -1) {
			glm::mat4 mvp = projection * camera.getViewMatrix() * transform;

			glUniformMatrix4fv(programs[current_program].u_transform_loc, 1,
				GL_FALSE, glm::value_ptr(mvp));
		}

		if (programs[current_program].u_nor_transform_loc != -1) {
			glm::mat3 nor_transform =
				glm::transpose(glm::inverse(glm::mat3(transform)));

			glUniformMatrix3fv(programs[current_program].u_nor_transform_loc, 1,
				GL_FALSE, glm::value_ptr(nor_transform));
		}

		if (programs[current_program].u_dir_light_direction_loc != -1)
			glUniform3fv(programs[current_program].u_dir_light_direction_loc,
				1, glm::value_ptr(dir_light.direction));

		if (programs[current_program].u_dir_light_color_loc != -1)
			glUniform3fv(programs[current_program].u_dir_light_color_loc,
				1, glm::value_ptr(dir_light.color));

		specificUniforms();

		glBindVertexArray(device_mesh.vao_id);
		glDrawElements(GL_TRIANGLES, n_indices, GL_UNSIGNED_INT, nullptr);

		return true;
	}

	void customDestroy() override {
		for (int i = 0; i < N_LIGHTING_MODELS; ++i)
			if (glIsProgram(programs[i].id))
				glDeleteProgram(programs[i].id);

		if (glIsTexture(texture_id))
			glDeleteTextures(1, &texture_id);

		gl.destroyGeometry(device_mesh);
	}

	void buildGUI() {
		using namespace ImGui;

		/// LIGHTING MODEL
		Begin("Lighting Model");
		Dummy(ImVec2(0.0f, 2.0f));

		BeginGroup();
		RadioButton("None", &current_program, 0);
		RadioButton("Lambert Diffuse Lighting", &current_program, 1);
		RadioButton("Half-Lambert (Diffuse Wrap)", &current_program, 2);
		RadioButton("Phong Lighting", &current_program, 3);
		//RadioButton("Blinn-Phong Lighting", &current_program, 4);
		//RadioButton("Banded Lighting", &current_program, 5);
		//RadioButton("Minnaert Lighting", &current_program, 6);
		//RadioButton("Oren-Nayer Lighting", &current_program, 6);
		EndGroup();

		Dummy(ImVec2(0.0f, 2.0f));
		End();

		lightOptionsGUI();

		/// LIGHTS
		Begin("Directional Light");
		Dummy(ImVec2(0.0f, 2.0f));

		Separator();
		SliderFloat("Direction X", &dir_light.direction.x, -1.0f, 1.0f);
		SliderFloat("Direction Y", &dir_light.direction.y, -1.0f, 1.0f);
		SliderFloat("Direction Z", &dir_light.direction.z, -1.0f, 1.0f);

		Dummy(ImVec2(0.0f, 2.0f));
		Separator();
		ColorPicker4("Color", glm::value_ptr(dir_light.color));

		End();

		/// CAMERA
		Begin("Camera");
		Dummy(ImVec2(0.0f, 2.0f));

		Text("To move the camera use WASD and QE keys");
		Text("To look around click and drag with RMB");

		Dummy(ImVec2(0.0f, 2.0f));
		Separator();
		SliderFloat("Move Speed", &camera.move_speed, 1.0f, 20.0f);
		SliderFloat("Look Speed", &camera.look_speed, 1.0f, 20.0f);

		Dummy(ImVec2(0.0f, 2.0f));
		Separator();
		SliderFloat("X", &camera.position.x, -10.0f, 10.0f);
		SliderFloat("Y", &camera.position.y, -10.0f, 10.0f);
		SliderFloat("Z", &camera.position.z, -10.0f, 10.0f);

		Dummy(ImVec2(0.0f, 2.0f));
		Separator();
		SliderFloat("Yaw", &camera.yaw, 180.0f, -180.0f);

		Dummy(ImVec2(0.0f, 2.0f));
		Separator();
		SliderFloat("Pitch", &camera.pitch, -89.0f, 89.0f);

		Dummy(ImVec2(0.0f, 2.0f));
		End();
	}

	void lightOptionsGUI() {
		using namespace ImGui;

		switch (current_program) {
			case 2:
				Begin("Half-Lambert - Options");
				SliderFloat("Wrap value", &half_lambert_wrap, 0.0f, 1.0f);
				End();

				break;

			case 3:
				Begin("Phong - Options");
				End();

				break;
		}
	}

	void specificUniforms() {
		switch (current_program) {
			case 2:
				assert(programs[current_program].u_wrap_value_loc != -1);
				glUniform1f(programs[current_program].u_wrap_value_loc, half_lambert_wrap);

				break;
		}
	}

	void updateCamera(float delta_time) {
		if (forward)
			camera.move(CameraDirection::FORWARD, delta_time);
		else if (backward)
			camera.move(CameraDirection::BACKWARD, delta_time);

		if (left)
			camera.move(CameraDirection::LEFT, delta_time);
		else if (right)
			camera.move(CameraDirection::RIGHT, delta_time);

		if (up)
			camera.move(CameraDirection::UP, delta_time);
		else if (down)
			camera.move(CameraDirection::DOWN, delta_time);

		if (mouse_grab)
			camera.look(mouse_last_x - mouse_x, mouse_last_y - mouse_y, delta_time);

		mouse_last_x = mouse_x;
		mouse_last_y = mouse_y;
	}

	bool createProgram() {
		std::vector<ShaderInfo> shaders[N_LIGHTING_MODELS];

		std::string files[N_LIGHTING_MODELS][2] {
			{
				"shaders/none/vs.glsl",
				"shaders/none/fs.glsl",
			},
			{
				"shaders/lambert/vs.glsl",
				"shaders/lambert/fs.glsl",
			},
			{
				"shaders/half_lambert/vs.glsl",
				"shaders/half_lambert/fs.glsl",
			},
			{
				"shaders/phong/vs.glsl",
				"shaders/phong/fs.glsl",
			},
			{
				"shaders/blinn_phong/vs.glsl",
				"shaders/blinn_phong/fs.glsl",
			},
			{
				"shaders/banded/vs.glsl",
				"shaders/banded/fs.glsl",
			},
			{
				"shaders/minnaert/vs.glsl",
				"shaders/minnaert/fs.glsl",
			},
			{
				"shaders/oren_nayer/vs.glsl",
				"shaders/oren_nayer/fs.glsl",
			}
		};

		for (int i = 0; i < N_LIGHTING_MODELS; ++i) {
			std::ifstream vs_file(files[i][0], std::ios::ate);
			std::ifstream fs_file(files[i][1], std::ios::ate);

			if (!vs_file) {
				std::cerr << "ERROR: Could not open " << files[i][0] << '\n';
				return false;
			}

			if (!fs_file) {
				std::cerr << "ERROR: Could not open " << files[i][1] << '\n';
				return false;
			}

			std::cout << "Creating program: "
				<< files[i][0].substr(0, files[i][0].find_last_of("\\/"))
				<< " ... ";

			readShader(vs_file, fs_file, shaders[i]);

			fs_file.close();
			vs_file.close();

			bool success;

			programs[i].id = gl.createProgram(shaders[i], success);

			if (!success)
				return false;
			else
				std::cout << "SUCCESS\n";

			programs[i].u_transform_loc =
				glGetUniformLocation(programs[i].id, "u_transform");
			programs[i].u_nor_transform_loc =
				glGetUniformLocation(programs[i].id, "u_nor_transform");

			programs[i].u_dir_light_direction_loc =
				glGetUniformLocation(programs[i].id, "u_dir_light.direction");
			programs[i].u_dir_light_color_loc =
				glGetUniformLocation(programs[i].id, "u_dir_light.color");

			programs[i].u_wrap_value_loc =
				glGetUniformLocation(programs[i].id, "u_wrap_value");
		}

		return true;
	}

	void readShader(
		std::ifstream& vs,
		std::ifstream& fs,
		std::vector<ShaderInfo>& shaders) {

		shaders.resize(2);

		int vs_size = vs.tellg();
		shaders[0].type = GL_VERTEX_SHADER;
		shaders[0].source.resize(vs_size);
		vs.seekg(0, std::ios::beg);
		vs.read(&shaders[0].source[0], vs_size);

		int fs_size = fs.tellg();
		shaders[1].type = GL_FRAGMENT_SHADER;
		shaders[1].source.resize(fs_size); 
		fs.seekg(0, std::ios::beg);
		fs.read(&shaders[1].source[0], fs_size);
	}

	void createTexture() {
		std::vector<float> texture_data {
			0.4f, 0.6f,
			0.6f, 0.4f
		};

		glCreateTextures(GL_TEXTURE_2D, 1, &texture_id);

		glTextureParameteri(texture_id, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTextureParameteri(texture_id, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		glTextureStorage2D(texture_id, 1, GL_R32F, 2, 2);
		glTextureSubImage2D(texture_id, 0, 0, 0, 2, 2,
			GL_RED, GL_FLOAT, texture_data.data());

		glBindTextureUnit(0, texture_id);
	}

	bool createGeometry() {
		std::vector<BufferInfo<float>> f_buffers;
		std::vector<BufferInfo<int>> i_buffers;
		std::vector<unsigned> indices;

		//bool success = parseOBJ("../res/square.obj", f_buffers, indices);
		//bool success = parseOBJ("../res/cube.obj", f_buffers, indices);
		bool success = parseOBJ("../res/materialBall.obj", f_buffers, indices);

		if (!success)
			return false;

		f_buffers[0].attribute_name = "a_pos";
		f_buffers[1].attribute_name = "a_nor";
		f_buffers[2].attribute_name = "a_tex";

		// Assuming all programs attrib locations are the same
		// (Except program 0 - None)
		// Shader must specify attrib location on layout
		device_mesh = gl.createPackedStaticGeometry(
			programs[1].id, f_buffers,
			i_buffers, indices, success);

		if (!success)
			return false;

		n_indices = indices.size();

		return true;
	}

	/// OpenGL handles
	DeviceMesh device_mesh;

	GLuint texture_id;

	Program programs[N_LIGHTING_MODELS];

	/// Lighting model
	float half_lambert_wrap = 0.5f;

	/// Cube properties
	glm::vec3 cube_pos = glm::vec3(0.0f, 0.0f, -5.0f);
	float cube_pitch = 0.0f;
	float cube_yaw = 0.0f;
	float cube_roll = 0.0f;

	size_t n_indices = 0u;

	/// Lights
	DirectionalLight dir_light;

	/// Camera
	FlyThroughCamera camera;

	bool forward = false;
	bool backward = false;
	bool left = false;
	bool right = false;
	bool up = false;
	bool down = false;

	/// Application state
	int current_program = 0;

	bool mouse_grab = false;
	double mouse_x;
	double mouse_y;
	double mouse_last_x = 0.0f;
	double mouse_last_y = 0.0f;

	glm::mat4 projection;
};

void onKey(GLFWwindow* window, int key, int scancode, int action, int mods) {
	auto app = static_cast<Application*>(glfwGetWindowUserPointer(window));

	app->fastCamera((mods & GLFW_MOD_SHIFT) == GLFW_MOD_SHIFT);

	switch (key) {
		case GLFW_KEY_W:
			app->moveForward(action != GLFW_RELEASE);
			break;

		case GLFW_KEY_A:
			app->moveLeft(action != GLFW_RELEASE);
			break;

		case GLFW_KEY_S:
			app->moveBackward(action != GLFW_RELEASE);
			break;

		case GLFW_KEY_D:
			app->moveRight(action != GLFW_RELEASE);
			break;

		case GLFW_KEY_Q:
			app->moveUp(action != GLFW_RELEASE);
			break;

		case GLFW_KEY_E:
			app->moveDown(action != GLFW_RELEASE);
			break;

		case GLFW_KEY_ESCAPE:
			app->close();
	}
}

void onMouseMove(GLFWwindow* window, double xpos, double ypos) {
	auto app = static_cast<Application*>(glfwGetWindowUserPointer(window));

	app->updateMousePos(xpos, ypos);
}

void onMouseButton(GLFWwindow* window, int button, int action, int mods) {
	auto app = static_cast<Application*>(glfwGetWindowUserPointer(window));

	if (button == GLFW_MOUSE_BUTTON_RIGHT) {
		if (action != GLFW_RELEASE) {
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			app->mouseGrab(true);
		}
		else {
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			app->mouseGrab(false);
		}
	}
}

void windowResize(GLFWwindow* window, int width, int height) {
	auto app = static_cast<Application*>(glfwGetWindowUserPointer(window));

	app->setProjection(glm::perspective(
		glm::radians(60.0f), (float)width / height, 0.1f, 100.0f));

	glViewport(0, 0, width, height);
}

int main() {
	Application app("Classic Model", WINDOW_WIDTH, WINDOW_HEIGHT);

	if (app.init()) {
		app.run();
		app.destroy();
	}
}

