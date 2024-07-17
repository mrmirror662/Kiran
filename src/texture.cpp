#include "texture.h"

Texture::Texture(int width, int height, const std::vector<glm::vec4>& buffer)
{
	glGenTextures(1, &id);
	this->width = width;
	this->height = height;

	this->bind();
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, this->width, this->height, 0, GL_RGBA, GL_FLOAT, buffer.data());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}

Texture::Texture(int width, int height)
{
	glGenTextures(1, &id);
	this->width = width;
	this->height = height;

	this->bind();
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, this->width, this->height, 0, GL_RGBA, GL_FLOAT, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}
Texture::Texture()
{
	this->id = 0;
	this->width = -1;
	this->height = -1;
}
Texture::Texture(Texture&& other)
{
	this->id = other.id;
	this->width = other.width;
	this->height = other.height;
	other.id = 0;
	other.width = other.height = -1;

}

void Texture::setActiveUnit(unsigned int unit)
{
	glActiveTexture(GL_TEXTURE0 + unit);
}

void Texture::bind()
{
	glBindTexture(GL_TEXTURE_2D, id);

}

void Texture::unBind()
{
	glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture::reSize(int new_width, int new_height)
{
	glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture first
	glDeleteTextures(1, &this->id); // Delete the texture
	auto t = Texture(new_width, new_height);
	this->id = t.id;
	this->width = t.width;
	this->height = t.height;
	t.id = 0;

}
void Texture::reSize(int new_width, int new_height, const std::vector<glm::vec4>& buffer)
{
	glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture first
	glDeleteTextures(1, &this->id); // Delete the texture
	auto t = Texture(new_width, new_height, buffer);
	this->id = t.id;
	this->width = t.width;
	this->height = t.height;
	t.id = 0;

}
unsigned int Texture::getId()
{
	return id;
}
