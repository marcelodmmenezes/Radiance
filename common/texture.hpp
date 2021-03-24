#ifndef TEXTURE_HPP
#define TEXTURE_HPP

#include <glad/glad.h>

#include <string>
#include <vector>

// TODO: support depth stencil texture
class Texture2D
{
public:
	// Creates texture from image from disk
	// and generates mipmaps
	Texture2D(
		std::string const& file_path,
		int n_desired_channels,
		GLint wrap_s,
		GLint wrap_t,
		GLint min_filter,
		GLint mag_filter);

	// Creates texture from @data
	// @data should contain all the mipmap levels
	// @width and @height are respective to the highest resolution level
	Texture2D(
		std::vector<unsigned char*> data,
		int width,
		int height,
		GLenum internal_format,
		GLenum data_format,
		GLint wrap_s,
		GLint wrap_t,
		GLint min_filter,
		GLint mag_filter);

	Texture2D()
	{}

	~Texture2D()
	{}

	void setParameteri(GLenum parameter, GLint value);

	// Manually destroying to avoid deleting the
	// texture in the case of a vector resize
	void destroy();

	void bind(GLuint unit);

private:
	std::string path;

	GLuint id;
};

#endif // TEXTURE_HPP

