#include "texture.hpp"
#include "glContext.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include <cassert>
#include <cmath>

Texture2D::Texture2D(
	std::string const& file_path,
	int n_desired_channels,
	GLenum wrap_s,
	GLenum wrap_t,
	GLenum min_filter,
	GLenum mag_filter)
	:
	path(file_path)
{
	int width, height, channels;

	stbi_set_flip_vertically_on_load(true);

	unsigned char* image = stbi_load(file_path.c_str(),
		&width, &height, &channels, n_desired_channels);

	assert(image && ("ERROR: Could not load texture " +
		file_path + ": " + stbi_failure_reason() + '\n').c_str());

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

	default:
		internal_format = GL_RGBA8;
		data_format = GL_RGBA;
	}

	glCreateTextures(GL_TEXTURE_2D, 1, &id);

	assert(glIsTexture(id) && ("ERROR: Could not create texture from " +
		file_path + '\n').c_str());

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

void Texture2D::bind(GLuint unit)
{
	assert(glIsTexture(id) && ("ERROR: Texture " +
		path + " was not created or was deleted\n").c_str());

	glBindTextureUnit(unit, id);
}

void Texture2D::destroy()
{
	if (glIsTexture(id))
	{
		glDeleteTextures(1, &id);
	}
}

