#ifndef LVRC_FRAMEBUFFEROBJECT_HPP
#define LVRC_FRAMEBUFFEROBJECT_HPP

#include <glad/glad.h>
#include <vector>
#include <memory>

#include "renderer/opengl/utils/Texture.hpp"
#include "renderer/opengl/utils/RenderBufferObject.hpp"

class FrameBufferObject
{
public:
    FrameBufferObject();
    ~FrameBufferObject();

    GLuint getName() const;
    void gen();
    void bind() const;
    void unbind() const;
    static void unbindAnyFrameBuffer();

    void attachNewTexture(TextureBase::FormatEnum formatEnum, GLint filter = GL_LINEAR, GLint wrap = GL_REPEAT);
    void attachExistingTexture(const std::shared_ptr<Texture<TextureBase::Texture2D>>& texture);
    void setDrawBuffersForTextures() const;
    void attachNewRbo(RenderBufferObject::FormatEnum formatEnum);
    void allocateTexturesAndRbo(int width = 1, int height = 1);

    std::shared_ptr<Texture<TextureBase::Texture2D>> & getAttachedTexture(uint32_t attachedTextureIndex = 0);

private:
    inline bool isGenerated() const;

private:
    GLuint framebufferObjectName;
    std::vector<std::shared_ptr<Texture<TextureBase::Texture2D>>> attachedTextures;
    std::vector<std::shared_ptr<RenderBufferObject>> attachedRbos;
};

#endif //LVRC_FRAMEBUFFEROBJECT_HPP