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

	GLuint getId() const;

	void attachTexture(
		GLenum attachment,
		Texture const& texture,
		GLint mipmap_level);

	void detachTexture(GLenum attachment);

	void attachCubeMapTexture(
		GLenum attachment,
		Texture const& texture,
		GLint mipmap_level,
		GLint layer);

	void detachCubeMapTexture(GLenum attachment);

	void attachRenderbuffer(
		GLenum attachment,
		Renderbuffer const& renderbuffer);

	void detachRenderbuffer(GLenum attachment);

	void destroy();

	void bind(); 

	void checkStatus();

private:
	GLuint id;
};

#endif // FRAMEBUFFER_HPP

