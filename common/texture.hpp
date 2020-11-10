#ifndef TEXTURE_HPP
#define TEXTURE_HPP

#include <glad/glad.h>

#include <string>

// TODO: support depth stencil texture
class Texture2D {
public:
	Texture2D(
		std::string const& file_path,
		int n_desired_channels,
		GLenum wrap_s,
		GLenum wrap_t,
		GLenum min_filter,
		GLenum mag_filter);

	Texture2D() {}
	~Texture2D() {}

	// Manually destroying to avoid deleting the
	// texture in the case of a vector resize
	void destroy();

	void bind(GLuint unit);

private:
	std::string path;

	GLuint id;
};

#endif // TEXTURE_HPP

