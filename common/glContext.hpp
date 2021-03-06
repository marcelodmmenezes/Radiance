#ifndef GL_CONTEXT_HPP
#define GL_CONTEXT_HPP

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <glad/glad.h>

#include <cassert>
#include <string>
#include <vector>

struct ShaderInfo
{
	GLenum type;
	std::string source;
};

template <typename T>
struct BufferInfo
{
	std::string attribute_name;
	size_t n_components;
	std::vector<T> values;
};

struct DeviceMesh
{
	/// Handles
	GLuint vao_id;
	GLuint vbo_id;
	GLuint ebo_id;

	/// Properties
	size_t n_indices;
};

class OpenGLContext
{
public:
	static bool checkErrors(std::string const& file, int line);

	OpenGLContext()
	{}

	~OpenGLContext()
	{}

	void setWireframe(bool state);
	void setLineWidth(float width);

	bool load(GLADloadproc loader);

	void enable(GLenum capability);
	void disable(GLenum capability);

	GLuint createProgram(
		std::vector<ShaderInfo>& shader_infos,
		bool& success) const;

	// - Each buffer should have the same number of
	// vertices (values.size() / n_components)
	// - Accepting only float and int attributes for now
	DeviceMesh createPackedStaticGeometry(
		GLuint program_id,
		std::vector<BufferInfo<float>> const& f_buffers,
		std::vector<BufferInfo<int>> const& i_buffers,
		std::vector<unsigned> const& indices,
		bool& success) const;

	void destroyGeometry(DeviceMesh& mesh) const;

private:
	GLuint compileShader(ShaderInfo& shader_info, bool& success) const;
};

#endif // GL_CONTEXT_HPP

