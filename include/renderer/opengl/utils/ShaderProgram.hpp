#ifndef LVRC_SHADERPROGRAM_HPP
#define LVRC_SHADERPROGRAM_HPP

#include <filesystem>
#include <glad/glad.h>
#include <string>
#include <glm/glm.hpp>

class ShaderProgram {
public:
    enum ShaderTypes {
        VertexShader = GL_VERTEX_SHADER,
        FragmentShader = GL_FRAGMENT_SHADER,
        ComputeShader = GL_COMPUTE_SHADER
    };

public:
    ShaderProgram();
    ~ShaderProgram();

    void addShaderFromFile(ShaderTypes type, const std::string& path);
    void addShaderFromSources(ShaderTypes type, const std::string& sources, const std::string& path = "");
    
    void link();

    void use();
    void release();

    [[nodiscard]] GLuint getGlId() const;

    [[nodiscard]] int getUniformLocation(const std::string& name) const;

    void setUniform(const std::string& name, int value);
    void setUniform(int location, int value);

    void setUniform(const std::string& name, float value);
    void setUniform(int location, float value);

    void setUniform(const std::string& name, unsigned int value);
    void setUniform(int location, unsigned int value);

    void setUniform(const std::string& name, const glm::vec2& vector);
    void setUniform(int location, const glm::vec2& vector);

    void setUniform(const std::string& name, const glm::vec3& vector);
    void setUniform(int location, const glm::vec3& vector);

    void setUniform(const std::string& name, const glm::vec4& vector);
    void setUniform(int location, const glm::vec4& vector);

    void setUniform(const std::string& name, const glm::ivec2& vector);
    void setUniform(int location, const glm::ivec2& vector);

    void setUniform(const std::string& name, const glm::mat4& mat);
    void setUniform(int location, const glm::mat4& mat);

private:
    bool linked;
    GLuint shaderProgramId;
};

#endif //LVRC_SHADERPROGRAM_HPP
