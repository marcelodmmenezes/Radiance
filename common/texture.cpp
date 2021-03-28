#include "texture.hpp"
#include "glContext.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include <cassert>
#include <cmath>
#include <iostream>

Texture::Texture(std::string const& file_path)
	:
	path{ file_path }
{}

Texture::Texture(
	std::string const& file_path,
	int width,
	int height)
	:
	path{ file_path },
	width{ width },
	height{ height }
{}

void Texture::setParameteri(GLenum parameter, GLint value)
{
	glTextureParameteri(id, parameter, value);
}

void Texture::bind(GLuint unit)
{
	if (!glIsTexture(id))
	{
		std::cerr << "ERROR: Texture " + path +
			" was not created or was deleted\n";

		abort();
	}

	glBindTextureUnit(unit, id);
}

GLuint Texture::getId() const
{
	return id;
}

int Texture::getWidth() const
{
	return width;
}

int Texture::getHeight() const
{
	return height;
}

int Texture::getChannels() const
{
	return channels;
}

void Texture::destroy()
{
	if (glIsTexture(id))
	{
		glDeleteTextures(1, &id);
	}
}

Texture2D::Texture2D(
	std::string const& file_path,
	int n_desired_channels,
	GLint wrap_s,
	GLint wrap_t,
	GLint min_filter,
	GLint mag_filter)
	:
	Texture(file_path)
{
	stbi_set_flip_vertically_on_load(true);

	unsigned char* image = stbi_load(path.c_str(),
		&width, &height, &channels, n_desired_channels);

	if (!image)
	{
		std::cerr << "ERROR: Could not load texture " +
			path + ": " + stbi_failure_reason() + '\n';

		abort();
	}

	if (n_desired_channels < channels)
	{
		channels = n_desired_channels;
	}

	GLenum internal_format, data_format;

	switch (channels)
	{
	case 1:
		internal_format = GL_R8;
		data_format = GL_RED;
		break;

	case 2:
		internal_format = GL_RG8;
		data_format = GL_RG;
		break;

	case 3:
		internal_format = GL_RGB8;
		data_format = GL_RGB;
		break;

	case 4:
		internal_format = GL_RGBA8;
		data_format = GL_RGBA;
		break;

	default:
		assert(false);
	}

	glCreateTextures(GL_TEXTURE_2D, 1, &id);

	if (!glIsTexture(id))
	{
		std::cerr << "ERROR: Could not create texture from " + path + '\n';
		abort();
	}

	GLsizei n_mipmap_levels = 1;

	if (min_filter == GL_NEAREST_MIPMAP_NEAREST ||
		min_filter == GL_NEAREST_MIPMAP_LINEAR ||
		min_filter == GL_LINEAR_MIPMAP_NEAREST ||
		min_filter == GL_LINEAR_MIPMAP_LINEAR)
	{
		n_mipmap_levels = 1 + floor(std::log2(std::max(width, height)));
	}

	glTextureStorage2D(id, n_mipmap_levels, internal_format, width, height);

	glTextureParameteri(id, GL_TEXTURE_WRAP_S, wrap_s);
	glTextureParameteri(id, GL_TEXTURE_WRAP_T, wrap_t);
	glTextureParameteri(id, GL_TEXTURE_MIN_FILTER, min_filter);
	glTextureParameteri(id, GL_TEXTURE_MAG_FILTER, mag_filter);

	glTextureSubImage2D(id, 0, 0, 0, width, height,
		data_format, GL_UNSIGNED_BYTE, image);

	if (min_filter == GL_NEAREST_MIPMAP_NEAREST ||
		min_filter == GL_NEAREST_MIPMAP_LINEAR ||
		min_filter == GL_LINEAR_MIPMAP_NEAREST ||
		min_filter == GL_LINEAR_MIPMAP_LINEAR)
	{
		glGenerateTextureMipmap(id);
	}

	stbi_image_free(image);
}

Texture2D::Texture2D(
	std::vector<unsigned char*> data,
	int width,
	int height,
	GLenum internal_format,
	GLenum data_format,
	GLint wrap_s,
	GLint wrap_t,
	GLint min_filter,
	GLint mag_filter)
	:
	Texture("Built from memory", width, height)
{
	switch (data_format)
	{
	case GL_RED:
		channels = 1;
		break;

	case GL_RG:
		channels = 2;
		break;

	case GL_RGB:
		channels = 3;
		break;

	case GL_RGBA:
		channels = 4;
		break;
	}

	glCreateTextures(GL_TEXTURE_2D, 1, &id);

	if (!glIsTexture(id))
	{
		std::cerr << "ERROR: Could not create texture from " + path + '\n';
		abort();
	}

	GLsizei n_mipmap_levels = 1;

	if (min_filter == GL_NEAREST_MIPMAP_NEAREST ||
		min_filter == GL_NEAREST_MIPMAP_LINEAR ||
		min_filter == GL_LINEAR_MIPMAP_NEAREST ||
		min_filter == GL_LINEAR_MIPMAP_LINEAR)
	{
		n_mipmap_levels = 1 + floor(std::log2(std::max(width, height)));
	}

	assert((unsigned)n_mipmap_levels == data.size() &&
		"ERROR: Number of provided mipmaps doesnt match");

	glTextureStorage2D(id, n_mipmap_levels, internal_format, width, height);

	glTextureParameteri(id, GL_TEXTURE_WRAP_S, wrap_s);
	glTextureParameteri(id, GL_TEXTURE_WRAP_T, wrap_t);
	glTextureParameteri(id, GL_TEXTURE_MIN_FILTER, min_filter);
	glTextureParameteri(id, GL_TEXTURE_MAG_FILTER, mag_filter);

	if (min_filter == GL_NEAREST_MIPMAP_NEAREST ||
		min_filter == GL_NEAREST_MIPMAP_LINEAR ||
		min_filter == GL_LINEAR_MIPMAP_NEAREST ||
		min_filter == GL_LINEAR_MIPMAP_LINEAR)
	{
		int current_width = width, current_height = height;

		for (int i = 0; i < n_mipmap_levels; ++i)
		{
			glTextureSubImage2D(id, i, 0, 0, current_width, current_height,
				data_format, GL_UNSIGNED_BYTE, data[i]);

			current_width /= 2;
			current_height /= 2;
		}
	}
}

TextureCube::TextureCube(
	std::string const& folder,
	std::string const& extension,
	int n_desired_channels,
	GLint wrap_s,
	GLint wrap_t,
	GLint wrap_r,
	GLint min_filter,
	GLint mag_filter,
	bool flip_on_load)
	:
	Texture(folder)
{
	stbi_set_flip_vertically_on_load(flip_on_load);

	std::string faces[]
	{
		"right.",
		"left.",
		"top.",
		"bottom.",
		"back.",
		"front."
	};

	glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &id);

	if (!glIsTexture(id))
	{
		std::cerr << "ERROR: Could not create texture from " + path + '\n';
		abort();
	}

	unsigned char* images[6];

	images[0] = stbi_load(
		(folder + faces[0] + extension).c_str(),
		&width, &height, &channels, n_desired_channels);

	if (!images[0])
	{
		std::cerr << "ERROR: Could not load texture " +
			path + faces[0] + extension + ": " +
			stbi_failure_reason() + '\n';

		abort();
	}

	if (n_desired_channels < channels)
	{
		channels = n_desired_channels;
	}

	GLenum internal_format, data_format;

	switch (channels)
	{
		case 1:
			internal_format = GL_R8;
			data_format = GL_RED;
			break;

		case 2:
			internal_format = GL_RG8;
			data_format = GL_RG;
			break;

		case 3:
			internal_format = GL_RGB8;
			data_format = GL_RGB;
			break;

		case 4:
			internal_format = GL_RGBA8;
			data_format = GL_RGBA;
			break;

		default:
			assert(false);
	}

	for (int i = 0; i < 6; ++i)
	{
		int w, h, c;

		images[i] = stbi_load(
			(folder + faces[i] + extension).c_str(),
			&w, &h, &c, n_desired_channels);

		if (!images[i])
		{
			std::cerr << "ERROR: Could not load texture " +
				path + faces[0] + extension + ": " +
				stbi_failure_reason() + '\n';

			abort();
		}

		if (n_desired_channels < c)
		{
			c = n_desired_channels;
		}

		assert(w == width && h == height && c == channels &&
			"Cube map faces must have equal properties");
	}

	GLsizei n_mipmap_levels = 1;

	if (min_filter == GL_NEAREST_MIPMAP_NEAREST ||
		min_filter == GL_NEAREST_MIPMAP_LINEAR ||
		min_filter == GL_LINEAR_MIPMAP_NEAREST ||
		min_filter == GL_LINEAR_MIPMAP_LINEAR)
	{
		n_mipmap_levels = 1 + floor(std::log2(std::max(width, height)));
	}

	glTextureStorage2D(id, n_mipmap_levels, internal_format, width, height);

	glTextureParameteri(id, GL_TEXTURE_WRAP_S, wrap_s);
	glTextureParameteri(id, GL_TEXTURE_WRAP_T, wrap_t);
	glTextureParameteri(id, GL_TEXTURE_WRAP_R, wrap_r);
	glTextureParameteri(id, GL_TEXTURE_MIN_FILTER, min_filter);
	glTextureParameteri(id, GL_TEXTURE_MAG_FILTER, mag_filter);

	for (int i = 0; i < 6; ++i)
	{
		glTextureSubImage3D(id, 0, 0, 0, i, width, height,
				1, data_format, GL_UNSIGNED_BYTE, images[i]);

		stbi_image_free(images[i]);
	}

	if (min_filter == GL_NEAREST_MIPMAP_NEAREST ||
		min_filter == GL_NEAREST_MIPMAP_LINEAR ||
		min_filter == GL_LINEAR_MIPMAP_NEAREST ||
		min_filter == GL_LINEAR_MIPMAP_LINEAR)
	{
		glGenerateTextureMipmap(id);
	}
}

Empty16FTextureCube::Empty16FTextureCube(
	int width,
	int height,
	int channels,
	GLint wrap_s,
	GLint wrap_t,
	GLint wrap_r)
	:
	Texture("Empty 16F Texture Cube", width, height)
{
	this->channels = channels;

	GLenum internal_format, data_format;

	glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &id);

	if (!glIsTexture(id))
	{
		std::cout << "ERROR: Could not create Empty16FTextureCube!\n";
		abort();
	}

	switch (channels)
	{
		case 1:
			internal_format = GL_R16F;
			data_format = GL_RED;
			break;

		case 2:
			internal_format = GL_RG16F;
			data_format = GL_RG;
			break;

		case 3:
			internal_format = GL_RGB16F;
			data_format = GL_RGB;
			break;

		case 4:
			internal_format = GL_RGBA16F;
			data_format = GL_RGBA;
			break;

		default:
			assert(false);
	}

	glTextureStorage2D(id, 1, internal_format, width, height);

	glTextureParameteri(id, GL_TEXTURE_WRAP_S, wrap_s);
	glTextureParameteri(id, GL_TEXTURE_WRAP_T, wrap_t);
	glTextureParameteri(id, GL_TEXTURE_WRAP_R, wrap_r);
	glTextureParameteri(id, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTextureParameteri(id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

TextureHDREnvironment::TextureHDREnvironment(
	std::string const& file_path,
	GLint wrap_s,
	GLint wrap_t,
	GLint min_filter,
	GLint mag_filter,
	bool flip_on_load)
	:
	Texture(file_path)
{
	stbi_set_flip_vertically_on_load(flip_on_load);

	float* image = stbi_loadf(file_path.c_str(), &width, &height, &channels, 0);

	if (!image)
	{
		std::cerr << "ERROR: Could not load texture " +
			path + ": " + stbi_failure_reason() + '\n';

		abort();
	}

	GLenum internal_format, data_format;

	glCreateTextures(GL_TEXTURE_2D, 1, &id);

	if (!glIsTexture(id))
	{
		std::cerr << "ERROR: Could not create texture from " + path + '\n';
		abort();
	}

	GLsizei n_mipmap_levels = 1;

	if (min_filter == GL_NEAREST_MIPMAP_NEAREST ||
		min_filter == GL_NEAREST_MIPMAP_LINEAR ||
		min_filter == GL_LINEAR_MIPMAP_NEAREST ||
		min_filter == GL_LINEAR_MIPMAP_LINEAR)
	{
		n_mipmap_levels = 1 + floor(std::log2(std::max(width, height)));
	}

	glTextureStorage2D(id, n_mipmap_levels, GL_RGB16F, width, height);

	glTextureParameteri(id, GL_TEXTURE_WRAP_S, wrap_s);
	glTextureParameteri(id, GL_TEXTURE_WRAP_T, wrap_t);
	glTextureParameteri(id, GL_TEXTURE_MIN_FILTER, min_filter);
	glTextureParameteri(id, GL_TEXTURE_MAG_FILTER, mag_filter);

	glTextureSubImage2D(id, 0, 0, 0, width, height, GL_RGB, GL_FLOAT, image);

	if (min_filter == GL_NEAREST_MIPMAP_NEAREST ||
		min_filter == GL_NEAREST_MIPMAP_LINEAR ||
		min_filter == GL_LINEAR_MIPMAP_NEAREST ||
		min_filter == GL_LINEAR_MIPMAP_LINEAR)
	{
		glGenerateTextureMipmap(id);
	}

	stbi_image_free(image);
}

