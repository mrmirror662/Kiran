#include "BindlessTexture.h"
#include<cassert>
BindlessTexture::BindlessTexture(int width, int height, int channel, std::vector<uint8_t> buffer)
{
	this->width = width;
	this->height = height;
	this->buffer = std::move(buffer);
	this->size = width * height * channel;
	this->channel = channel;

	glCreateTextures(GL_TEXTURE_2D, 1, &this->textureId);
	assert(!(channel != 3 && channel != 4));


	if (channel == 3)
		glTextureStorage2D(this->textureId, 1, GL_RGB8, width, height);
	if (channel == 4)
		glTextureStorage2D(this->textureId, 1, GL_RGBA8, width, height);

	glTextureSubImage2D(
		this->textureId,
		0, 0, 0, this->width, this->height, // level, xoffset, yoffset, width, height
		GL_RGB, GL_UNSIGNED_BYTE,
		(const void*)this->buffer.data());

	// Set the texture wrapping mode to repeat
	glTextureParameteri(this->textureId, GL_TEXTURE_WRAP_S, GL_REPEAT);  // X axis
	glTextureParameteri(this->textureId, GL_TEXTURE_WRAP_T, GL_REPEAT);  // Y axis
	// Set texture filtering to nearest or linear
	glTextureParameteri(this->textureId, GL_TEXTURE_MIN_FILTER, GL_LINEAR); // Minification
	glTextureParameteri(this->textureId, GL_TEXTURE_MAG_FILTER, GL_LINEAR); // Magnification
	this->textureHandle = glGetTextureHandleARB(this->textureId);

	if (this->textureHandle == 0) {
		throw std::runtime_error("Couldn't initialize and get handle of Bindless Texture");
		exit(-1);
	}

}
void BindlessTexture::MakeResident()
{
	glMakeTextureHandleResidentARB(this->textureHandle);
}
void BindlessTexture::MakeNonResident()
{
	glMakeTextureHandleNonResidentARB(this->textureHandle);
}
uint64_t  BindlessTexture::GetHandle()
{
	return this->textureHandle;
}
uint32_t  BindlessTexture::GetId()
{
	return this->textureId;
}

bool BindlessTexture::IsResident()
{
	return glIsTextureHandleResidentARB(this->textureHandle);
}
