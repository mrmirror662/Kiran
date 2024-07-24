#pragma once

#include<glad/glad.h>
#include <vector>
#include<glm/glm.hpp>

class Texture
{
	unsigned int id;
	int width, height;
public:
	Texture(int width, int height, const std::vector<glm::vec4>& buffer);
	Texture(int width, int height);
	Texture();
	Texture(Texture&& other);
	static void setActiveUnit(unsigned int);
	void bind();
	void unBind();
	void reSize(int new_width, int new_height);
	void reSize(int new_width, int new_height, const std::vector<glm::vec4>& buffer);

	unsigned int getId();
};

/*
*		glGenFramebuffers(1, &fbo[i]);
		glGenTextures(1, &texture[i]);
		glBindTexture(GL_TEXTURE_2D, texture[i]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glBindFramebuffer(GL_FRAMEBUFFER, fbo[i]);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture[i], 0);
*/
