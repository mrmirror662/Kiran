#pragma once
#include <glad/glad.h>
#include <vector>

template <typename T> class ShaderStorage {
  unsigned int id;

public:
  ShaderStorage();
  void fillData(const std::vector<T> &data);
  void subData(const std::vector<T> &data, size_t offset);
  void bind();
  void unBind();
  void bindBase(int index);
  auto getId();
};

template <typename T> ShaderStorage<T>::ShaderStorage() {
  glGenBuffers(1, &this->id);
}

template <typename T>
void ShaderStorage<T>::fillData(const std::vector<T> &data) {
  this->bind();
  glBufferData(GL_SHADER_STORAGE_BUFFER, data.size() * sizeof(T), data.data(),
               GL_DYNAMIC_DRAW);
}
template <typename T>
void ShaderStorage<T>::subData(const std::vector<T> &data, size_t offset) {
  this->bind();
  glBufferSubData(GL_SHADER_STORAGE_BUFFER, offset, data.size() * sizeof(T),
                  data.data());
}
template <typename T> void ShaderStorage<T>::bind() {
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, this->id);
}

template <typename T> void ShaderStorage<T>::unBind() {
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

template <typename T> void ShaderStorage<T>::bindBase(int index) {
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, index, this->id);
}
template <typename T> auto ShaderStorage<T>::getId() { return this->id; }
