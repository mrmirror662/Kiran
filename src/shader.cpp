#include "shader.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <glad/glad.h>
std::string ShaderTypeToString(unsigned int shaderType)
{
	switch (shaderType)
	{
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
int CompileShader(const std::string &sh, unsigned int type)
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
		glGetShaderInfoLog(id, length, &length, (char *)msg.data());
		std::cout << "shader compiler:" << msg << " type:" << ShaderTypeToString(type) << "\nsrc:\n"
				  << sh << '\n';
		return -1;
	}
	return id;
}

int CreateShader(const std::string &fragSh, const std::string &vertSh)
{
	unsigned int program = glCreateProgram();
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
std::string ReadShFile(const std::string &path)
{
	std::ifstream file(path);

	if (!file.is_open())
	{
		throw std::runtime_error("Could not open file");
	}

	std::ostringstream ss{};
	ss << file.rdbuf();
	return ss.str();
}

Shader::Shader(const std::string &fragSh, const std::string &vertSh)
{
	auto vertSrc = ReadShFile(vertSh);
	auto fragSrc = ReadShFile(fragSh);
	this->id = CreateShader(fragSrc, vertSrc);
}

// Renamed for clarity
bool Shader::initUniform(const std::string &uniform_name)
{
	if (uniform_map.find(uniform_name) != uniform_map.end())
		return true;
	int uniform_location = glGetUniformLocation(this->id, uniform_name.c_str());
	if (uniform_location == -1)
	{
		std::cerr << "Warning: Uniform '" << uniform_name << "' not found in shader program!" << std::endl;
		return false;
	}
	uniform_map[uniform_name] = uniform_location;
	return true;
}
int Shader::getUniformLocation(const std::string &uniform_name)
{
	if (uniform_map.find(uniform_name) == uniform_map.end())
		return -1;
	return this->uniform_map[uniform_name];
}
// set floats careful while passing types
void Shader::setUniform(const std::string &name, float v1)
{
	int uniform_location = getUniformLocation(name);
	if (uniform_location == -1)
	{
		std::cerr << "Warning: setUniform called for '" << name << "' but location is -1!" << std::endl;
		return;
	}
	glUniform1f(uniform_location, v1);
}

void Shader::setUniform(const std::string &name, float v1, float v2)
{
	int uniform_location = getUniformLocation(name);
	if (uniform_location == -1)
	{
		std::cerr << "Warning: setUniform called for '" << name << "' but location is -1!" << std::endl;
		return;
	}
	glUniform2f(uniform_location, v1, v2);
}
void Shader::setUniform(const std::string &name, float v1, float v2, float v3)
{
	int uniform_location = getUniformLocation(name);
	if (uniform_location == -1)
	{
		std::cerr << "Warning: setUniform called for '" << name << "' but location is -1!" << std::endl;
		return;
	}
	glUniform3f(uniform_location, v1, v2, v3);
}
// set ints
void Shader::setUniform(const std::string &name, int v1)
{
	int uniform_location = getUniformLocation(name);
	if (uniform_location == -1)
	{
		std::cerr << "Warning: setUniform called for '" << name << "' but location is -1!" << std::endl;
		return;
	}
	glUniform1i(uniform_location, v1);
}

void Shader::setUniform(const std::string &name, int v1, int v2)
{
	int uniform_location = getUniformLocation(name);
	if (uniform_location == -1)
	{
		std::cerr << "Warning: setUniform called for '" << name << "' but location is -1!" << std::endl;
		return;
	}
	glUniform2i(uniform_location, v1, v2);
}
void Shader::setUniform(const std::string &name, int v1, int v2, int v3)
{
	int uniform_location = getUniformLocation(name);
	if (uniform_location == -1)
	{
		std::cerr << "Warning: setUniform called for '" << name << "' but location is -1!" << std::endl;
		return;
	}
	glUniform3i(uniform_location, v1, v2, v3);
}

void Shader::setUniform(const std::string &name, uint64_t v1)
{
	int uniform_location = getUniformLocation(name);
	if (uniform_location == -1)
	{
		std::cerr << "Warning: setUniform called for '" << name << "' but location is -1!" << std::endl;
		return;
	}
	glUniformHandleui64ARB(uniform_location, v1);
}

Shader::Shader(const std::string &compSh)
{
	auto compSrc = ReadShFile(compSh);
	unsigned int program = glCreateProgram();
	unsigned int cs = CompileShader(compSrc, GL_COMPUTE_SHADER);
	if (cs == -1)
	{
		id = -1;
		return;
	}
	glAttachShader(program, cs);
	glLinkProgram(program);
	GLint status;
	glGetProgramiv(program, GL_LINK_STATUS, &status);
	if (status == GL_FALSE)
	{
		GLint len;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &len);
		std::string log(len, '\0');
		glGetProgramInfoLog(program, len, nullptr, log.data());
		std::cerr << "Program link failed:\n"
				  << log << std::endl;
		glDeleteProgram(program);
		id = -1; // Set id to -1 on failure
		return;
	}
	glValidateProgram(program);
	glDeleteShader(cs);
	id = program;
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

Shader::~Shader()
{
	this->unBind();
}
