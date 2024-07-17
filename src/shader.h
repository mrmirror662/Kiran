#pragma once
#include<string>
#include<unordered_map>
std::string ShaderTypeToString(unsigned int shaderType);

int CompileShader(const std::string& sh, unsigned int type);
int CreateShader(const std::string& fragSh, const std::string& vertSh);
std::string ReadShFile(const std::string& path);


class Shader
{
	int id;
	std::unordered_map<std::string, int> uniform_map;
	int getUniformLocation(const std::string& uniform_name);
public:
	Shader(const std::string& fragSh, const std::string& vertSh);
	void bind();
	void unBind();
	int getId();
	bool initUniForm(const std::string& uniform_name);
	void setUniform(const std::string& name, float val);
	void setUniform(const std::string& name, float v1, float v2);
	void setUniform(const std::string& name, float v1, float v2, float v3);

	void setUniform(const std::string& name, int val);
	void setUniform(const std::string& name, int v1, int v2);
	void setUniform(const std::string& name, int v1, int v2, int v3);

	~Shader();

};