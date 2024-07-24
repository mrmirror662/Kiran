#pragma once

#include "texture.h"
class FrameBuffer {
  unsigned int id;

public:
  FrameBuffer() { glGenFramebuffers(1, &id); }
  void bind() { glBindFramebuffer(GL_FRAMEBUFFER, id); }
  void unbind() { glBindFramebuffer(GL_FRAMEBUFFER, 0); }
  void attachTexure(Texture &t) {
    this->bind();
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           t.getId(), 0);
    this->unbind();
  }
};
