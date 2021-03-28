#include "../common/baseApp.hpp"
#include "../common/flyThroughCamera.hpp"
#include "../common/objParser.hpp"
#include "../common/texture.hpp"
#include "../common/renderbuffer.hpp"
#include "../common/framebuffer.hpp"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#define WINDOW_WIDTH 1366
#define WINDOW_HEIGHT 768

#define N_TEXTURES 4

#define FBO_WIDTH 512
#define FBO_HEIGHT 512

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
	struct StandardPBRProgram
	{
		GLuint id;

		GLint u_model_matrix_loc;
		GLint u_view_matrix_loc;
		GLint u_projection_matrix_loc;
		GLint u_nor_transform_loc;

		GLint u_irrandiance_sampler_loc;

		GLint u_color_sampler_loc;
		GLint u_normal_sampler_loc;

		GLint u_has_metallic_map_loc;
		GLint u_metallic_sampler_loc;
		GLint u_metallic_loc;

		GLint u_has_roughness_map_loc;
		GLint u_roughness_sampler_loc;
		GLint u_roughness_loc;

		GLint u_dir_light_direction_loc;
		GLint u_dir_light_color_loc;

		GLint u_view_pos_loc;

		GLint u_bump_map_active_loc;

		GLint u_gamma_loc;
		GLint u_exposure_loc;
	};

	struct ToCubeMapProgram
	{
		GLuint id;

		GLint u_view_matrix_loc;
		GLint u_projection_matrix_loc;

		GLint u_env_map_sampler_loc;
	};

	struct SkyboxProgram
	{
		GLint id;

		GLint u_view_matrix_loc;
		GLint u_projection_matrix_loc;

		GLint u_cube_sampler_loc;

		GLint u_gamma_loc;
		GLint u_exposure_loc;
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

		if (!createStandardPBRProgram() ||
			!createToCubeMapProgram() ||
			!createSkyboxProgram() ||
			!createGeometry() ||
			!createCube())
		{
			return false;
		}

		createEnvironmentCubeMap();
		createTextures();

		windowResize(window, window_width, window_height);

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

		standardPBR(camera_position, view_matrix);

		glBindVertexArray(geometry.vao_id);
		glDrawElements(GL_TRIANGLES, geometry.n_indices, GL_UNSIGNED_INT, nullptr);

		glDepthFunc(GL_LEQUAL);

		glUseProgram(skybox_program.id);

		glUniformMatrix4fv(skybox_program.u_view_matrix_loc,
			1, GL_FALSE, glm::value_ptr(
				glm::mat4(glm::mat3(view_matrix))));
		glUniformMatrix4fv(skybox_program.u_projection_matrix_loc,
			1, GL_FALSE, glm::value_ptr(projection));

		glUniform1i(skybox_program.u_cube_sampler_loc, skybox_sampler_unit);

		glUniform1f(skybox_program.u_gamma_loc, gamma_correction);
		glUniform1f(skybox_program.u_exposure_loc, exposure);

		glBindVertexArray(cube.vao_id);
		glDrawElements(GL_TRIANGLES, cube.n_indices, GL_UNSIGNED_INT, nullptr);

		glDepthFunc(GL_LESS);

		return true;
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

		glUniform1i(standard_pbr.u_irrandiance_sampler_loc, 1);

		glUniform1i(standard_pbr.u_color_sampler_loc, 2);
		glUniform1i(standard_pbr.u_normal_sampler_loc, 3);

		glUniform1i(standard_pbr.u_has_metallic_map_loc, has_metallic_map);
		glUniform1i(standard_pbr.u_metallic_sampler_loc, 4);
		glUniform1f(standard_pbr.u_metallic_loc, metallic);

		glUniform1i(standard_pbr.u_has_roughness_map_loc, has_roughness_map);
		glUniform1i(standard_pbr.u_roughness_sampler_loc, 5);
		glUniform1f(standard_pbr.u_roughness_loc, roughness);

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
		if (glIsProgram(standard_pbr.id))
		{
			glDeleteProgram(standard_pbr.id);
		}

		if (glIsProgram(to_cube_map.id))
		{
			glDeleteProgram(to_cube_map.id);
		}

		if (glIsProgram(skybox_program.id))
		{
			glDeleteProgram(skybox_program.id);
		}

		env_cube_texture->destroy();
		delete env_cube_texture;

		irr_cube_texture->destroy();
		delete irr_cube_texture;

		for (int i = 0; i < N_TEXTURES; ++i)
		{
			textures[i].destroy();
		}

		gl.destroyGeometry(geometry);
		gl.destroyGeometry(cube);
	}

	void buildGUI()
	{
		using namespace ImGui;

		/// OBJECT
		Begin("Material");

		Dummy(ImVec2(0.0f, 2.0f));
		Separator();

		Checkbox("Normal map", &bump_map_active);

		Dummy(ImVec2(0.0f, 2.0f));
		Separator();

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

		End();

		/// SKYBOX
		Begin("Skybox");

		Dummy(ImVec2(0.0f, 2.0f));
		Separator();

		BeginGroup();
		RadioButton("Environment map", &skybox_sampler_unit, 0);
		RadioButton("Irradiance map", &skybox_sampler_unit, 1);
		EndGroup();

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

		standard_pbr.u_irrandiance_sampler_loc =
			glGetUniformLocation(standard_pbr.id, "u_irradiance_sampler");

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
		assert(standard_pbr.u_irrandiance_sampler_loc != -1);
		assert(standard_pbr.u_color_sampler_loc != -1);
		assert(standard_pbr.u_normal_sampler_loc != -1);
		assert(standard_pbr.u_has_metallic_map_loc != -1);
		assert(standard_pbr.u_metallic_sampler_loc != -1);
		assert(standard_pbr.u_metallic_loc != -1);
		assert(standard_pbr.u_has_roughness_map_loc != -1);
		assert(standard_pbr.u_roughness_sampler_loc != -1);
		assert(standard_pbr.u_roughness_loc != -1);
		assert(standard_pbr.u_dir_light_color_loc != -1);
		assert(standard_pbr.u_view_pos_loc != -1);
		assert(standard_pbr.u_bump_map_active_loc != -1);
		assert(standard_pbr.u_gamma_loc != -1);
		assert(standard_pbr.u_exposure_loc != -1);

		return true;
	}

	bool createToCubeMapProgram()
	{
		std::vector<ShaderInfo> shaders;

		std::ifstream vs_file("shaders/toCubeMap/vs.glsl");
		std::ifstream fs_file("shaders/toCubeMap/fs.glsl");

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

		std::cout << "Creating ToCubeMap program ... ";

		readShader(vs_file, fs_file, shaders);

		bool success;

		to_cube_map.id = gl.createProgram(shaders, success);

		if (!success)
		{
			return false;
		}
		else
		{
			std::cout << "SUCCESS\n";
		}

		to_cube_map.u_view_matrix_loc =
			glGetUniformLocation(to_cube_map.id, "u_view_matrix");
		to_cube_map.u_projection_matrix_loc =
			glGetUniformLocation(to_cube_map.id, "u_projection_matrix");

		to_cube_map.u_env_map_sampler_loc =
			glGetUniformLocation(to_cube_map.id, "u_env_map_sampler");

		assert(to_cube_map.u_view_matrix_loc != -1);
		assert(to_cube_map.u_projection_matrix_loc != -1);
		assert(to_cube_map.u_env_map_sampler_loc != -1);

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

		std::cout << "Creating ToCubeMap program ... ";

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

		skybox_program.u_gamma_loc =
			glGetUniformLocation(skybox_program.id, "u_gamma");
		skybox_program.u_exposure_loc =
			glGetUniformLocation(skybox_program.id, "u_exposure");

		assert(skybox_program.u_view_matrix_loc != -1);
		assert(skybox_program.u_projection_matrix_loc != -1);
		assert(skybox_program.u_cube_sampler_loc != -1);
		assert(skybox_program.u_gamma_loc != -1);
		assert(skybox_program.u_exposure_loc != -1);

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

		textures[2] = Texture2D("../res/materialBall/metallic.png", 1,
			GL_REPEAT, GL_REPEAT, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);

		textures[3] = Texture2D("../res/materialBall/roughness.png", 3,
			GL_REPEAT, GL_REPEAT, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);

		for (int i = 0; i < N_TEXTURES; ++i)
		{
			textures[i].bind(i + 2);
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
			standard_pbr.id, f_buffers, i_buffers, indices, success);

		if (!success)
		{
			return false;
		}

		return true;
	}

	bool createCube()
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

		cube = gl.createPackedStaticGeometry(
			to_cube_map.id, f_buffers, i_buffers, indices, success);

		if (!success)
		{
			return false;
		}

		return true;
	}

	void createEnvironmentCubeMap()
	{
		std::cout << "\nCreating environment cube map ... ";

		env_cube_texture = new Empty16FTextureCube(
			FBO_WIDTH,
			FBO_HEIGHT,
			3,
			GL_CLAMP_TO_EDGE,
			GL_CLAMP_TO_EDGE,
			GL_CLAMP_TO_EDGE);

		irr_cube_texture = new Empty16FTextureCube(
			FBO_WIDTH,
			FBO_HEIGHT,
			3,
			GL_CLAMP_TO_EDGE,
			GL_CLAMP_TO_EDGE,
			GL_CLAMP_TO_EDGE);

		env_source_texture = new TextureHDREnvironment(
			"../res/environmentMaps/gravelPlaza.hdr",
			GL_CLAMP_TO_EDGE,
			GL_CLAMP_TO_EDGE,
			GL_LINEAR,
			GL_LINEAR,
			true);

		irr_source_texture = new TextureHDREnvironment(
			"../res/environmentMaps/gravelPlazaIrradiance.hdr",
			GL_CLAMP_TO_EDGE,
			GL_CLAMP_TO_EDGE,
			GL_LINEAR,
			GL_LINEAR,
			true);

		Renderbuffer env_renderbuffer(GL_DEPTH_COMPONENT24, FBO_WIDTH, FBO_HEIGHT);
		Framebuffer env_framebuffer;

		env_framebuffer.attachRenderbuffer(GL_DEPTH_ATTACHMENT, env_renderbuffer);

		glm::mat4 env_projection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
		glm::mat4 env_views[]
		{
			glm::lookAt(
				glm::vec3(0.0f, 0.0f, 0.0f),
				glm::vec3(1.0f, 0.0f, 0.0f),
				glm::vec3(0.0f, -1.0f, 0.0f)),

			glm::lookAt(
				glm::vec3(0.0f, 0.0f, 0.0f),
				glm::vec3(-1.0f, 0.0f, 0.0f),
				glm::vec3(0.0f, -1.0f, 0.0f)),

			glm::lookAt(
				glm::vec3(0.0f, 0.0f, 0.0f),
				glm::vec3(0.0f, 1.0f, 0.0f),
				glm::vec3(0.0f, 0.0f, 1.0f)),

			glm::lookAt(
				glm::vec3(0.0f, 0.0f, 0.0f),
				glm::vec3(0.0f, -1.0f, 0.0f),
				glm::vec3(0.0f, 0.0f, -1.0f)),

			glm::lookAt(
				glm::vec3(0.0f, 0.0f, 0.0f),
				glm::vec3(0.0f, 0.0f, 1.0f),
				glm::vec3(0.0f, -1.0f, 0.0f)),

			glm::lookAt(
				glm::vec3(0.0f, 0.0f, 0.0f),
				glm::vec3(0.0f, 0.0f, -1.0f),
				glm::vec3(0.0f, -1.0f, 0.0f))
		};

		env_source_texture->bind(0);
		irr_source_texture->bind(1);

		glUseProgram(to_cube_map.id);

		glUniformMatrix4fv(to_cube_map.u_projection_matrix_loc,
			1, GL_FALSE, glm::value_ptr(env_projection));

		glViewport(0, 0, FBO_WIDTH, FBO_HEIGHT);

		env_framebuffer.bind();

		for (unsigned i = 0; i < 6; ++i)
		{
			glUniformMatrix4fv(to_cube_map.u_view_matrix_loc,
				1, GL_FALSE, glm::value_ptr(env_views[i]));
			glUniform1i(to_cube_map.u_env_map_sampler_loc, 0);

			env_framebuffer.attachCubeMapTexture(
				GL_COLOR_ATTACHMENT0, *env_cube_texture, 0, i);

			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			glBindVertexArray(cube.vao_id);
			glDrawElements(GL_TRIANGLES, cube.n_indices, GL_UNSIGNED_INT, nullptr);

			glUniform1i(to_cube_map.u_env_map_sampler_loc, 1);

			env_framebuffer.attachCubeMapTexture(
				GL_COLOR_ATTACHMENT0, *irr_cube_texture, 0, i);

			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			glBindVertexArray(cube.vao_id);
			glDrawElements(GL_TRIANGLES, cube.n_indices, GL_UNSIGNED_INT, nullptr);
		}

		Framebuffer::bindDefault();

		env_cube_texture->bind(0);
		irr_cube_texture->bind(1);

		env_source_texture->destroy();
		delete env_source_texture;

		irr_source_texture->destroy();
		delete irr_source_texture;

		std::cout << "DONE\n\n";
	}

	/// Environment cube map
	Empty16FTextureCube* env_cube_texture;
	Empty16FTextureCube* irr_cube_texture;
	TextureHDREnvironment* env_source_texture;
	TextureHDREnvironment* irr_source_texture;

	/// Programs
	StandardPBRProgram standard_pbr;
	ToCubeMapProgram to_cube_map;
	SkyboxProgram skybox_program;

	/// Material
	Texture textures[N_TEXTURES];

	float shininess = 32.0f;
	bool bump_map_active = true;

	bool has_metallic_map = false;
	float metallic = 0.0f;
	bool has_roughness_map = false;
	float roughness = 0.05f;

	/// Skybox
	int skybox_sampler_unit = 0;

	/// Object properties
	DeviceMesh geometry;
	DeviceMesh cube;
	glm::mat4 model_matrix;

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
	Application app("CubeMaps", WINDOW_WIDTH, WINDOW_HEIGHT);

	if (app.init())
	{
		app.run();
		app.destroy();
	}
}

