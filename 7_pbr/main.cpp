#include "../common/baseApp.hpp"
#include "../common/flyThroughCamera.hpp"
#include "../common/objParser.hpp"
#include "../common/texture.hpp"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#define WINDOW_WIDTH 1366
#define WINDOW_HEIGHT 768

#define N_TEXTURES 4

void onKey(GLFWwindow* window, int key, int, int action, int mods);
void onMouseMove(GLFWwindow* window, double xpos, double ypos);
void onMouseButton(GLFWwindow* window, int button, int action, int);
void windowResize(GLFWwindow* window, int width, int height);

class Application : public BaseApplication
{
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
			fullscreen) 
	{
		amb_light.color = glm::vec3(0.01f, 0.025f, 0.015f);

		dir_light.direction = glm::vec3(-1.0f, -1.0f, -1.0f);
		dir_light.color = glm::vec3(1.0f, 1.0f, 1.0f);

		camera = FlyThroughCamera(glm::vec3(0.0f, 1.0f, 4.0f), 0.0f, -20.0f);

		model_matrix = glm::mat4(1.0f);
	}

	~Application()
	{}

	void moveForward(bool state)
	{
		forward = state;
	}

	void moveBackward(bool state)
	{
		backward = state;
	}

	void moveLeft(bool state)
	{
		left = state;
	}

	void moveRight(bool state)
	{
		right = state;
	}

	void moveUp(bool state)
	{
		up = state;
	}

	void moveDown(bool state)
	{
		down = state;
	}

	void updateMousePos(double x, double y)
	{
		mouse_x = x;
		mouse_y = y;
	}

	void mouseGrab(bool state)
	{
		mouse_grab = state;
	}

	void fastCamera(bool fast)
	{
		camera.fast(fast);
	}

	void setProjection(glm::mat4&& proj)
	{
		projection = proj;
	}

private:
	struct BlinnPhongProgram
	{
		GLuint id;

		GLint u_model_matrix_loc;
		GLint u_view_matrix_loc;
		GLint u_projection_matrix_loc;
		GLint u_nor_transform_loc;

		GLint u_color_sampler_loc;
		GLint u_normal_sampler_loc;

		GLint u_amb_light_color_loc;

		GLint u_dir_light_direction_loc;
		GLint u_dir_light_color_loc;

		GLint u_view_pos_loc;
		GLint u_shininess_loc;

		GLint u_bump_map_active_loc;

		GLint u_gamma_loc;
		GLint u_exposure_loc;
	};

	struct StandardPBRProgram
	{
		GLuint id;

		GLint u_model_matrix_loc;
		GLint u_view_matrix_loc;
		GLint u_projection_matrix_loc;
		GLint u_nor_transform_loc;

		GLint u_color_sampler_loc;
		GLint u_normal_sampler_loc;

		GLint u_has_metallic_map_loc;
		GLint u_metallic_sampler_loc;
		GLint u_metallic_loc;

		GLint u_has_roughness_map_loc;
		GLint u_roughness_sampler_loc;
		GLint u_roughness_loc;

		GLint u_amb_light_color_loc;

		GLint u_dir_light_direction_loc;
		GLint u_dir_light_color_loc;

		GLint u_view_pos_loc;

		GLint u_bump_map_active_loc;

		GLint u_gamma_loc;
		GLint u_exposure_loc;
	};

	struct AmbientLight
	{
		glm::vec3 color;
	};

	struct DirectionalLight
	{
		glm::vec3 direction;
		glm::vec3 color;
	};

	bool customInit() override
	{
		glfwSetKeyCallback(window, onKey);
		glfwSetCursorPosCallback(window, onMouseMove);
		glfwSetMouseButtonCallback(window, onMouseButton);
		glfwSetWindowSizeCallback(window, windowResize);

		windowResize(window, window_width, window_height);

		if (!createBlinnPhongProgram())
		{
			return false;
		}

		if (!createStandardPBRProgram())
		{
			return false;
		}

		createTextures();

		if (!createGeometry())
		{
			return false;
		}

		glClearColor(0.10, 0.25, 0.15, 1.0);

		gl.enable(GL_DEPTH_TEST);
		gl.enable(GL_CULL_FACE);

		if (OpenGLContext::checkErrors(__FILE__, __LINE__))
		{
			return false;
		}

		return true;
	}

	bool customLoop(double delta_time) override
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		buildGUI();

		updateCamera(delta_time);
		glm::vec3 camera_position = camera.new_position;
		glm::mat4 view_matrix = camera.getViewMatrix();

		switch (current_program)
		{
		case 0:
			blinnPhong(camera_position, view_matrix);
			break;

		case 1:
			standardPBR(camera_position, view_matrix);
			break;
		}

		glBindVertexArray(geometry.vao_id);
		glDrawElements(GL_TRIANGLES, geometry.n_indices, GL_UNSIGNED_INT, nullptr);

		return true;
	}

	void blinnPhong(
		glm::vec3 const& camera_position,
		glm::mat4 const& view_matrix)
	{
		glUseProgram(blinn_phong.id);

		glUniformMatrix4fv(blinn_phong.u_model_matrix_loc,
			1, GL_FALSE, glm::value_ptr(model_matrix));
		glUniformMatrix4fv(blinn_phong.u_view_matrix_loc,
			1, GL_FALSE, glm::value_ptr(view_matrix));
		glUniformMatrix4fv(blinn_phong.u_projection_matrix_loc,
			1, GL_FALSE, glm::value_ptr(projection));
		glUniformMatrix3fv(blinn_phong.u_nor_transform_loc,
			1, GL_FALSE, glm::value_ptr(glm::mat3(
				glm::transpose(glm::inverse(model_matrix)))));

		glUniform1i(blinn_phong.u_color_sampler_loc, 0);
		glUniform1i(blinn_phong.u_normal_sampler_loc, 1);

		glUniform3fv(blinn_phong.u_amb_light_color_loc,
			1, glm::value_ptr(amb_light.color));

		glUniform3fv(blinn_phong.u_dir_light_direction_loc,
			1, glm::value_ptr(dir_light.direction));
		glUniform3fv(blinn_phong.u_dir_light_color_loc,
			1, glm::value_ptr(dir_light.color));

		glUniform3fv(blinn_phong.u_view_pos_loc,
			1, glm::value_ptr(camera_position));

		glUniform1f(blinn_phong.u_shininess_loc, shininess);

		glUniform1f(blinn_phong.u_bump_map_active_loc, bump_map_active);

		glUniform1f(blinn_phong.u_gamma_loc, gamma_correction);
		glUniform1f(blinn_phong.u_exposure_loc, exposure);
	}

	void standardPBR(
		glm::vec3 const& camera_position,
		glm::mat4 const& view_matrix)
	{
		glUseProgram(standard_pbr.id);

		glUniformMatrix4fv(standard_pbr.u_model_matrix_loc,
			1, GL_FALSE, glm::value_ptr(model_matrix));
		glUniformMatrix4fv(standard_pbr.u_view_matrix_loc,
			1, GL_FALSE, glm::value_ptr(view_matrix));
		glUniformMatrix4fv(standard_pbr.u_projection_matrix_loc,
			1, GL_FALSE, glm::value_ptr(projection));
		glUniformMatrix3fv(standard_pbr.u_nor_transform_loc,
			1, GL_FALSE, glm::value_ptr(glm::mat3(
				glm::transpose(glm::inverse(model_matrix)))));

		glUniform1i(standard_pbr.u_color_sampler_loc, 0);
		glUniform1i(standard_pbr.u_normal_sampler_loc, 1);

		glUniform1i(standard_pbr.u_has_metallic_map_loc, has_metallic_map);
		glUniform1i(standard_pbr.u_metallic_sampler_loc, 2);
		glUniform1f(standard_pbr.u_metallic_loc, metallic);

		glUniform1i(standard_pbr.u_has_roughness_map_loc, has_roughness_map);
		glUniform1i(standard_pbr.u_roughness_sampler_loc, 3);
		glUniform1f(standard_pbr.u_roughness_loc, roughness);

		glUniform3fv(standard_pbr.u_amb_light_color_loc,
			1, glm::value_ptr(amb_light.color));

		glUniform3fv(standard_pbr.u_dir_light_direction_loc,
			1, glm::value_ptr(dir_light.direction));
		glUniform3fv(standard_pbr.u_dir_light_color_loc,
			1, glm::value_ptr(dir_light.color));

		glUniform3fv(standard_pbr.u_view_pos_loc,
			1, glm::value_ptr(camera_position));

		glUniform1f(standard_pbr.u_bump_map_active_loc, bump_map_active);

		glUniform1f(standard_pbr.u_gamma_loc, gamma_correction);
		glUniform1f(standard_pbr.u_exposure_loc, exposure);
	}

	void customDestroy() override
	{
		if (glIsProgram(blinn_phong.id))
		{
			glDeleteProgram(blinn_phong.id);
		}

		if (glIsProgram(standard_pbr.id))
		{
			glDeleteProgram(standard_pbr.id);
		}

		for (int i = 0; i < N_TEXTURES; ++i)
		{
			textures[i].destroy();
		}

		gl.destroyGeometry(geometry);
	}

	void buildGUI()
	{
		using namespace ImGui;

		/// OBJECT
		Begin("Material");

		Dummy(ImVec2(0.0f, 2.0f));
		Separator();

		Text("Shading Model");
		RadioButton("Blinn Phong", &current_program, 0);
		RadioButton("Standard PBR", &current_program, 1);

		Checkbox("Normal map", &bump_map_active);

		Dummy(ImVec2(0.0f, 2.0f));
		Separator();

		switch (current_program)
		{
		case 0:
			blinnPhongGUI();
			break;

		case 1:
			standardPBRGUI();
			break;
		}

		End();

		/// LIGHTS
		Begin("Ambient Light");

		Dummy(ImVec2(0.0f, 2.0f));
		Separator();

		ColorPicker3("Color", glm::value_ptr(amb_light.color));

		End();

		Begin("Directional Light");

		Dummy(ImVec2(0.0f, 2.0f));
		Separator();

		SliderFloat("Direction X", &dir_light.direction.x, -1.0f, 1.0f);
		SliderFloat("Direction Y", &dir_light.direction.y, -1.0f, 1.0f);
		SliderFloat("Direction Z", &dir_light.direction.z, -1.0f, 1.0f);

		Dummy(ImVec2(0.0f, 2.0f));
		Separator();

		ColorPicker3("Color", glm::value_ptr(dir_light.color));

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
		Separator();

		SliderFloat("Gamma", &gamma_correction, 0.1f, 5.0f);
		SliderFloat("Exposure", &exposure, 0.1f, 5.0f);

		Dummy(ImVec2(0.0f, 2.0f));

		End();
	}

	void blinnPhongGUI()
	{
		using namespace ImGui;

		SliderFloat("Shininess", &shininess, 1.0f, 64.0f);
	}

	void standardPBRGUI()
	{
		using namespace ImGui;

		Checkbox("Use metallic map", &has_metallic_map);
		Checkbox("Use roughness map", &has_roughness_map);

		if (!has_metallic_map)
		{
			SliderFloat("Metallic", &metallic, 0.0f, 1.0f);
		}

		if (!has_roughness_map)
		{
			SliderFloat("Roughness", &roughness, 0.05f, 1.0f);
		}
	}

	void updateCamera(float delta_time)
	{
		if (forward)
		{
			camera.move(CameraDirection::FORWARD, delta_time);
		}
		else if (backward)
		{
			camera.move(CameraDirection::BACKWARD, delta_time);
		}

		if (left)
		{
			camera.move(CameraDirection::LEFT, delta_time);
		}
		else if (right)
		{
			camera.move(CameraDirection::RIGHT, delta_time);
		}

		if (up)
		{
			camera.move(CameraDirection::UP, delta_time);
		}
		else if (down)
		{
			camera.move(CameraDirection::DOWN, delta_time);
		}

		if (mouse_grab)
		{
			camera.look(mouse_last_x - mouse_x, mouse_last_y - mouse_y, delta_time);
		}

		mouse_last_x = mouse_x;
		mouse_last_y = mouse_y;
	}

	bool createBlinnPhongProgram()
	{
		std::vector<ShaderInfo> shaders;

		std::ifstream vs_file("shaders/blinnPhong/vs.glsl");
		std::ifstream fs_file("shaders/blinnPhong/fs.glsl");

		if (!vs_file)
		{
			std::cerr << "ERROR: Could not open vertex shader\n";
			return false;
		}

		if (!fs_file)
		{
			std::cerr << "ERROR: Could not open fragment shader\n";
			return false;
		}

		std::cout << "Creating BlinnPhong program ... ";

		readShader(vs_file, fs_file, shaders);

		bool success;

		blinn_phong.id = gl.createProgram(shaders, success);

		if (!success)
		{
			return false;
		}
		else
		{
			std::cout << "SUCCESS\n";
		}

		blinn_phong.u_model_matrix_loc =
			glGetUniformLocation(blinn_phong.id, "u_model_matrix");
		blinn_phong.u_view_matrix_loc =
			glGetUniformLocation(blinn_phong.id, "u_view_matrix");
		blinn_phong.u_projection_matrix_loc =
			glGetUniformLocation(blinn_phong.id, "u_projection_matrix");
		blinn_phong.u_nor_transform_loc =
			glGetUniformLocation(blinn_phong.id, "u_nor_transform");

		blinn_phong.u_color_sampler_loc =
			glGetUniformLocation(blinn_phong.id, "u_color_sampler");
		blinn_phong.u_normal_sampler_loc =
			glGetUniformLocation(blinn_phong.id, "u_normal_sampler");

		blinn_phong.u_amb_light_color_loc =
			glGetUniformLocation(blinn_phong.id, "u_amb_light.color");

		blinn_phong.u_dir_light_direction_loc =
			glGetUniformLocation(blinn_phong.id, "u_dir_light.direction");
		blinn_phong.u_dir_light_color_loc =
			glGetUniformLocation(blinn_phong.id, "u_dir_light.color");

		blinn_phong.u_view_pos_loc =
			glGetUniformLocation(blinn_phong.id, "u_view_pos");

		blinn_phong.u_shininess_loc =
			glGetUniformLocation(blinn_phong.id, "u_shininess");

		blinn_phong.u_bump_map_active_loc =
			glGetUniformLocation(blinn_phong.id, "u_bump_map_active");

		blinn_phong.u_gamma_loc =
			glGetUniformLocation(blinn_phong.id, "u_gamma");
		blinn_phong.u_exposure_loc =
			glGetUniformLocation(blinn_phong.id, "u_exposure");

		assert(blinn_phong.u_model_matrix_loc != -1);
		assert(blinn_phong.u_view_pos_loc != -1);
		assert(blinn_phong.u_projection_matrix_loc != -1);
		assert(blinn_phong.u_nor_transform_loc != -1);
		assert(blinn_phong.u_color_sampler_loc != -1);
		assert(blinn_phong.u_normal_sampler_loc != -1);
		assert(blinn_phong.u_amb_light_color_loc != -1);
		assert(blinn_phong.u_dir_light_color_loc != -1);
		assert(blinn_phong.u_view_pos_loc != -1);
		assert(blinn_phong.u_shininess_loc != -1);
		assert(blinn_phong.u_bump_map_active_loc != -1);
		assert(blinn_phong.u_gamma_loc != -1);
		assert(blinn_phong.u_exposure_loc != -1);

		return true;
	}

	bool createStandardPBRProgram()
	{
		std::vector<ShaderInfo> shaders;

		std::ifstream vs_file("shaders/standardPBR/vs.glsl");
		std::ifstream fs_file("shaders/standardPBR/fs.glsl");

		if (!vs_file)
		{
			std::cerr << "ERROR: Could not open vertex shader\n";
			return false;
		}

		if (!fs_file)
		{
			std::cerr << "ERROR: Could not open fragment shader\n";
			return false;
		}

		std::cout << "Creating StandardPBR program ... ";

		readShader(vs_file, fs_file, shaders);

		bool success;

		standard_pbr.id = gl.createProgram(shaders, success);

		if (!success)
		{
			return false;
		}
		else
		{
			std::cout << "SUCCESS\n";
		}

		standard_pbr.u_model_matrix_loc =
			glGetUniformLocation(standard_pbr.id, "u_model_matrix");
		standard_pbr.u_view_matrix_loc =
			glGetUniformLocation(standard_pbr.id, "u_view_matrix");
		standard_pbr.u_projection_matrix_loc =
			glGetUniformLocation(standard_pbr.id, "u_projection_matrix");
		standard_pbr.u_nor_transform_loc =
			glGetUniformLocation(standard_pbr.id, "u_nor_transform");

		standard_pbr.u_color_sampler_loc =
			glGetUniformLocation(standard_pbr.id, "u_color_sampler");
		standard_pbr.u_normal_sampler_loc =
			glGetUniformLocation(standard_pbr.id, "u_normal_sampler");

		standard_pbr.u_has_metallic_map_loc =
			glGetUniformLocation(standard_pbr.id, "u_has_metallic_map");
		standard_pbr.u_metallic_sampler_loc =
			glGetUniformLocation(standard_pbr.id, "u_metallic_sampler");
		standard_pbr.u_metallic_loc =
			glGetUniformLocation(standard_pbr.id, "u_metallic");

		standard_pbr.u_has_roughness_map_loc =
			glGetUniformLocation(standard_pbr.id, "u_has_roughness_map");
		standard_pbr.u_roughness_sampler_loc =
			glGetUniformLocation(standard_pbr.id, "u_roughness_sampler");
		standard_pbr.u_roughness_loc =
			glGetUniformLocation(standard_pbr.id, "u_roughness");

		standard_pbr.u_amb_light_color_loc =
			glGetUniformLocation(standard_pbr.id, "u_amb_light.color");

		standard_pbr.u_dir_light_direction_loc =
			glGetUniformLocation(standard_pbr.id, "u_dir_light.direction");
		standard_pbr.u_dir_light_color_loc =
			glGetUniformLocation(standard_pbr.id, "u_dir_light.color");

		standard_pbr.u_view_pos_loc =
			glGetUniformLocation(standard_pbr.id, "u_view_pos");

		standard_pbr.u_bump_map_active_loc =
			glGetUniformLocation(standard_pbr.id, "u_bump_map_active");

		standard_pbr.u_gamma_loc =
			glGetUniformLocation(standard_pbr.id, "u_gamma");
		standard_pbr.u_exposure_loc =
			glGetUniformLocation(standard_pbr.id, "u_exposure");

		assert(standard_pbr.u_model_matrix_loc != -1);
		assert(standard_pbr.u_view_pos_loc != -1);
		assert(standard_pbr.u_projection_matrix_loc != -1);
		assert(standard_pbr.u_nor_transform_loc != -1);
		assert(standard_pbr.u_color_sampler_loc != -1);
		assert(standard_pbr.u_normal_sampler_loc != -1);
		assert(standard_pbr.u_has_metallic_map_loc != -1);
		assert(standard_pbr.u_metallic_sampler_loc != -1);
		assert(standard_pbr.u_metallic_loc != -1);
		assert(standard_pbr.u_has_roughness_map_loc != -1);
		assert(standard_pbr.u_roughness_sampler_loc != -1);
		assert(standard_pbr.u_roughness_loc != -1);
		assert(standard_pbr.u_amb_light_color_loc != -1);
		assert(standard_pbr.u_dir_light_color_loc != -1);
		assert(standard_pbr.u_view_pos_loc != -1);
		assert(standard_pbr.u_bump_map_active_loc != -1);
		assert(standard_pbr.u_gamma_loc != -1);
		assert(standard_pbr.u_exposure_loc != -1);

		return true;
	}

	void readShader(
		std::ifstream& vs,
		std::ifstream& fs,
		std::vector<ShaderInfo>& shaders)
	{
		shaders.resize(2);

		shaders[0].type = GL_VERTEX_SHADER;
		readFile(vs, shaders[0]);

		shaders[1].type = GL_FRAGMENT_SHADER;
		readFile(fs, shaders[1]);
	}

	void readFile(std::ifstream& stream, ShaderInfo& shader_info)
	{
		std::string line;

		while (std::getline(stream, line))
		{
			shader_info.source += line + "\n";
		}
	}

	void createTextures()
	{
		textures[0] = Texture2D("../res/materialBall/color.png", 3,
			GL_REPEAT, GL_REPEAT, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, true);

		textures[1] = Texture2D("../res/materialBall/normal.png", 3,
			GL_REPEAT, GL_REPEAT, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, true);

		textures[2] = Texture2D("../res/materialBall/metallic.png", 1,
			GL_REPEAT, GL_REPEAT, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, true);

		textures[3] = Texture2D("../res/materialBall/roughness.png", 3,
			GL_REPEAT, GL_REPEAT, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, true);

		for (int i = 0; i < N_TEXTURES; ++i)
		{
			textures[i].bind(i);
		}
	}

	bool createGeometry()
	{
		std::vector<BufferInfo<float>> f_buffers;
		std::vector<BufferInfo<int>> i_buffers;
		std::vector<unsigned> indices;

		bool success = parseOBJ("../res/materialBall/mesh.obj", f_buffers, indices);

		if (!success)
		{
			return false;
		}

		f_buffers[0].attribute_name = "a_pos";
		f_buffers[1].attribute_name = "a_nor";
		f_buffers[2].attribute_name = "a_tex";

		f_buffers.emplace_back(
			BufferInfo<float>
			{
				"a_tan",
				3,
				std::vector<float>(f_buffers[0].values.size(), 0.0f)
			});

		generateTangentVectors(
			indices,
			f_buffers[0].values,
			f_buffers[2].values,
			f_buffers[3].values);

		geometry = gl.createPackedStaticGeometry(
			blinn_phong.id, f_buffers, i_buffers, indices, success);

		if (!success)
		{
			return false;
		}

		return true;
	}

	/// Programs
	int current_program = 0;

	BlinnPhongProgram blinn_phong;
	StandardPBRProgram standard_pbr;

	/// Material
	Texture textures[N_TEXTURES];

	float shininess = 32.0f;
	bool bump_map_active = true;

	bool has_metallic_map = false;
	float metallic = 0.0f;
	bool has_roughness_map = false;
	float roughness = 0.05f;

	/// Object properties
	DeviceMesh geometry;
	glm::mat4 model_matrix;

	/// Lights
	AmbientLight amb_light;
	DirectionalLight dir_light;

	/// Camera
	FlyThroughCamera camera;

	bool forward = false;
	bool backward = false;
	bool left = false;
	bool right = false;
	bool up = false;
	bool down = false;

	float gamma_correction = 2.2f;
	float exposure = 1.0f;

	/// Application state
	bool mouse_grab = false;
	double mouse_x;
	double mouse_y;
	double mouse_last_x = 0.0f;
	double mouse_last_y = 0.0f;

	glm::mat4 projection;
};

void onKey(GLFWwindow* window, int key, int, int action, int mods)
{
	auto app = static_cast<Application*>(glfwGetWindowUserPointer(window));

	app->fastCamera((mods & GLFW_MOD_SHIFT) == GLFW_MOD_SHIFT);

	switch (key)
	{
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

void onMouseMove(GLFWwindow* window, double xpos, double ypos)
{
	auto app = static_cast<Application*>(glfwGetWindowUserPointer(window));

	app->updateMousePos(xpos, ypos);
}

void onMouseButton(GLFWwindow* window, int button, int action, int)
{
	auto app = static_cast<Application*>(glfwGetWindowUserPointer(window));

	if (button == GLFW_MOUSE_BUTTON_RIGHT)
	{
		if (action != GLFW_RELEASE)
		{
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			app->mouseGrab(true);
		}
		else
		{
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			app->mouseGrab(false);
		}
	}
}

void windowResize(GLFWwindow* window, int width, int height)
{
	auto app = static_cast<Application*>(glfwGetWindowUserPointer(window));

	if (width > 0 && height > 0)
	{
		app->setProjection(glm::perspective(
			glm::radians(60.0f), (float)width / height, 0.1f, 100.0f));

		glViewport(0, 0, width, height);
	}
}

int main()
{
	Application app("Cook-Torrance", WINDOW_WIDTH, WINDOW_HEIGHT);

	if (app.init())
	{
		app.run();
		app.destroy();
	}
}

