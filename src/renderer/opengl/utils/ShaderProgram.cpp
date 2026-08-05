#include "renderer/opengl/utils/ShaderProgram.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <glm/gtc/type_ptr.hpp>

#include "renderer/opengl/utils/ShaderIncludes.hpp"

ShaderProgram::ShaderProgram() :
	linked(false),
	shaderProgramId(glCreateProgram())
{
}

ShaderProgram::~ShaderProgram()
{
	glDeleteProgram(shaderProgramId);
}

void ShaderProgram::addShaderFromFile(ShaderProgram::ShaderTypes type,
                                      const std::string& path)
{
	std::ifstream file(path, std::ios::in | std::ios::binary);

	if (!file.good())
	{
		std::cerr << "Unable to open file '" << path << "'." << std::endl;
		throw std::runtime_error("Unable to open file '" + path + "'.");
	}

	file.seekg(0, std::ios::end);
	auto fileSize = file.tellg();
	file.seekg(0, std::ios::beg);

	std::string source(fileSize, '\0');
	file.read(source.data(), fileSize);

	file.close();

	return addShaderFromSources(type, source, path);
}

void ShaderProgram::addShaderFromSources(ShaderProgram::ShaderTypes type,
                                         const std::string& source,
                                         const std::string& path)
{
	GLint success;
	char infoLog[512];

	GLuint shader = glCreateShader(type);
	const auto shader_sources = ShaderIncluder(source, path);
	auto cSource = shader_sources.unwrapped_code.c_str();
	glShaderSource(shader, 1, &cSource, nullptr);

	glCompileShader(shader);

	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(shader, 512, nullptr, infoLog);
		shader_sources.print_error("Shader compilation error", infoLog);
	}

	glAttachShader(shaderProgramId, shader);

	linked = false;

	glDeleteShader(shader);
}

void ShaderProgram::link()
{
	GLint success;
	char infoLog[512];

	if (linked)
	{
		return;
	}
	glLinkProgram(shaderProgramId);

	glGetProgramiv(shaderProgramId, GL_LINK_STATUS, &success);
	if (!success)
	{
		glGetProgramInfoLog(shaderProgramId, 512, nullptr, infoLog);
		std::cerr << "Shader program linking error:\n" << std::string(infoLog) << std::endl;
		throw std::runtime_error(
			"Shader program linking error:\n" + std::string(infoLog)
		);
	}

	linked = true;
}

void ShaderProgram::use()
{
	if (linked)
	{
		glUseProgram(shaderProgramId);
	}
}

void ShaderProgram::release()
{
	glUseProgram(0);
}

GLuint ShaderProgram::getGlId() const
{
	return shaderProgramId;
}

int ShaderProgram::getUniformLocation(const std::string& name) const
{
	const int location = glGetUniformLocation(shaderProgramId, name.c_str());
	if (location == -1)
	{
		std::cerr << "\033[33m" << "Warning: unable to locate uniform with name '" << name << "'" << "\033[0m"
			<< std::endl;
	}

	return location;
}

void ShaderProgram::setUniform(const std::string& name, int value)
{
	setUniform(getUniformLocation(name), value);
}

void ShaderProgram::setUniform(int location, int value)
{
	glUniform1i(location, value);
}

void ShaderProgram::setUniform(const std::string& name, float value)
{
	setUniform(getUniformLocation(name), value);
}

void ShaderProgram::setUniform(int location, float value)
{
	glUniform1f(location, value);
}

void ShaderProgram::setUniform(const std::string& name, unsigned int value)
{
	setUniform(getUniformLocation(name), value);
}

void ShaderProgram::setUniform(int location, unsigned int value)
{
	glUniform1ui(location, value);
}

void ShaderProgram::setUniform(const std::string& name,
                               const glm::vec2& vector)
{
	setUniform(getUniformLocation(name), vector);
}

void ShaderProgram::setUniform(int location, const glm::vec2& vector)
{
	glUniform2fv(location, 1, glm::value_ptr(vector));
}

void ShaderProgram::setUniform(const std::string& name,
                               const glm::vec3& vector)
{
	setUniform(getUniformLocation(name), vector);
}

void ShaderProgram::setUniform(int location, const glm::vec3& vector)
{
	glUniform3fv(location, 1, glm::value_ptr(vector));
}

void ShaderProgram::setUniform(const std::string& name,
                               const glm::vec4& vector)
{
	setUniform(getUniformLocation(name), vector);
}

void ShaderProgram::setUniform(int location, const glm::vec4& vector)
{
	glUniform4fv(location, 1, glm::value_ptr(vector));
}

void ShaderProgram::setUniform(const std::string& name,
                               const glm::ivec2& vector)
{
	setUniform(getUniformLocation(name), vector);
}

void ShaderProgram::setUniform(int location, const glm::ivec2& vector)
{
	glUniform2iv(location, 1, glm::value_ptr(vector));
}

void ShaderProgram::setUniform(const std::string& name, const glm::mat4& mat)
{
	setUniform(getUniformLocation(name), mat);
}

void ShaderProgram::setUniform(int location, const glm::mat4& mat)
{
	glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(mat));
}
