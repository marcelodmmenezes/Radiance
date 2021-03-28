#ifndef RENDERBUFFER_HPP
#define RENDERBUFFER_HPP

#include <glad/glad.h>

class Renderbuffer
{
public:
	Renderbuffer(
		GLenum internal_format,
		int width,
		int height);

	virtual ~Renderbuffer()
	{}

	void destroy();

	GLuint getId() const;
	GLenum getInternalFormat() const;
	int getWidth() const;
	int getHeight() const;

private:
	GLuint id;
	GLenum internal_format;
	int width;
	int height;
};

#endif // RENDERBUFFER_HPP

