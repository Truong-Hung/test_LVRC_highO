#include "renderer/opengl/utils/FrameBufferObject.hpp"
#include <iostream>

FrameBufferObject::FrameBufferObject():
    framebufferObjectName(0),
    attachedTextures(),
    attachedRbos()
{}

FrameBufferObject::~FrameBufferObject()
{
    if(!isGenerated())
        return;
    glDeleteFramebuffers(1, &framebufferObjectName);
}

GLuint FrameBufferObject::getName() const
{
    return framebufferObjectName;
}

void FrameBufferObject::gen()
{
    if(isGenerated())
        return;
    glGenFramebuffers(1, &framebufferObjectName);
}

void FrameBufferObject::bind() const
{
    if (!isGenerated())
        throw std::runtime_error("Framebuffer is not generated");
    glBindFramebuffer(GL_FRAMEBUFFER, framebufferObjectName);
}

void FrameBufferObject::unbind() const
{
    unbindAnyFrameBuffer();
}

void FrameBufferObject::unbindAnyFrameBuffer()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

#include <iostream>
void FrameBufferObject::attachNewTexture(TextureBase::FormatEnum formatEnum, GLint filter, GLint wrap)
{
    std::shared_ptr<Texture<TextureBase::Texture2D>> newTexture = std::make_shared<Texture<TextureBase::Texture2D>>(formatEnum);
    newTexture->gen();
    newTexture->bind();
    newTexture->setFilterAndWrapParameters(filter, wrap);
    newTexture->allocateAndFill();
    newTexture->unbind();

    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + static_cast<GLenum>(attachedTextures.size()), newTexture->getName(), 0);

    attachedTextures.emplace_back(newTexture);
}

void FrameBufferObject::attachExistingTexture(const std::shared_ptr<Texture<TextureBase::Texture2D>>& texture)
{
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + static_cast<GLenum>(attachedTextures.size()), texture->getName(), 0);

    attachedTextures.emplace_back(texture);
}

void FrameBufferObject::setDrawBuffersForTextures() const
{
    size_t nbAttachedTextures = attachedTextures.size();
    std::vector<GLenum> buff{};
    for(uint32_t colorAttachmentOffset = 0; colorAttachmentOffset < nbAttachedTextures; colorAttachmentOffset++)
    {
        buff.push_back(GL_COLOR_ATTACHMENT0 + colorAttachmentOffset);
    }

    glDrawBuffers(static_cast<GLsizei>(buff.size()), buff.data());
}

void FrameBufferObject::attachNewRbo(RenderBufferObject::FormatEnum formatEnum)
{
    std::shared_ptr<RenderBufferObject> newRbo = std::make_shared<RenderBufferObject>(formatEnum);
    newRbo->gen();
    newRbo->bind();
    newRbo->allocate();
    newRbo->unbind();

    glFramebufferRenderbuffer(GL_FRAMEBUFFER, newRbo->getAttachmentPoint(), GL_RENDERBUFFER, newRbo->getName());

    attachedRbos.emplace_back(newRbo);
}

void FrameBufferObject::allocateTexturesAndRbo(int width, int height)
{
    for(auto & texture : attachedTextures)
    {
        texture->bind();
        texture->allocateAndFill(nullptr, width, height);
        texture->unbind();
    }
    for(auto & rbo : attachedRbos)
    {
        rbo->bind();
        rbo->allocate(width, height);
        rbo->unbind();
    }
}

std::shared_ptr<Texture<TextureBase::Texture2D>> & FrameBufferObject::getAttachedTexture(uint32_t attachedTextureIndex)
{
    return attachedTextures[attachedTextureIndex];
}

bool FrameBufferObject::isGenerated() const
{
    return framebufferObjectName != 0;
}