#include "../common/baseApp.hpp"
#include "../common/objParser.hpp"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#define WINDOW_WIDTH 1366
#define WINDOW_HEIGHT 768

#define N_LIGHTING_MODELS 7

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

	void setProjection(glm::mat4&& proj) {
		projection = proj;
	}

private:
	struct Program {
		GLuint id;
		GLuint u_transform_loc;
		GLuint u_nor_transform_loc;
	};

	bool customInit() override {
		glfwSetWindowSizeCallback(window, windowResize);

		windowResize(window, window_width, window_height);

		if (!createProgram())
			return false;

		createTexture();

		if (!createGeometry())
			return false;

		glClearColor(0.10, 0.25, 0.15, 1.0);

		if (gl.checkErrors(__FILE__, __LINE__))
			return false;

		return true;
	}

	bool customLoop(double delta_time) override {
		if (gl.checkErrors(__FILE__, __LINE__))
			return false;

		buildGUI();

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glUseProgram(programs[current_program].id);

		glm::mat4 transform = projection * glm::mat4(1.0f);
		glm::mat3 nor_transform = glm::transpose(glm::inverse(glm::mat3(transform)));

		glUniformMatrix4fv(programs[current_program].u_transform_loc, 1,
			GL_FALSE, glm::value_ptr(transform));
		glUniformMatrix3fv(programs[current_program].u_nor_transform_loc, 1,
			GL_FALSE, glm::value_ptr(nor_transform));

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

		Begin("Lighting Model");
		Dummy(ImVec2(0.0f, 2.0f));
		BeginGroup();
		RadioButton("Lambert Diffuse Lighting", &current_program, 0);
		//RadioButton("Half-Lambert (Diffuse Wrap)", &current_program, 1);
		//RadioButton("Phong Lighting", &current_program, 2);
		//RadioButton("Blinn-Phong Lighting", &current_program, 3);
		//RadioButton("Banded Lighting", &current_program, 4);
		//RadioButton("Minnaert Lighting", &current_program, 5);
		//RadioButton("Oren-Nayer Lighting", &current_program, 6);
		EndGroup();
		Dummy(ImVec2(0.0f, 2.0f));
		End();
	}

	bool createProgram() {
		std::vector<ShaderInfo> shaders[N_LIGHTING_MODELS];

		std::string files[N_LIGHTING_MODELS][2] {
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
			0.0f, 1.0f,
			1.0f, 0.0f
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
		bool success = parseOBJ("../res/cube.obj", f_buffers, indices);

		if (!success)
			return false;

		f_buffers[0].attribute_name = "a_pos";
		f_buffers[1].attribute_name = "a_nor";
		f_buffers[2].attribute_name = "a_tex";

		// Assuming all programs attrib locations are the same
		// Shader must specify attrib location on layout
		device_mesh = gl.createPackedStaticGeometry(
			programs[current_program].id, f_buffers,
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

	/// Cube properties
	glm::vec3 cube_pos = glm::vec3(0.0f, 0.0f, -5.0f);
	float cube_pitch = 0.0f;
	float cube_yaw = 0.0f;
	float cube_roll = 0.0f;

	size_t n_indices = 0u;

	/// Application state
	int current_program = 0;

	glm::mat4 projection;
};

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

