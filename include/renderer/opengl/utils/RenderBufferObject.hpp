#ifndef LVRC_RENDERBUFFEROBJECT_HPP
#define LVRC_RENDERBUFFEROBJECT_HPP

#include <glad/glad.h>

class RenderBufferObject
{
public:
    enum FormatEnum
    {
        INVALID_ENUM = -1,
        DEPTH_COMPONENT,
        DEPTH_COMPONENT16,
        DEPTH_COMPONENT24,
        DEPTH_COMPONENT32F,
        DEPTH24_STENCIL8,
        DEPTH32F_STENCIL8,
        STENCIL_INDEX8
    };

private:
    struct RboFormatAttachmentPoint
    {
        GLint internalFormat;
        GLenum attachmentPoint;
    };

public:
    RenderBufferObject(FormatEnum formatEnum = INVALID_ENUM);
    ~RenderBufferObject();

    GLuint getName() const;
    GLenum getAttachmentPoint() const;
    void gen();
    void bind() const;
    void unbind() const;
    void allocate(int width = 0, int height = 0) const;

private:
    inline bool isGenerated() const;
    inline bool isFormatSet() const;

    inline RboFormatAttachmentPoint getRboFormatAttachmentPoint() const;
    static constexpr RboFormatAttachmentPoint getAttachmentPoint(FormatEnum formatEnum);

private:
    GLuint renderBufferObjectName;
    FormatEnum renderBufferObjectFormatEnum;
};

constexpr RenderBufferObject::RboFormatAttachmentPoint RenderBufferObject::getAttachmentPoint(FormatEnum formatEnum)
{
    constexpr RenderBufferObject::RboFormatAttachmentPoint attachmentPointList[] =
    {
        { GL_DEPTH_COMPONENT,       GL_DEPTH_ATTACHMENT },
        { GL_DEPTH_COMPONENT16,     GL_DEPTH_ATTACHMENT },
        { GL_DEPTH_COMPONENT24,     GL_DEPTH_ATTACHMENT },
        { GL_DEPTH_COMPONENT32F,    GL_DEPTH_ATTACHMENT },
        { GL_DEPTH24_STENCIL8,      GL_DEPTH_STENCIL_ATTACHMENT },
        { GL_DEPTH32F_STENCIL8,     GL_DEPTH_STENCIL_ATTACHMENT },
        { GL_STENCIL_INDEX8,        GL_STENCIL_ATTACHMENT }
    };
    return attachmentPointList[formatEnum];
}

#endif //LVRC_RENDERBUFFEROBJECT_HPP