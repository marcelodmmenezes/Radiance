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

#define N_TEXTURES 3

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
		dir_light.direction = glm::vec3(0.4f, -1.0f, -0.85f);
		dir_light.color = glm::vec3(1.0f, 1.0f, 1.0f);

		camera = FlyThroughCamera(glm::vec3(-1.0f, 0.5f, 4.0f), -20.0f, -10.0f);

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
	struct GeometryProgram
	{
		GLuint id;

		GLint u_model_matrix_loc;
		GLint u_view_matrix_loc;
		GLint u_projection_matrix_loc;
		GLint u_nor_transform_loc;

		GLint u_color_sampler_loc;
		GLint u_normal_sampler_loc;
		GLint u_cube_sampler_loc;

		GLint u_dir_light_direction_loc;
		GLint u_dir_light_color_loc;

		GLint u_view_pos_loc;
		GLint u_shininess_loc;

		GLint u_uv_multiplier_loc;
		GLint u_bump_map_active_loc;

		GLint u_diffuse_loc;
		GLint u_reflection_loc;
		GLint u_refraction_loc;
	};

	struct SkyboxProgram
	{
		GLint id;

		GLint u_view_matrix_loc;
		GLint u_projection_matrix_loc;

		GLint u_cube_sampler_loc;
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

		if (!createGeometryProgram())
		{
			return false;
		}

		if (!createSkyboxProgram())
		{
			return false;
		}

		createTextures();

		if (!createGeometry())
		{
			return false;
		}

		if (!createSkybox())
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
		if (OpenGLContext::checkErrors(__FILE__, __LINE__))
		{
			return false;
		}

		buildGUI();
		updateCamera(delta_time);

		glm::vec3 camera_position = camera.new_position;
		glm::mat4 view_matrix = camera.getViewMatrix();

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glUseProgram(geometry_program.id);

		adjustTextureProperties();

		glUniformMatrix4fv(geometry_program.u_model_matrix_loc,
			1, GL_FALSE, glm::value_ptr(model_matrix));
		glUniformMatrix4fv(geometry_program.u_view_matrix_loc,
			1, GL_FALSE, glm::value_ptr(view_matrix));
		glUniformMatrix4fv(geometry_program.u_projection_matrix_loc,
			1, GL_FALSE, glm::value_ptr(projection));
		glUniformMatrix3fv(geometry_program.u_nor_transform_loc,
			1, GL_FALSE, glm::value_ptr(glm::mat3(
				glm::transpose(glm::inverse(model_matrix)))));

		glUniform1i(geometry_program.u_color_sampler_loc, 0);
		glUniform1i(geometry_program.u_normal_sampler_loc, 1);
		glUniform1i(geometry_program.u_cube_sampler_loc, 2);

		glUniform3fv(geometry_program.u_dir_light_direction_loc,
			1, glm::value_ptr(dir_light.direction));
		glUniform3fv(geometry_program.u_dir_light_color_loc,
			1, glm::value_ptr(dir_light.color));

		glUniform3fv(geometry_program.u_view_pos_loc,
			1, glm::value_ptr(camera_position));

		glUniform1f(geometry_program.u_shininess_loc, material_shineness);

		glUniform1f(geometry_program.u_uv_multiplier_loc, uv_multiplier);

		glUniform1f(geometry_program.u_bump_map_active_loc, bump_map_active);

		glUniform1f(geometry_program.u_diffuse_loc, diffuse);
		glUniform1f(geometry_program.u_reflection_loc, reflection);
		glUniform1f(geometry_program.u_refraction_loc, refraction);

		glBindVertexArray(geometry.vao_id);
		glDrawElements(GL_TRIANGLES, geometry.n_indices, GL_UNSIGNED_INT, nullptr);

		glDepthFunc(GL_LEQUAL);

		glUseProgram(skybox_program.id);

		glUniformMatrix4fv(skybox_program.u_view_matrix_loc,
			1, GL_FALSE, glm::value_ptr(
				glm::mat4(glm::mat3(view_matrix))));
		glUniformMatrix4fv(skybox_program.u_projection_matrix_loc,
			1, GL_FALSE, glm::value_ptr(projection));

		glUniform1i(skybox_program.u_cube_sampler_loc, 2);

		glBindVertexArray(skybox.vao_id);
		glDrawElements(GL_TRIANGLES, skybox.n_indices, GL_UNSIGNED_INT, nullptr);

		glDepthFunc(GL_LESS);

		return true;
	}

	void customDestroy() override
	{
		if (glIsProgram(geometry_program.id))
		{
			glDeleteProgram(geometry_program.id);
		}

		if (glIsProgram(skybox_program.id))
		{
			glDeleteProgram(skybox_program.id);
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

		/// TEXTURE
		Begin("Bump map");

		Checkbox("Active", &bump_map_active);

		Text("Magnification filter");
		BeginGroup();
		RadioButton("Mag Nearest", &mag_filter, 0);
		SameLine();
		RadioButton("Mag Linear", &mag_filter, 1);
		EndGroup();

		Text("Minification filter");
		BeginGroup();
		RadioButton("Min Nearest", &min_filter, 0);
		SameLine();
		RadioButton("Min Linear", &min_filter, 1);
		RadioButton("Min Nearest | Mipmap Nearest", &min_filter, 2);
		SameLine();
		RadioButton("Min Nearest | Mipmap Linear", &min_filter, 3);
		RadioButton("Min Linear | Mipmap Nearest", &min_filter, 4);
		SameLine();
		RadioButton("Min Linear | Mipmap Linear", &min_filter, 5);
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

		/// OBJECT
		Begin("Object");
		SliderFloat("UV multiplier", &uv_multiplier, 1.0f, 200.0f);
		SliderFloat("Shineness", &material_shineness, 1.0f, 64.0f);
		SliderFloat("Diffuse contribution", &diffuse, 0.0f, 1.0f);
		SliderFloat("Reflection contribution", &reflection, 0.0f, 1.0f);
		SliderFloat("Refraction contribution", &refraction, 0.0f, 1.0f);
		End();

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

	void adjustTextureProperties()
	{
		switch (mag_filter)
		{
			case 0:
				textures[1].setParameteri(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
				break;

			case 1:
				textures[1].setParameteri(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				break;
		}

		switch (min_filter)
		{
			case 0:
				textures[1].setParameteri(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				break;

			case 1:
				textures[1].setParameteri(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				break;

			case 2:
				textures[1].setParameteri(GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
				break;

			case 3:
				textures[1].setParameteri(GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
				break;

			case 4:
				textures[1].setParameteri(GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
				break;

			case 5:
				textures[1].setParameteri(GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
				break;
		}

		switch (texture_wrap_s)
		{
#define CASE(id, param) \
	case id: \
		textures[1].setParameteri(GL_TEXTURE_WRAP_S, param); \
		break
			
			CASE(0, GL_CLAMP_TO_EDGE);
			CASE(1, GL_CLAMP_TO_BORDER);
			CASE(2, GL_MIRRORED_REPEAT);
			CASE(3, GL_REPEAT);
			CASE(4, GL_MIRROR_CLAMP_TO_EDGE);

#undef CASE
		}

		switch (texture_wrap_t)
		{
#define CASE(id, param) \
	case id: \
		textures[1].setParameteri(GL_TEXTURE_WRAP_T, param); \
		break
			
			CASE(0, GL_CLAMP_TO_EDGE);
			CASE(1, GL_CLAMP_TO_BORDER);
			CASE(2, GL_MIRRORED_REPEAT);
			CASE(3, GL_REPEAT);
			CASE(4, GL_MIRROR_CLAMP_TO_EDGE);

#undef CASE
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

	bool createGeometryProgram()
	{
		std::vector<ShaderInfo> shaders;

		std::ifstream vs_file("shaders/geometry/vs.glsl");
		std::ifstream fs_file("shaders/geometry/fs.glsl");

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

		std::cout << "Creating program ... ";

		readShader(vs_file, fs_file, shaders);

		bool success;

		geometry_program.id = gl.createProgram(shaders, success);

		if (!success)
		{
			return false;
		}
		else
		{
			std::cout << "SUCCESS\n";
		}

		geometry_program.u_model_matrix_loc =
			glGetUniformLocation(geometry_program.id, "u_model_matrix");
		geometry_program.u_view_matrix_loc =
			glGetUniformLocation(geometry_program.id, "u_view_matrix");
		geometry_program.u_projection_matrix_loc =
			glGetUniformLocation(geometry_program.id, "u_projection_matrix");
		geometry_program.u_nor_transform_loc =
			glGetUniformLocation(geometry_program.id, "u_nor_transform");

		geometry_program.u_color_sampler_loc =
			glGetUniformLocation(geometry_program.id, "u_color_sampler");

		geometry_program.u_normal_sampler_loc =
			glGetUniformLocation(geometry_program.id, "u_normal_sampler");

		geometry_program.u_cube_sampler_loc =
			glGetUniformLocation(geometry_program.id, "u_cube_sampler");

		geometry_program.u_dir_light_direction_loc =
			glGetUniformLocation(geometry_program.id, "u_dir_light.direction");
		geometry_program.u_dir_light_color_loc =
			glGetUniformLocation(geometry_program.id, "u_dir_light.color");

		geometry_program.u_view_pos_loc =
			glGetUniformLocation(geometry_program.id, "u_view_pos");

		geometry_program.u_shininess_loc =
			glGetUniformLocation(geometry_program.id, "u_shininess");

		geometry_program.u_uv_multiplier_loc =
			glGetUniformLocation(geometry_program.id, "u_uv_multiplier");

		geometry_program.u_bump_map_active_loc =
			glGetUniformLocation(geometry_program.id, "u_bump_map_active");

		geometry_program.u_diffuse_loc =
			glGetUniformLocation(geometry_program.id, "u_diffuse");
		geometry_program.u_reflection_loc =
			glGetUniformLocation(geometry_program.id, "u_reflection");
		geometry_program.u_refraction_loc =
			glGetUniformLocation(geometry_program.id, "u_refraction");

		assert(geometry_program.u_model_matrix_loc != -1);
		assert(geometry_program.u_view_pos_loc != -1);
		assert(geometry_program.u_projection_matrix_loc != -1);
		assert(geometry_program.u_nor_transform_loc != -1);
		assert(geometry_program.u_color_sampler_loc != -1);
		assert(geometry_program.u_normal_sampler_loc != -1);
		assert(geometry_program.u_cube_sampler_loc != -1);
		assert(geometry_program.u_dir_light_color_loc != -1);
		assert(geometry_program.u_view_pos_loc != -1);
		assert(geometry_program.u_shininess_loc != -1);
		assert(geometry_program.u_uv_multiplier_loc != -1);
		assert(geometry_program.u_bump_map_active_loc != -1);
		assert(geometry_program.u_diffuse_loc != -1);
		assert(geometry_program.u_reflection_loc != -1);
		assert(geometry_program.u_refraction_loc != -1);

		return true;
	}

	bool createSkyboxProgram()
	{
		std::vector<ShaderInfo> shaders;

		std::ifstream vs_file("shaders/skybox/vs.glsl");
		std::ifstream fs_file("shaders/skybox/fs.glsl");

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

		std::cout << "Creating program ... ";

		readShader(vs_file, fs_file, shaders);

		bool success;

		skybox_program.id = gl.createProgram(shaders, success);

		if (!success)
		{
			return false;
		}
		else
		{
			std::cout << "SUCCESS\n";
		}

		skybox_program.u_view_matrix_loc =
			glGetUniformLocation(skybox_program.id, "u_view_matrix");
		skybox_program.u_projection_matrix_loc =
			glGetUniformLocation(skybox_program.id, "u_projection_matrix");

		skybox_program.u_cube_sampler_loc =
			glGetUniformLocation(skybox_program.id, "u_cube_sampler");

		assert(skybox_program.u_view_matrix_loc != -1);
		assert(skybox_program.u_projection_matrix_loc != -1);
		assert(skybox_program.u_cube_sampler_loc != -1);

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
			GL_REPEAT, GL_REPEAT, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);

		textures[1] = Texture2D("../res/materialBall/normal.png", 3,
			GL_REPEAT, GL_REPEAT, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);

		OpenGLContext::checkErrors(__FILE__, __LINE__);

		textures[2] = TextureCube("../res/skybox/saintPeterSquare/", "jpg", 3,
			GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE,
			GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR,
			false);

		OpenGLContext::checkErrors(__FILE__, __LINE__);

		textures[0].bind(0);
		textures[1].bind(1);
		textures[2].bind(2);
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
			geometry_program.id, f_buffers, i_buffers, indices, success);

		if (!success)
		{
			return false;
		}

		return true;
	}

	bool createSkybox()
	{
		std::vector<BufferInfo<float>> f_buffers
		{
			BufferInfo<float>
			{
				"a_pos",
				3,
				std::vector<float>
				{
					-1.0f, -1.0f, -1.0f,
					-1.0f, -1.0f, +1.0f,
					-1.0f, +1.0f, -1.0f,
					-1.0f, +1.0f, +1.0f,
					+1.0f, -1.0f, -1.0f,
					+1.0f, -1.0f, +1.0f,
					+1.0f, +1.0f, -1.0f,
					+1.0f, +1.0f, +1.0f
				}
			}
		};

		std::vector<BufferInfo<int>> i_buffers;

		std::vector<unsigned> indices
		{
			0, 1, 5,
			0, 5, 4,

			1, 3, 7,
			1, 7, 5,

			2, 6, 7,
			2, 7, 3,

			3, 1, 0,
			3, 0, 2,

			4, 5, 7,
			4, 7, 6,

			0, 4, 6,
			0, 6, 2
		};

		bool success;

		skybox = gl.createPackedStaticGeometry(
			skybox_program.id, f_buffers, i_buffers, indices, success);

		if (!success)
		{
			return false;
		}

		return true;
	}

	/// Programs
	GeometryProgram geometry_program;
	SkyboxProgram skybox_program;

	/// Material
	float material_shineness = 32.0f;

	/// Textures
	Texture textures[N_TEXTURES];

	bool bump_map_active = true;

	int mag_filter = 1;
	int min_filter = 5;
	int texture_wrap_s = 3;
	int texture_wrap_t = 3;
	float texture_angle = 0.0f;
	float uv_multiplier = 1.0f;

	/// Object properties
	DeviceMesh geometry;
	DeviceMesh skybox;
	glm::mat4 model_matrix;

	float diffuse = 1.0f;
	float reflection = 0.2f;
	float refraction = 0.0f;

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

	app->setProjection(glm::perspective(
		glm::radians(60.0f), (float)width / height, 0.1f, 100.0f));

	glViewport(0, 0, width, height);
}

int main()
{
	Application app("CubeMaps", WINDOW_WIDTH, WINDOW_HEIGHT);

	if (app.init())
	{
		app.run();
		app.destroy();
	}
}

