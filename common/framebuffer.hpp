#ifndef FRAMEBUFFER_HPP
#define FRAMEBUFFER_HPP

#include "texture.hpp"
#include "renderbuffer.hpp"

#include <glad/glad.h>

class Framebuffer
{
public:
	static void bindDefault();

	Framebuffer();

	virtual ~Framebuffer()
	{}

	void attachTexture(
		GLenum attachment,
		Texture const& texture,
		GLint mipmap_level);

	void attachCubeMapTexture(
		GLenum attachment,
		Texture const& texture,
		GLint mipmap_level,
		GLint layer);

	void attachRenderbuffer(
		GLenum attachment,
		Renderbuffer const& renderbuffer);

	void destroy();

	void bind(); 

private:
	void checkStatus();

	GLuint id;
};

#endif // FRAMEBUFFER_HPP

