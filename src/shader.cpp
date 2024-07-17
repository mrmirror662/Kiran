#include "shader.h"
#include<iostream>
#include<fstream>
#include<sstream>
#include<glad/glad.h>
std::string ShaderTypeToString(unsigned int shaderType) {
	switch (shaderType) {
	case GL_VERTEX_SHADER:
		return "GL_VERTEX_SHADER";
	case GL_FRAGMENT_SHADER:
		return "GL_FRAGMENT_SHADER";
		// Add other cases for different shader types if needed
		// case GL_GEOMETRY_SHADER:
		//     return "GL_GEOMETRY_SHADER";
		// case GL_COMPUTE_SHADER:
		//     return "GL_COMPUTE_SHADER";
	default:
		return "Unknown Shader Type";
	}
}
int CompileShader(const std::string& sh, unsigned int type)
{
	unsigned int id = glCreateShader(type);
	auto src = sh.data();
	const auto size = (int)sh.size();
	glShaderSource(id, 1, &src, &size);
	glCompileShader(id);
	int result;
	glGetShaderiv(id, GL_COMPILE_STATUS, &result);
	if (result == GL_FALSE)
	{
		int length;
		glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length);
		std::string msg;
		msg.resize(length);
		glGetShaderInfoLog(id, length, &length, (char*)msg.data());
		std::cout << "shader compiler:" << msg << " type:" << ShaderTypeToString(type) << "\nsrc:\n" << sh << '\n';
		return -1;
	}
	return id;
}

int CreateShader(const std::string& fragSh, const std::string& vertSh)
{
	unsigned int  program = glCreateProgram();
	unsigned int vs = CompileShader(vertSh, GL_VERTEX_SHADER);
	unsigned int fs = CompileShader(fragSh, GL_FRAGMENT_SHADER);
	if (vs == -1 || fs == -1)
		return -1;
	glAttachShader(program, vs);
	glAttachShader(program, fs);
	glLinkProgram(program);
	glValidateProgram(program);
	glDeleteShader(vs);
	glDeleteShader(fs);
	return program;
}
std::string ReadShFile(const std::string& path)
{
	std::ifstream file(path);

	if (!file.is_open()) {
		throw std::runtime_error("Could not open file");
	}

	std::ostringstream ss{};
	ss << file.rdbuf();
	return ss.str();

}



Shader::Shader(const std::string& fragSh, const std::string& vertSh)
{
	auto vertSrc = ReadShFile(vertSh);
	auto fragSrc = ReadShFile(fragSh);
	this->id = CreateShader(fragSrc, vertSrc);
}

void Shader::bind()
{
	glUseProgram(id);
}

void Shader::unBind()
{
	glUseProgram(0);
}

int Shader::getId()
{
	return this->id;
}

bool Shader::initUniForm(const std::string& uniform_name)
{
	if (uniform_map.find(uniform_name) != uniform_map.end())
		return true;
	int uniform_location = glGetUniformLocation(this->id, uniform_name.c_str());
	if (uniform_location == -1) return false;
	uniform_map[uniform_name] = uniform_location;
	return true;
}
int Shader::getUniformLocation(const std::string& uniform_name)
{
	if (uniform_map.find(uniform_name) == uniform_map.end())
		return -1;
	return this->uniform_map[uniform_name];
}
//set floats careful while passing types
void Shader::setUniform(const std::string& name, float v1)
{
	int uniform_location = getUniformLocation(name);
	glUniform1f(uniform_location, v1);
}

void Shader::setUniform(const std::string& name, float v1, float v2)
{
	int uniform_location = getUniformLocation(name);
	glUniform2f(uniform_location, v1, v2);
}
void Shader::setUniform(const std::string& name, float v1, float v2, float v3)
{
	int uniform_location = getUniformLocation(name);
	glUniform3f(uniform_location, v1, v2, v3);
}
//set ints 
void Shader::setUniform(const std::string& name, int v1)
{
	int uniform_location = getUniformLocation(name);
	glUniform1i(uniform_location, v1);
}

void Shader::setUniform(const std::string& name, int v1, int v2)
{
	int uniform_location = getUniformLocation(name);
	glUniform2i(uniform_location, v1, v2);
}
void Shader::setUniform(const std::string& name, int v1, int v2, int v3)
{
	int uniform_location = getUniformLocation(name);
	glUniform3i(uniform_location, v1, v2, v3);
}


Shader::~Shader()
{
	this->unBind();
}
