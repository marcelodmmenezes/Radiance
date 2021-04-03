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

#define N_ENVIRONMENTS 3
#define N_MATERIAL_TEXTURES 5

#define FBO_ENV_WIDTH 512
#define FBO_ENV_HEIGHT 512
#define FBO_SPEC_WIDTH 128
#define FBO_SPEC_HEIGHT 128

#define BRDF_LUT_WIDTH 512
#define BRDF_LUT_HEIGHT 512

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
		camera = FlyThroughCamera(glm::vec3(0.0f, 1.0f, 4.0f), 0.0f, -20.0f);
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

	void setWindowSize(int width, int height)
	{
		window_width = width;
		window_height = height;

		glViewport(0, 0, window_width, window_height);
	}

private:
	struct StandardPBRProgram
	{
		GLuint id;

		GLint u_model_matrix_loc;
		GLint u_pv_matrix_loc;
		GLint u_nor_transform_loc;

		GLint u_view_pos_loc;

		GLint u_irrandiance_sampler_loc;
		GLint u_specular_sampler_loc;
		GLint u_brdf_lut_sampler_loc;

		GLint u_has_normal_map_loc;
		GLint u_has_ao_map_loc;
		GLint u_has_metallic_map_loc;
		GLint u_has_roughness_map_loc;

		GLint u_albedo_sampler_loc;
		GLint u_normal_sampler_loc;
		GLint u_ao_sampler_loc;
		GLint u_metallic_sampler_loc;
		GLint u_roughness_sampler_loc;

		GLint u_metallic_loc;
		GLint u_roughness_loc;

		GLint u_gamma_loc;
		GLint u_exposure_loc;
	};

	struct IrradianceProgram
	{
		GLuint id;

		GLint u_view_matrix_loc;
		GLint u_projection_matrix_loc;

		GLint u_env_map_sampler_loc;
	};

	struct SpecularMapProgram
	{
		GLuint id;

		GLint u_view_matrix_loc;
		GLint u_projection_matrix_loc;

		GLint u_env_map_sampler_loc;
		GLint u_roughness_loc;
	};

	struct BRDFConvolutionProgram
	{
		GLuint id;
	};

	struct SkyboxProgram
	{
		GLint id;

		GLint u_view_matrix_loc;
		GLint u_projection_matrix_loc;

		GLint u_cube_sampler_loc;

		GLint u_mipmap_level_loc;

		GLint u_gamma_loc;
		GLint u_exposure_loc;
	};

	bool customInit() override
	{
		glfwSetKeyCallback(window, onKey);
		glfwSetCursorPosCallback(window, onMouseMove);
		glfwSetMouseButtonCallback(window, onMouseButton);
		glfwSetWindowSizeCallback(window, windowResize);

		if (!createStandardPBRProgram() ||
			!createIrradianceProgram() ||
			!createSpecularMapProgram() ||
			!createBRDFConvolutionProgram() ||
			!createSkyboxProgram() ||
			!createGeometry() ||
			!createCube() ||
			!createQuad())
		{
			return false;
		}

		createEnvironments();
		createBRDFLUT();
		createTextures();

		windowResize(window, window_width, window_height);

		glClearColor(0.10, 0.25, 0.15, 1.0);

		gl.enable(GL_DEPTH_TEST);
		gl.enable(GL_CULL_FACE);
		gl.enable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

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

		env_cube_texture[current_environment]->bind(0);
		irr_cube_texture[current_environment]->bind(1);
		spec_cube_texture[current_environment]->bind(2);

		drawGeometry(camera_position, view_matrix);
		drawSkybox(view_matrix);

		return true;
	}

	void drawGeometry(
		glm::vec3 const& camera_position,
		glm::mat4 const& view_matrix)
	{
		glUseProgram(standard_pbr.id);

		glUniformMatrix4fv(standard_pbr.u_model_matrix_loc,
			1, GL_FALSE, glm::value_ptr(model_matrix));
		glUniformMatrix4fv(standard_pbr.u_pv_matrix_loc,
			1, GL_FALSE, glm::value_ptr(projection * view_matrix));
		glUniformMatrix3fv(standard_pbr.u_nor_transform_loc,
			1, GL_FALSE, glm::value_ptr(glm::mat3(
				glm::transpose(glm::inverse(model_matrix)))));

		glUniform3fv(standard_pbr.u_view_pos_loc,
			1, glm::value_ptr(camera_position));

		glUniform1i(standard_pbr.u_irrandiance_sampler_loc, 1);
		glUniform1i(standard_pbr.u_specular_sampler_loc, 2);
		glUniform1i(standard_pbr.u_brdf_lut_sampler_loc, 3);

		glUniform1i(standard_pbr.u_has_normal_map_loc, has_normal_map);
		glUniform1i(standard_pbr.u_has_ao_map_loc, has_ao_map);
		glUniform1i(standard_pbr.u_has_metallic_map_loc, has_metallic_map);
		glUniform1i(standard_pbr.u_has_roughness_map_loc, has_roughness_map);

		glUniform1i(standard_pbr.u_albedo_sampler_loc, 4);
		glUniform1i(standard_pbr.u_normal_sampler_loc, 5);
		glUniform1i(standard_pbr.u_ao_sampler_loc, 6);
		glUniform1i(standard_pbr.u_metallic_sampler_loc, 7);
		glUniform1i(standard_pbr.u_roughness_sampler_loc, 8);

		glUniform1f(standard_pbr.u_metallic_loc, metallic);
		glUniform1f(standard_pbr.u_roughness_loc, roughness);

		glUniform1f(standard_pbr.u_gamma_loc, gamma_correction);
		glUniform1f(standard_pbr.u_exposure_loc, exposure);

		glBindVertexArray(geometry.vao_id);
		glDrawElements(GL_TRIANGLES, geometry.n_indices, GL_UNSIGNED_INT, nullptr);
	}

	void drawSkybox(glm::mat4 const& view_matrix)
	{
		glDepthFunc(GL_LEQUAL);

		glUseProgram(skybox_program.id);

		glUniformMatrix4fv(skybox_program.u_view_matrix_loc,
			1, GL_FALSE, glm::value_ptr(
				glm::mat4(glm::mat3(view_matrix))));
		glUniformMatrix4fv(skybox_program.u_projection_matrix_loc,
			1, GL_FALSE, glm::value_ptr(projection));

		glUniform1i(skybox_program.u_cube_sampler_loc, skybox_sampler_unit);
		glUniform1f(skybox_program.u_mipmap_level_loc, skybox_mipmap_level);

		glUniform1f(skybox_program.u_gamma_loc, gamma_correction);
		glUniform1f(skybox_program.u_exposure_loc, exposure);

		glBindVertexArray(cube.vao_id);
		glDrawElements(GL_TRIANGLES, cube.n_indices, GL_UNSIGNED_INT, nullptr);

		glDepthFunc(GL_LESS);
	}

	void customDestroy() override
	{
		glDeleteProgram(standard_pbr.id);
		glDeleteProgram(irradiance_program.id);
		glDeleteProgram(specular_program.id);
		glDeleteProgram(brdf_convolution_program.id);
		glDeleteProgram(skybox_program.id);

		for (int i = 0; i < N_ENVIRONMENTS; ++i)
		{
			env_cube_texture[i]->destroy();
			delete env_cube_texture[i];

			irr_cube_texture[i]->destroy();
			delete irr_cube_texture[i];

			spec_cube_texture[i]->destroy();
			delete spec_cube_texture[i];
		}

		brdf_lut.destroy();

		for (int i = 0; i < N_MATERIAL_TEXTURES; ++i)
		{
			textures[i].destroy();
		}

		gl.destroyGeometry(geometry);
		gl.destroyGeometry(cube);
		gl.destroyGeometry(quad);
	}

	void buildGUI()
	{
		using namespace ImGui;

		/// OBJECT
		Begin("Object properties");

		Text("Rotate");
		SliderFloat("X", &rotation.x, -180.0f, 180.0f);
		SliderFloat("Y", &rotation.y, -180.0f, 180.0f);
		SliderFloat("Z", &rotation.z, -180.0f, 180.0f);

		model_matrix =
			glm::rotate(glm::mat4(1.0f), glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f)) *
			glm::rotate(glm::mat4(1.0f), glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f)) *
			glm::rotate(glm::mat4(1.0f), glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

		Dummy(ImVec2(0.0f, 2.0f));
		Separator();

		Checkbox("Normal Map", &has_normal_map);

		if (has_normal_map)
		{
			Image((void*)(intptr_t)textures[1].getId(), ImVec2(128, 128));
		}

		Checkbox("AO Map", &has_ao_map);

		if (has_ao_map)
		{
			Image((void*)(intptr_t)textures[2].getId(), ImVec2(128, 128));
		}

		Checkbox("Metallic Map", &has_metallic_map);

		if (has_metallic_map)
		{
			Image((void*)(intptr_t)textures[3].getId(), ImVec2(128, 128));
		}
		else
		{
			SliderFloat("Metallic", &metallic, 0.0f, 1.0f);
		}

		Checkbox("Roughness Map", &has_roughness_map);

		if (has_roughness_map)
		{
			Image((void*)(intptr_t)textures[4].getId(), ImVec2(128, 128));
		}
		else
		{
			SliderFloat("Roughness", &roughness, 0.05f, 1.0f);
		}

		Text("BRDF LUT");
		Image((void*)(intptr_t)brdf_lut.getId(), ImVec2(128, 128));

		End();

		/// ENVIRONMENT
		Begin("Environment");

		Text("Map");

		Dummy(ImVec2(0.0f, 2.0f));
		Separator();

		BeginGroup();
		RadioButton("Gravel Plaza", &current_environment, 0);
		RadioButton("Paper Mill", &current_environment, 1);
		RadioButton("Winter Forest", &current_environment, 2);
		EndGroup();

		Dummy(ImVec2(0.0f, 2.0f));
		Separator();

		Text("Skybox");

		BeginGroup();
		RadioButton("Environment map", &skybox_sampler_unit, 0);
		RadioButton("Irradiance map", &skybox_sampler_unit, 1);
		RadioButton("Specular map", &skybox_sampler_unit, 2);
		EndGroup();

		if (skybox_sampler_unit == 2)
		{
			int n_mipmap_levels = floor(std::log2(std::max(FBO_SPEC_WIDTH, FBO_SPEC_HEIGHT)));
			SliderFloat("Mipmap level", &skybox_mipmap_level, 0.0, n_mipmap_levels);
		}

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

		std::cout << "Creating standard pbr program ... ";

		readShader(vs_file, fs_file, shaders);

		bool success;

		standard_pbr.id = gl.createProgram(shaders, success);

		if (!success)
		{
			return false;
		}

		standard_pbr.u_model_matrix_loc =
			glGetUniformLocation(standard_pbr.id, "u_model_matrix");
		standard_pbr.u_pv_matrix_loc =
			glGetUniformLocation(standard_pbr.id, "u_pv_matrix");
		standard_pbr.u_nor_transform_loc =
			glGetUniformLocation(standard_pbr.id, "u_nor_transform");

		standard_pbr.u_view_pos_loc =
			glGetUniformLocation(standard_pbr.id, "u_view_pos");

		standard_pbr.u_irrandiance_sampler_loc =
			glGetUniformLocation(standard_pbr.id, "u_irradiance_sampler");
		standard_pbr.u_specular_sampler_loc =
			glGetUniformLocation(standard_pbr.id, "u_specular_sampler");
		standard_pbr.u_brdf_lut_sampler_loc =
			glGetUniformLocation(standard_pbr.id, "u_brdf_lut_sampler");

		standard_pbr.u_has_normal_map_loc =
			glGetUniformLocation(standard_pbr.id, "u_has_normal_map");
		standard_pbr.u_has_ao_map_loc =
			glGetUniformLocation(standard_pbr.id, "u_has_ao_map");
		standard_pbr.u_has_metallic_map_loc =
			glGetUniformLocation(standard_pbr.id, "u_has_metallic_map");
		standard_pbr.u_has_roughness_map_loc =
			glGetUniformLocation(standard_pbr.id, "u_has_roughness_map");

		standard_pbr.u_albedo_sampler_loc =
			glGetUniformLocation(standard_pbr.id, "u_albedo_sampler");
		standard_pbr.u_normal_sampler_loc =
			glGetUniformLocation(standard_pbr.id, "u_normal_sampler");
		standard_pbr.u_ao_sampler_loc =
			glGetUniformLocation(standard_pbr.id, "u_ao_sampler");
		standard_pbr.u_metallic_sampler_loc =
			glGetUniformLocation(standard_pbr.id, "u_metallic_sampler");
		standard_pbr.u_roughness_sampler_loc =
			glGetUniformLocation(standard_pbr.id, "u_roughness_sampler");

		standard_pbr.u_metallic_loc =
			glGetUniformLocation(standard_pbr.id, "u_metallic");
		standard_pbr.u_roughness_loc =
			glGetUniformLocation(standard_pbr.id, "u_roughness");

		standard_pbr.u_gamma_loc =
			glGetUniformLocation(standard_pbr.id, "u_gamma");
		standard_pbr.u_exposure_loc =
			glGetUniformLocation(standard_pbr.id, "u_exposure");

		assert(standard_pbr.u_model_matrix_loc != -1);
		assert(standard_pbr.u_pv_matrix_loc != -1);
		assert(standard_pbr.u_nor_transform_loc != -1);

		assert(standard_pbr.u_view_pos_loc != -1);

		assert(standard_pbr.u_irrandiance_sampler_loc != -1);
		assert(standard_pbr.u_specular_sampler_loc != -1);
		assert(standard_pbr.u_brdf_lut_sampler_loc != -1);

		assert(standard_pbr.u_has_normal_map_loc != -1);
		assert(standard_pbr.u_has_ao_map_loc != -1);
		assert(standard_pbr.u_has_metallic_map_loc != -1);
		assert(standard_pbr.u_has_roughness_map_loc != -1);

		assert(standard_pbr.u_albedo_sampler_loc != -1);
		assert(standard_pbr.u_normal_sampler_loc != -1);
		assert(standard_pbr.u_ao_sampler_loc != -1);
		assert(standard_pbr.u_metallic_sampler_loc != -1);
		assert(standard_pbr.u_roughness_sampler_loc != -1);

		assert(standard_pbr.u_metallic_loc != -1);
		assert(standard_pbr.u_roughness_loc != -1);

		assert(standard_pbr.u_gamma_loc != -1);
		assert(standard_pbr.u_exposure_loc != -1);

		std::cout << "SUCCESS\n";

		return true;
	}

	bool createIrradianceProgram()
	{
		std::vector<ShaderInfo> shaders;

		std::ifstream vs_file("shaders/irradiance/vs.glsl");
		std::ifstream fs_file("shaders/irradiance/fs.glsl");

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

		std::cout << "Creating irradiance program ... ";

		readShader(vs_file, fs_file, shaders);

		bool success;

		irradiance_program.id = gl.createProgram(shaders, success);

		if (!success)
		{
			return false;
		}

		irradiance_program.u_view_matrix_loc =
			glGetUniformLocation(irradiance_program.id, "u_view_matrix");
		irradiance_program.u_projection_matrix_loc =
			glGetUniformLocation(irradiance_program.id, "u_projection_matrix");

		irradiance_program.u_env_map_sampler_loc =
			glGetUniformLocation(irradiance_program.id, "u_env_map_sampler");

		assert(irradiance_program.u_view_matrix_loc != -1);
		assert(irradiance_program.u_projection_matrix_loc != -1);
		assert(irradiance_program.u_env_map_sampler_loc != -1);

		std::cout << "SUCCESS\n";

		return true;
	}

	bool createSpecularMapProgram()
	{
		std::vector<ShaderInfo> shaders;

		std::ifstream vs_file("shaders/specularMap/vs.glsl");
		std::ifstream fs_file("shaders/specularMap/fs.glsl");

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

		std::cout << "Creating specular map program ... ";

		readShader(vs_file, fs_file, shaders);

		bool success;

		specular_program.id = gl.createProgram(shaders, success);

		if (!success)
		{
			return false;
		}

		specular_program.u_view_matrix_loc =
			glGetUniformLocation(specular_program.id, "u_view_matrix");
		specular_program.u_projection_matrix_loc =
			glGetUniformLocation(specular_program.id, "u_projection_matrix");

		specular_program.u_env_map_sampler_loc =
			glGetUniformLocation(specular_program.id, "u_env_map_sampler");
		specular_program.u_roughness_loc =
			glGetUniformLocation(specular_program.id, "u_roughness");

		assert(specular_program.u_view_matrix_loc != -1);
		assert(specular_program.u_projection_matrix_loc != -1);
		assert(specular_program.u_env_map_sampler_loc != -1);
		assert(specular_program.u_roughness_loc != -1);

		std::cout << "SUCCESS\n";

		return true;
	}

	bool createBRDFConvolutionProgram()
	{
		std::vector<ShaderInfo> shaders;

		std::ifstream vs_file("shaders/brdfConvolution/vs.glsl");
		std::ifstream fs_file("shaders/brdfConvolution/fs.glsl");

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

		std::cout << "Creating brdf convolution program ... ";

		readShader(vs_file, fs_file, shaders);

		bool success;

		brdf_convolution_program.id = gl.createProgram(shaders, success);

		if (!success)
		{
			return false;
		}

		std::cout << "SUCCESS\n";

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

		std::cout << "Creating skybox program ... ";

		readShader(vs_file, fs_file, shaders);

		bool success;

		skybox_program.id = gl.createProgram(shaders, success);

		if (!success)
		{
			return false;
		}

		skybox_program.u_view_matrix_loc =
			glGetUniformLocation(skybox_program.id, "u_view_matrix");
		skybox_program.u_projection_matrix_loc =
			glGetUniformLocation(skybox_program.id, "u_projection_matrix");

		skybox_program.u_cube_sampler_loc =
			glGetUniformLocation(skybox_program.id, "u_cube_sampler");

		skybox_program.u_mipmap_level_loc =
			glGetUniformLocation(skybox_program.id, "u_mipmap_level");

		skybox_program.u_gamma_loc =
			glGetUniformLocation(skybox_program.id, "u_gamma");
		skybox_program.u_exposure_loc =
			glGetUniformLocation(skybox_program.id, "u_exposure");

		assert(skybox_program.u_view_matrix_loc != -1);
		assert(skybox_program.u_projection_matrix_loc != -1);
		assert(skybox_program.u_cube_sampler_loc != -1);
		assert(skybox_program.u_mipmap_level_loc != -1);
		assert(skybox_program.u_gamma_loc != -1);
		assert(skybox_program.u_exposure_loc != -1);

		std::cout << "SUCCESS\n";

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
		std::cout << "Loading material textures ... ";

		textures[0] = Texture2D("../res/materialBall/color.png", 3,
			GL_REPEAT, GL_REPEAT, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);

		textures[1] = Texture2D("../res/materialBall/normal.png", 3,
			GL_REPEAT, GL_REPEAT, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);

		textures[2] = Texture2D("../res/materialBall/ao.png", 1,
			GL_REPEAT, GL_REPEAT, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);

		textures[3] = Texture2D("../res/materialBall/metallic.png", 1,
			GL_REPEAT, GL_REPEAT, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);

		textures[4] = Texture2D("../res/materialBall/roughness.png", 1,
			GL_REPEAT, GL_REPEAT, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);

		for (int i = 0; i < N_MATERIAL_TEXTURES; ++i)
		{
			textures[i].bind(i + 4);
		}

		std::cout << "DONE\n";
	}

	bool createGeometry()
	{
		std::cout << "Creating Geometry ... ";

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
			irradiance_program.id, f_buffers, i_buffers, indices, success);

		if (!success)
		{
			return false;
		}

		return true;
	}

	bool createQuad()
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
			}
		};

		std::vector<BufferInfo<int>> i_buffers;

		std::vector<unsigned> indices
		{
			0, 1, 2, 0, 2, 3
		};

		bool success;

		quad = gl.createPackedStaticGeometry(
			brdf_convolution_program.id, f_buffers,
			i_buffers, indices, success);

		if (!success)
		{
			return false;
		}

		std::cout << "DONE\n";

		return true;
	}

	void createEnvironments()
	{
		std::cout << "\n";

		std::vector<std::pair<std::string, std::string>> files
		{
			{
				"../res/environmentMaps/gravelPlaza.hdr",
				"../res/environmentMaps/gravelPlazaIrradiance.hdr",
			},
			{
				"../res/environmentMaps/paperMill.hdr",
				"../res/environmentMaps/paperMillIrradiance.hdr",
			},
			{
				"../res/environmentMaps/winterForest.hdr",
				"../res/environmentMaps/winterForestIrradiance.hdr",
			}
		};

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

		for (size_t i = 0; i < files.size(); ++i)
		{
			createEnvironmentCubeMap(i, env_projection, env_views, files[i]);
		}

		env_cube_texture[current_environment]->bind(0);
		irr_cube_texture[current_environment]->bind(1);
		spec_cube_texture[current_environment]->bind(2);

		std::cout << "\n";
	}

	void createEnvironmentCubeMap(
		size_t index,
		glm::mat4 const& env_projection,
		glm::mat4 const* env_views,
		std::pair<std::string, std::string> const& files)
	{
		std::cout << "Creating environment cube map: "
			<< files.first << " ... ";

		env_cube_texture[index] = new Empty16FTextureCube(
			FBO_ENV_WIDTH,
			FBO_ENV_HEIGHT,
			3,
			GL_CLAMP_TO_EDGE,
			GL_CLAMP_TO_EDGE,
			GL_CLAMP_TO_EDGE,
			false);

		irr_cube_texture[index] = new Empty16FTextureCube(
			FBO_ENV_WIDTH,
			FBO_ENV_HEIGHT,
			3,
			GL_CLAMP_TO_EDGE,
			GL_CLAMP_TO_EDGE,
			GL_CLAMP_TO_EDGE,
			false);

		spec_cube_texture[index] = new Empty16FTextureCube(
			FBO_SPEC_WIDTH,
			FBO_SPEC_HEIGHT,
			3,
			GL_CLAMP_TO_EDGE,
			GL_CLAMP_TO_EDGE,
			GL_CLAMP_TO_EDGE,
			true);

		env_source_texture[index] = new TextureHDREnvironment(
			files.first,
			GL_CLAMP_TO_EDGE,
			GL_CLAMP_TO_EDGE,
			GL_LINEAR,
			GL_LINEAR,
			true);

		irr_source_texture[index] = new TextureHDREnvironment(
			files.second,
			GL_CLAMP_TO_EDGE,
			GL_CLAMP_TO_EDGE,
			GL_LINEAR,
			GL_LINEAR,
			true);

		Renderbuffer env_renderbuffer(GL_DEPTH_COMPONENT24, FBO_ENV_WIDTH, FBO_ENV_HEIGHT);
		Framebuffer env_framebuffer;

		env_framebuffer.attachRenderbuffer(GL_DEPTH_ATTACHMENT, env_renderbuffer);

		env_source_texture[index]->bind(0);
		irr_source_texture[index]->bind(1);

		glUseProgram(irradiance_program.id);

		glUniformMatrix4fv(irradiance_program.u_projection_matrix_loc,
			1, GL_FALSE, glm::value_ptr(env_projection));

		glViewport(0, 0, FBO_ENV_WIDTH, FBO_ENV_HEIGHT);

		env_framebuffer.bind();

		for (int i = 0; i < 6; ++i)
		{
			glUniformMatrix4fv(irradiance_program.u_view_matrix_loc,
				1, GL_FALSE, glm::value_ptr(env_views[i]));
			glUniform1i(irradiance_program.u_env_map_sampler_loc, 0);

			env_framebuffer.attachCubeMapTexture(
				GL_COLOR_ATTACHMENT0, *env_cube_texture[index], 0, i);
			env_framebuffer.checkStatus();

			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			glBindVertexArray(cube.vao_id);
			glDrawElements(GL_TRIANGLES, cube.n_indices, GL_UNSIGNED_INT, nullptr);

			glUniform1i(irradiance_program.u_env_map_sampler_loc, 1);

			env_framebuffer.attachCubeMapTexture(
				GL_COLOR_ATTACHMENT0, *irr_cube_texture[index], 0, i);
			env_framebuffer.checkStatus();

			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			glBindVertexArray(cube.vao_id);
			glDrawElements(GL_TRIANGLES, cube.n_indices, GL_UNSIGNED_INT, nullptr);
		}

		env_renderbuffer.destroy();

		std::cout << "specular map ... ";

		env_cube_texture[index]->bind(0);

		glUseProgram(specular_program.id);

		glUniformMatrix4fv(specular_program.u_projection_matrix_loc,
			1, GL_FALSE, glm::value_ptr(env_projection));

		glUniform1i(specular_program.u_env_map_sampler_loc, 0);

		int n_mipmap_levels = 1 +
			floor(std::log2(std::max(FBO_SPEC_WIDTH, FBO_SPEC_HEIGHT)));
		int mip_width = FBO_SPEC_WIDTH;
		int mip_height = FBO_SPEC_HEIGHT;

		for (int i = 0; i < n_mipmap_levels; ++i)
		{
			float r = (float)i / (float)(n_mipmap_levels - 1);
			glUniform1f(specular_program.u_roughness_loc, r);

			Renderbuffer rb(GL_DEPTH_COMPONENT24, mip_width, mip_height);

			env_framebuffer.detachCubeMapTexture(GL_COLOR_ATTACHMENT0);
			env_framebuffer.attachRenderbuffer(GL_DEPTH_ATTACHMENT, rb);

			glViewport(0, 0, mip_width, mip_height);

			for (int j = 0; j < 6; ++j)
			{
				glUniformMatrix4fv(specular_program.u_view_matrix_loc,
					1, GL_FALSE, glm::value_ptr(env_views[j]));

				env_framebuffer.attachCubeMapTexture(
					GL_COLOR_ATTACHMENT0, *spec_cube_texture[index], i, j);
				env_framebuffer.checkStatus();

				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

				glBindVertexArray(cube.vao_id);
				glDrawElements(GL_TRIANGLES, cube.n_indices, GL_UNSIGNED_INT, nullptr);
			}

			mip_width /= 2;
			mip_height /= 2;

			rb.destroy();
		}

		env_source_texture[index]->destroy();
		delete env_source_texture[index];

		irr_source_texture[index]->destroy();
		delete irr_source_texture[index];

		Framebuffer::bindDefault();

		std::cout << "DONE\n";
	}

	void createBRDFLUT()
	{
		std::cout << "Creating BRDF LUT ... ";

		brdf_lut = Texture2D(
			BRDF_LUT_WIDTH,
			BRDF_LUT_HEIGHT,
			GL_RG16F,
			GL_CLAMP_TO_EDGE,
			GL_CLAMP_TO_EDGE,
			GL_LINEAR,
			GL_LINEAR);

		Renderbuffer lut_renderbuffer(GL_DEPTH_COMPONENT24, BRDF_LUT_WIDTH, BRDF_LUT_HEIGHT);

		Framebuffer lut_framebuffer;
		lut_framebuffer.attachTexture(GL_COLOR_ATTACHMENT0, brdf_lut, 0);
		lut_framebuffer.attachRenderbuffer(GL_DEPTH_ATTACHMENT, lut_renderbuffer);
		lut_framebuffer.bind();

		glViewport(0, 0, BRDF_LUT_WIDTH, BRDF_LUT_HEIGHT);

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glUseProgram(brdf_convolution_program.id);

		glBindVertexArray(quad.vao_id);
		glDrawElements(GL_TRIANGLES, quad.n_indices, GL_UNSIGNED_INT, nullptr);

		Framebuffer::bindDefault();

		brdf_lut.bind(3);

		std::cout << "DONE\n";
	}

	/// Environment
	int current_environment = 0;

	Empty16FTextureCube* env_cube_texture[N_ENVIRONMENTS];
	Empty16FTextureCube* irr_cube_texture[N_ENVIRONMENTS];
	Empty16FTextureCube* spec_cube_texture[N_ENVIRONMENTS];

	TextureHDREnvironment* env_source_texture[N_ENVIRONMENTS];
	TextureHDREnvironment* irr_source_texture[N_ENVIRONMENTS];

	Texture brdf_lut;

	/// Programs
	StandardPBRProgram standard_pbr;
	IrradianceProgram irradiance_program;
	SpecularMapProgram specular_program;
	BRDFConvolutionProgram brdf_convolution_program;
	SkyboxProgram skybox_program;

	/// Material
	Texture textures[N_MATERIAL_TEXTURES];

	bool has_normal_map = true;
	bool has_ao_map = true;
	bool has_metallic_map = true;
	bool has_roughness_map = true;

	float metallic = 0.0f;
	float roughness = 0.05f;

	/// Skybox
	int skybox_sampler_unit = 0;
	float skybox_mipmap_level = 0;

	/// Object properties
	DeviceMesh geometry;
	DeviceMesh cube;
	DeviceMesh quad;
	glm::mat4 model_matrix;
	glm::vec3 rotation{ 0.0f, 0.0f, 0.0f };

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

		app->setWindowSize(width, height);
	}
}

int main()
{
	Application app("Image Based Lighting", WINDOW_WIDTH, WINDOW_HEIGHT);

	if (app.init())
	{
		app.run();
		app.destroy();
	}
}

