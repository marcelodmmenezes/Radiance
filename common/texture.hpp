#ifndef TEXTURE_HPP
#define TEXTURE_HPP

#include <glad/glad.h>

#include <string>
#include <vector>

class Texture
{
public:
	Texture(std::string const& file_path);
	Texture(std::string const& file_path, int width, int height);

	Texture()
	{}

	virtual ~Texture()
	{}

	void setParameteri(GLenum parameter, GLint value);

	// Manually destroying to avoid deleting the
	// texture in the case of a vector resize
	void destroy();

	void bind(GLuint unit);

	GLuint getId() const;
	int getWidth() const;
	int getHeight() const;
	int getChannels() const;

protected:
	std::string path;

	GLuint id;

	int width;
	int height;
	int channels;
};

// TODO: support depth stencil texture
class Texture2D : public Texture
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
};

class TextureCube : public Texture
{
public:
	TextureCube(
		std::string const& folder,
		std::string const& extension,
		int n_desired_channels,
		GLint wrap_s,
		GLint wrap_t,
		GLint wrap_r,
		GLint min_filter,
		GLint mag_filter,
		bool flip_on_load);
};

class Empty16FTextureCube : public Texture
{
public:
	Empty16FTextureCube(
		int width,
		int height,
		int channels,
		GLint wrap_s,
		GLint wrap_t,
		GLint wrap_r,
		bool allocate_mipmap_space);
};

class TextureHDREnvironment : public Texture
{
public:
	TextureHDREnvironment(
		std::string const& file_path,
		GLint wrap_s,
		GLint wrap_t,
		GLint min_filter,
		GLint mag_filter,
		bool flip_on_load);
};

#endif // TEXTURE_HPP

