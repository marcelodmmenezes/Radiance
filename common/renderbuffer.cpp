#include "renderbuffer.hpp"

#include <cstdlib>
#include <iostream>

Renderbuffer::Renderbuffer(
	GLenum internal_format,
	int width,
	int height)
	:
	internal_format{ internal_format },
	width{ width },
	height{ height }
{
	glCreateRenderbuffers(1, &id);

	if (!glIsRenderbuffer(id))
	{
		std::cout << "ERROR: Could not create renderbuffer!\n";
		abort();
	}

	glNamedRenderbufferStorage(id, internal_format, width, height);
}

void Renderbuffer::destroy()
{
	if (glIsRenderbuffer(id))
	{
		glDeleteRenderbuffers(1, &id);
	}
}

GLuint Renderbuffer::getId() const
{
	return id;
}

GLenum Renderbuffer::getInternalFormat() const
{
	return internal_format;
}

int Renderbuffer::getWidth() const
{
	return width;
}

int Renderbuffer::getHeight() const
{
	return height;
}

