#ifndef GL_HELPERS_HPP
#define GL_HELPERS_HPP

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <glad/glad.h>

#include <string>
#include <vector>

struct ShaderInfo {
	GLenum type;
	std::string source;
};

class OpenGLContext {
public:
	OpenGLContext() {}
	~OpenGLContext() {}

	bool load(GLADloadproc loader);
	bool checkErrors();
	GLuint createProgram(std::vector<ShaderInfo>& shader_infos, bool& success);

private:
	GLuint compileShader(ShaderInfo& shader_info, bool& success);
};

#endif // GL_HELPERS_HPP

