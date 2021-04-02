#include "framebuffer.hpp"

#include <iostream>

void Framebuffer::bindDefault()
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

Framebuffer::Framebuffer()
{
	glCreateFramebuffers(1, &id);

	if (!glIsFramebuffer(id))
	{
		std::cout << "ERROR: Could not create framebuffer\n";
		abort();
	}
}

void Framebuffer::attachTexture(
	GLenum attachment,
	Texture const& texture,
	GLint mipmap_level)
{
	glNamedFramebufferTexture(id, attachment,
		texture.getId(), mipmap_level);
}

void Framebuffer::detachTexture(GLenum attachment)
{
	glNamedFramebufferTexture(id, attachment, 0, 0);
}

void Framebuffer::attachCubeMapTexture(
	GLenum attachment,
	Texture const& texture,
	GLint mipmap_level,
	GLint layer)
{
	glNamedFramebufferTextureLayer(id, attachment,
		texture.getId(), mipmap_level, layer);
}

void Framebuffer::detachCubeMapTexture(GLenum attachment)
{
	glNamedFramebufferTextureLayer(id, attachment, 0, 0, 0);
}

void Framebuffer::attachRenderbuffer(
	GLenum attachment,
	Renderbuffer const& renderbuffer)
{
	glNamedFramebufferRenderbuffer(id, attachment,
		GL_RENDERBUFFER, renderbuffer.getId());
}

void Framebuffer::detachRenderbuffer(GLenum attachment)
{
	glNamedFramebufferRenderbuffer(id, attachment,
		GL_RENDERBUFFER, 0);
}

void Framebuffer::destroy()
{
	if (glIsFramebuffer(id))
	{
		glDeleteFramebuffers(1, &id);
	}
}

void Framebuffer::bind()
{
	glBindFramebuffer(GL_FRAMEBUFFER, id);
}

void Framebuffer::checkStatus()
{
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) !=
		GL_FRAMEBUFFER_COMPLETE)
	{
		std::cout << "ERROR: Incomplete framebuffer!\n";
		abort();
	}
}

