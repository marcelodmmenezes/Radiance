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
		dir_light.direction = glm::vec3(-1.0f, -1.0f, -1.0f);
		dir_light.color = glm::vec3(1.0f, 1.0f, 1.0f);

		camera = FlyThroughCamera(glm::vec3(-2.0f, 1.2f, 0.0f), -90.0f, -30.0f);

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
	struct Program
	{
		GLuint id;

		GLint u_model_matrix_loc;
		GLint u_view_matrix_loc;
		GLint u_projection_matrix_loc;
		GLint u_nor_transform_loc;

		GLint u_sampler_loc;

		GLint u_dir_light_direction_loc;
		GLint u_dir_light_color_loc;

		GLint u_view_pos_loc;
		GLint u_shininess_loc;

		GLint u_uv_multiplier_loc;
		GLint u_has_3_channels_loc;
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

		if (!createProgram())
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
		if (OpenGLContext::checkErrors(__FILE__, __LINE__))
		{
			return false;
		}

		buildGUI();
		updateCamera(delta_time);

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glUseProgram(program.id);

		adjustTextureProperties();

		assert(program.u_model_matrix_loc != -1);
		assert(program.u_view_pos_loc != -1);
		assert(program.u_projection_matrix_loc != -1);
		assert(program.u_nor_transform_loc != -1);
		assert(program.u_sampler_loc != -1);
		assert(program.u_dir_light_color_loc != -1);
		assert(program.u_view_pos_loc != -1);
		assert(program.u_shininess_loc != -1);
		assert(program.u_uv_multiplier_loc != -1);
		assert(program.u_has_3_channels_loc != -1);

		model_matrix = glm::rotate(glm::mat4(1.0f), glm::radians(x_angle),
			glm::vec3(-1.0f, 0.0f, 0.0f));

		glUniformMatrix4fv(program.u_model_matrix_loc,
			1, GL_FALSE, glm::value_ptr(model_matrix));
		glUniformMatrix4fv(program.u_view_matrix_loc,
			1, GL_FALSE, glm::value_ptr(camera.getViewMatrix()));
		glUniformMatrix4fv(program.u_projection_matrix_loc,
			1, GL_FALSE, glm::value_ptr(projection));
		glUniformMatrix3fv(program.u_nor_transform_loc,
			1, GL_FALSE, glm::value_ptr(glm::mat3(
				glm::transpose(glm::inverse(model_matrix)))));

		glUniform1i(program.u_sampler_loc, 0);

		glUniform3fv(program.u_dir_light_direction_loc,
			1, glm::value_ptr(dir_light.direction));
		glUniform3fv(program.u_dir_light_color_loc,
			1, glm::value_ptr(dir_light.color));

		glUniform3fv(program.u_view_pos_loc,
			1, glm::value_ptr(camera.new_position));

		glUniform1f(program.u_shininess_loc, material_shineness);

		glUniform1f(program.u_uv_multiplier_loc, uv_multiplier);

		glUniform1i(program.u_has_3_channels_loc, active_texture);

		glBindVertexArray(device_mesh.vao_id);
		glDrawElements(GL_TRIANGLES, device_mesh.n_indices, GL_UNSIGNED_INT, nullptr);

		return true;
	}

	void customDestroy() override
	{
		if (glIsProgram(program.id))
		{
			glDeleteProgram(program.id);
		}

		texture[0].destroy();
		texture[1].destroy();

		gl.destroyGeometry(device_mesh);
	}

	void buildGUI()
	{
		using namespace ImGui;

		/// TEXTURE
		Begin("Texture");

		BeginGroup();
		RadioButton("Chess", &active_texture, 0);
		RadioButton("Mipmap vis", &active_texture, 1);
		EndGroup();

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
		SliderFloat("X axis angle", &x_angle, 0.0f, 180.0f);
		SliderFloat("UV multiplier", &uv_multiplier, 1.0f, 200.0f);
		SliderFloat("Shineness", &material_shineness, 1.0f, 64.0f);
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
		texture[active_texture].bind(0);

		switch (mag_filter)
		{
			case 0:
				texture[active_texture].setParameteri(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
				break;

			case 1:
				texture[active_texture].setParameteri(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				break;
		}

		switch (min_filter)
		{
			case 0:
				texture[active_texture].setParameteri(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				break;

			case 1:
				texture[active_texture].setParameteri(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				break;

			case 2:
				texture[active_texture].setParameteri(GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
				break;

			case 3:
				texture[active_texture].setParameteri(GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
				break;

			case 4:
				texture[active_texture].setParameteri(GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
				break;

			case 5:
				texture[active_texture].setParameteri(GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
				break;
		}

		switch (texture_wrap_s)
		{
#define CASE(id, param) \
	case id: \
		texture[active_texture].setParameteri(GL_TEXTURE_WRAP_S, param); \
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
		texture[active_texture].setParameteri(GL_TEXTURE_WRAP_T, param); \
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

	bool createProgram()
	{
		std::vector<ShaderInfo> shaders;

		std::ifstream vs_file("shaders/vs.glsl");
		std::ifstream fs_file("shaders/fs.glsl");

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

		program.id = gl.createProgram(shaders, success);

		if (!success)
		{
			return false;
		}
		else
		{
			std::cout << "SUCCESS\n";
		}

		program.u_model_matrix_loc =
			glGetUniformLocation(program.id, "u_model_matrix");
		program.u_view_matrix_loc =
			glGetUniformLocation(program.id, "u_view_matrix");
		program.u_projection_matrix_loc =
			glGetUniformLocation(program.id, "u_projection_matrix");
		program.u_nor_transform_loc =
			glGetUniformLocation(program.id, "u_nor_transform");

		program.u_sampler_loc =
			glGetUniformLocation(program.id, "u_sampler");

		program.u_dir_light_direction_loc =
			glGetUniformLocation(program.id, "u_dir_light.direction");
		program.u_dir_light_color_loc =
			glGetUniformLocation(program.id, "u_dir_light.color");

		program.u_view_pos_loc =
			glGetUniformLocation(program.id, "u_view_pos");

		program.u_shininess_loc =
			glGetUniformLocation(program.id, "u_shininess");

		program.u_uv_multiplier_loc =
			glGetUniformLocation(program.id, "u_uv_multiplier");
		program.u_has_3_channels_loc =
			glGetUniformLocation(program.id, "u_has_3_channels");

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
		unsigned char t1_level_0[]
		{
			0, 0, 0, 0, 255, 255, 255, 255,
			0, 0, 0, 0, 255, 255, 255, 255,
			0, 0, 0, 0, 255, 255, 255, 255,
			0, 0, 0, 0, 255, 255, 255, 255,

			255, 255, 255, 255, 0, 0, 0, 0,
			255, 255, 255, 255, 0, 0, 0, 0,
			255, 255, 255, 255, 0, 0, 0, 0,
			255, 255, 255, 255, 0, 0, 0, 0
		};

		unsigned char t1_level_1[]
		{
			0, 0, 255, 255,
			0, 0, 255, 255,

			255, 255, 0, 0,
			255, 255, 0, 0,
		};

		unsigned char t1_level_2[]
		{
			0, 255,

			255, 0
		};

		unsigned char t1_level_3[]
		{
			127
		};

		std::vector<unsigned char*> data(4);

		data[0] = (unsigned char*)t1_level_0;
		data[1] = (unsigned char*)t1_level_1;
		data[2] = (unsigned char*)t1_level_2;
		data[3] = (unsigned char*)t1_level_3;

		texture[0] = Texture2D(data, 8, 8, GL_R8, GL_RED,
			GL_REPEAT, GL_REPEAT, GL_NEAREST_MIPMAP_LINEAR, GL_NEAREST);

		texture[0].bind(0);

		unsigned char t2_level_0[]
		{
			200, 0, 0, 200, 0, 0, 200, 0, 0, 200, 0, 0, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
			200, 0, 0, 200, 0, 0, 200, 0, 0, 200, 0, 0, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
			200, 0, 0, 200, 0, 0, 200, 0, 0, 200, 0, 0, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
			200, 0, 0, 200, 0, 0, 200, 0, 0, 200, 0, 0, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,

			255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 200, 0, 0, 200, 0, 0, 200, 0, 0, 200, 0, 0,
			255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 200, 0, 0, 200, 0, 0, 200, 0, 0, 200, 0, 0,
			255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 200, 0, 0, 200, 0, 0, 200, 0, 0, 200, 0, 0,
			255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 200, 0, 0, 200, 0, 0, 200, 0, 0, 200, 0, 0
		};

		unsigned char t2_level_1[]
		{
			0, 200, 0, 0, 200, 0, 255, 255, 255, 255, 255, 255,
			0, 200, 0, 0, 200, 0, 255, 255, 255, 255, 255, 255,

			255, 255, 255, 255, 255, 255, 0, 200, 0, 0, 200, 0,
			255, 255, 255, 255, 255, 255, 0, 200, 0, 0, 200, 0,
		};

		unsigned char t2_level_2[]
		{
			0, 0, 255, 255, 255, 255,

			255, 255, 255, 0, 0, 255
		};

		unsigned char t2_level_3[]
		{
			127, 127, 127
		};

		data[0] = (unsigned char*)t2_level_0;
		data[1] = (unsigned char*)t2_level_1;
		data[2] = (unsigned char*)t2_level_2;
		data[3] = (unsigned char*)t2_level_3;

		texture[1] = Texture2D(data, 8, 8, GL_RGB8, GL_RGB,
			GL_REPEAT, GL_REPEAT, GL_NEAREST_MIPMAP_LINEAR, GL_NEAREST);
	}

	bool createGeometry()
	{
		std::vector<BufferInfo<float>> f_buffers
		{
			{
				"a_pos",
				3,
				std::vector<float>
				{
					-1.0f, -1.0f, 0.0f,
					1.0f, -1.0f, 0.0f,
					1.0f, 1.0f, 0.0f,
					-1.0f, 1.0f, 0.0f
				}
			},
			{
				"a_nor",
				3,
				std::vector<float>
				{
					0.0f, 0.0f, 1.0f,
					0.0f, 0.0f, 1.0f,
					0.0f, 0.0f, 1.0f,
					0.0f, 0.0f, 1.0f
				}
			},
			{
				"a_tex",
				2,
				std::vector<float>
				{
					0.0f, 0.0f,
					1.0f, 0.0f,
					1.0f, 1.0f,
					0.0f, 1.0f
				}
			}
		};

		std::vector<BufferInfo<int>> i_buffers;

		std::vector<unsigned> indices
		{
			0, 1, 2, 0, 2, 3
		};

		bool success;

		device_mesh = gl.createPackedStaticGeometry(
			program.id, f_buffers,
			i_buffers, indices, success);

		if (!success)
		{
			return false;
		}

		return true;
	}

	/// Programs
	Program program;

	/// Material
	float material_shineness = 32.0f;

	/// Texture
	int active_texture = 0;
	Texture2D texture[2];

	int mag_filter = 0;
	int min_filter = 4;
	int texture_wrap_s = 3;
	int texture_wrap_t = 3;
	float texture_angle = 0.0f;
	float uv_multiplier = 10.0f;

	/// Object properties
	DeviceMesh device_mesh;
	glm::mat4 model_matrix;
	float x_angle = 90.0f;

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
	Application app("Classic Model", WINDOW_WIDTH, WINDOW_HEIGHT);

	if (app.init())
	{
		app.run();
		app.destroy();
	}
}

