#include "renderer/opengl/utils/RenderBufferObject.hpp"

RenderBufferObject::RenderBufferObject(FormatEnum formatEnum):
    renderBufferObjectName(0),
    renderBufferObjectFormatEnum(formatEnum)
{
}

RenderBufferObject::~RenderBufferObject()
{
    if(!isGenerated())
        return;
    glDeleteRenderbuffers(1, &renderBufferObjectName);
}

GLuint RenderBufferObject::getName() const
{
    // if(!isGenerated())
    //     return;
    return renderBufferObjectName;
}

GLenum RenderBufferObject::getAttachmentPoint() const
{
    // if(!isFormatSet())
    //     return;
    return getRboFormatAttachmentPoint().attachmentPoint;
}

void RenderBufferObject::gen()
{
    if(isGenerated())
        return;
    glGenRenderbuffers(1, &renderBufferObjectName);
}

void RenderBufferObject::bind() const
{
    glBindRenderbuffer(GL_RENDERBUFFER, renderBufferObjectName); 
}

void RenderBufferObject::unbind() const
{
    glBindRenderbuffer(GL_RENDERBUFFER, 0); 
}

void RenderBufferObject::allocate(int width, int height) const
{
    if(!isGenerated())
        return;
    if(!isFormatSet())
        return;
    glRenderbufferStorage(GL_RENDERBUFFER, getRboFormatAttachmentPoint().internalFormat, width, height);
}

bool RenderBufferObject::isGenerated() const
{
    return renderBufferObjectName != 0;
}

bool RenderBufferObject::isFormatSet() const
{
    return renderBufferObjectFormatEnum != RenderBufferObject::INVALID_ENUM;
}

RenderBufferObject::RboFormatAttachmentPoint RenderBufferObject::getRboFormatAttachmentPoint() const
{
    return getAttachmentPoint(renderBufferObjectFormatEnum);
}