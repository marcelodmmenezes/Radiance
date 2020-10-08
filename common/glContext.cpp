#include "glContext.hpp"

#include <iostream>

void OpenGLContext::setWireframe(bool state) {
	glPolygonMode(GL_FRONT_AND_BACK, state ? GL_LINE : GL_FILL);
}

void OpenGLContext::setLineWidth(float width) {
	glLineWidth(width);
}

bool OpenGLContext::load(GLADloadproc loader) {
	if (!gladLoadGLLoader(loader))
		return false;

	std::cout << "OpenGL Info:\n"
		<< "\nGraphics Card Vendor: " << glGetString(GL_VENDOR)
		<< "\nRenderer:             " << glGetString(GL_RENDERER)
		<< "\nOpenGL version:       " << glGetString(GL_VERSION)
		<< "\nGLSL version:         " << glGetString(GL_SHADING_LANGUAGE_VERSION)
		<< "\n\n";

	return true;
}

bool OpenGLContext::checkErrors() {
	GLenum error;
	bool has_error = false;

	while ((error = glGetError()) != GL_NO_ERROR) {
		switch (error) {

#define CASE(error) \
	case (error): \
		std::cerr << #error "\n"; \
		break

			CASE(GL_INVALID_ENUM);
			CASE(GL_INVALID_VALUE);
			CASE(GL_INVALID_OPERATION);
			CASE(GL_STACK_OVERFLOW);
			CASE(GL_STACK_UNDERFLOW);
			CASE(GL_OUT_OF_MEMORY);
			CASE(GL_INVALID_FRAMEBUFFER_OPERATION);
			CASE(GL_CONTEXT_LOST);

#undef CASE

		}

		has_error = true;
	}

	return has_error;
}

GLuint OpenGLContext::createProgram(
	std::vector<ShaderInfo>& shader_infos,
	bool& success) {

	success = false;
	GLuint program_id;
	std::vector<GLuint> shader_ids;
	bool compiled;

	for (size_t i = 0u; i < shader_infos.size(); ++i) {
		GLuint shader_id = compileShader(shader_infos[i], compiled);

		if (compiled)
			shader_ids.emplace_back(shader_id);
		else
			break;
	}

	if (compiled) {
		program_id = glCreateProgram();

		for (size_t i = 0u; i < shader_ids.size(); ++i)
			glAttachShader(program_id, shader_ids[i]);

		glLinkProgram(program_id);

		GLint linked;
		glGetProgramiv(program_id, GL_LINK_STATUS, &linked);

		if (linked == GL_FALSE) {
			GLsizei log_length;
			glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &log_length);

			std::vector<char> log(log_length);
			glGetProgramInfoLog(program_id, log_length, nullptr, log.data());

			std::string log_str(log.begin(), log.end());
			std::cerr << "Program linkage failed:\n" << log_str << "\n\n";

			return 0u;
		}
		else
			success = true;
	}

	for (size_t i = 0u; i < shader_ids.size(); ++i)
		glDeleteShader(shader_ids[i]);

	return program_id;
}

GLuint OpenGLContext::compileShader(ShaderInfo& shader_info, bool& success) {
	GLuint shader_id = glCreateShader(shader_info.type);
	GLchar* source_c_str = (GLchar*)shader_info.source.c_str();
	glShaderSource(shader_id, 1, &source_c_str, nullptr);
	glCompileShader(shader_id);

	GLint compiled;
	glGetShaderiv(shader_id, GL_COMPILE_STATUS, &compiled);

	if (compiled == GL_FALSE) {
		GLsizei log_length;
		glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &log_length);

		std::vector<char> log(log_length);
		glGetShaderInfoLog(shader_id, log_length, nullptr, log.data());

		std::string log_str(log.begin(), log.end());

		switch (shader_info.type) {
			case GL_VERTEX_SHADER:
				std::cerr << "Vertex ";
				break;
			case GL_TESS_CONTROL_SHADER:
				std::cerr << "Tessellation control ";
				break;
			case GL_TESS_EVALUATION_SHADER:
				std::cerr << "Tessellation evaluation ";
				break;
			case GL_GEOMETRY_SHADER:
				std::cerr << "Geometry ";
				break;
			case GL_FRAGMENT_SHADER:
				std::cerr << "Fragment ";
				break;
			case GL_COMPUTE_SHADER:
				std::cerr << "Compute ";
				break;
		}

		std::cerr << "shader compilation failed:\n" << log_str << "\n\n";

		success = false;

		return 0u;
	}

	success = true;

	return shader_id;
}

