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

bool OpenGLContext::checkErrors(std::string const& file, int line) {
	GLenum error;
	bool has_error = false;

	while ((error = glGetError()) != GL_NO_ERROR) {
		switch (error) {

#define CASE(error) \
	case (error): \
		std::cerr << #error "\nFile: " << file << "\nLine: " << line << '\n'; \
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

template <typename T>
void populateBufferChunk(
	GLuint buffer_id,
	size_t vertex_id,
	size_t byte_offset,
	size_t vertex_size_in_bytes,
	std::vector<BufferInfo<T>> const& buffer) {

	for (size_t i = 0u; i < buffer.size(); ++i) {
		glNamedBufferSubData(buffer_id,
			vertex_id * vertex_size_in_bytes + byte_offset,
			buffer[i].n_components * sizeof(T),
			buffer[i].values.data() + vertex_id * buffer[i].n_components);

		byte_offset += buffer[i].n_components * sizeof(T);
	}
}

DeviceMesh OpenGLContext::createPackedStaticGeometry(
	GLuint program_id,
	std::vector<BufferInfo<float>> const& f_buffers,
	std::vector<BufferInfo<int>> const& i_buffers,
	std::vector<unsigned> const& indices) {

	assert(glIsProgram(program_id) == GL_TRUE && "Program is not valid");

	size_t n_buffers = f_buffers.size() + i_buffers.size();

	assert(n_buffers >= 1u && "At least one buffer must be passed");

	size_t n_vertices;

	if (f_buffers.size() > 0u)
		n_vertices = f_buffers[0].values.size() / f_buffers[0].n_components;
	else
		n_vertices = i_buffers[0].values.size() / i_buffers[0].n_components;

	size_t vertex_size_in_bytes = 0u;

	for (auto& it : f_buffers)
		vertex_size_in_bytes += it.n_components * sizeof(float);

	for (auto& it : i_buffers)
		vertex_size_in_bytes += it.n_components * sizeof(int);

	DeviceMesh mesh;
	mesh.n_indices = indices.size();

	glCreateVertexArrays(1, &mesh.vao_id);
	glCreateBuffers(1, &mesh.vbo_id);
	glCreateBuffers(1, &mesh.ebo_id);

	glBindVertexArray(mesh.vao_id);

	glNamedBufferData(mesh.vbo_id,
		n_vertices * vertex_size_in_bytes,
		nullptr, GL_STATIC_DRAW);

	for (size_t i = 0u; i < n_vertices; ++i) {
		size_t byte_offset = 0u;

		populateBufferChunk(mesh.vbo_id, i, byte_offset,
			vertex_size_in_bytes, f_buffers);
		populateBufferChunk(mesh.vbo_id, i, byte_offset,
			vertex_size_in_bytes, i_buffers);
	}

	glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo_id);

	size_t byte_offset = 0u;

	for (size_t i = 0u; i < f_buffers.size(); ++i) {
		GLint location = glGetAttribLocation(
			program_id, f_buffers[i].attribute_name.c_str());

		glVertexAttribPointer(location, f_buffers[i].n_components,
			GL_FLOAT, GL_FALSE, vertex_size_in_bytes, (GLvoid*)&byte_offset);
		glEnableVertexAttribArray(location);

		byte_offset += f_buffers[i].n_components * sizeof(float);
	}

	for (size_t i = 0u; i < i_buffers.size(); ++i) {
		GLint location = glGetAttribLocation(
			program_id, i_buffers[i].attribute_name.c_str());

		glVertexAttribIPointer(location, i_buffers[i].n_components,
			GL_INT, vertex_size_in_bytes, (GLvoid*)&byte_offset);
		glEnableVertexAttribArray(location);

		byte_offset += i_buffers[i].n_components * sizeof(int);
	}

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo_id);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER,
		indices.size() * sizeof(unsigned),
		indices.data(), GL_STATIC_DRAW);

	glBindVertexArray(0u);
	glBindBuffer(GL_ARRAY_BUFFER, 0u);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0u);

	return mesh;
}

void OpenGLContext::destroyGeometry(DeviceMesh& mesh) {
	if (glIsBuffer(mesh.ebo_id))
		glDeleteBuffers(1, &mesh.ebo_id);

	if (glIsBuffer(mesh.vbo_id))
		glDeleteBuffers(1, &mesh.vbo_id);

	if (glIsVertexArray(mesh.vao_id))
		glDeleteVertexArrays(1, &mesh.vao_id);
}

