#ifndef LVRC_TEXTURE_HPP
#define LVRC_TEXTURE_HPP

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>

class TextureBase
{
public:
    enum TextureType
    {
        Texture1D = GL_TEXTURE_1D,
        Texture2D = GL_TEXTURE_2D,
        Texture3D = GL_TEXTURE_3D
    };

    enum StorageType
    {
        Immutable,
        Mutable
    };

    enum FormatEnum
    {
        INVALID_ENUM = -1,
        R8, RG8, RGB8, RGBA8,
        R16, RG16, RGB16, RGBA16,
        R16F, RG16F, RGB16F, RGBA16F,
        R32F, RG32F, RGB32F, RGBA32F,
        R16I, RG16I, RGB16I, RGBA16I,
        R32I, RG32I, RGB32I, RGBA32I,
        R8UI, RG8UI, RGB8UI, RGBA8UI,
        R16UI, RG16UI, RGB16UI, RGBA16UI,
        R32UI, RG32UI, RGB32UI, RGBA32UI,
        RGB10_A2,
        DEPTH_COMPONENT, DEPTH_COMPONENT16, DEPTH_COMPONENT24, DEPTH_COMPONENT32F
    };

protected:
    struct TextureFormat
    {
        GLint internalFormat;
        GLenum pixelFormat;
        GLenum pixelType;
    };


public:
    inline TextureBase(TextureBase::FormatEnum formatEnum);
    inline ~TextureBase();

    inline bool isGenerated() const;
    inline GLuint getName() const;
    inline void gen();
    inline void bind() const;
    inline void unbind() const;
    inline void activeAndBind(GLuint textureUnitOffset = 0) const;
    inline void bindAsImageTexture(GLuint bindingUnit, GLenum access) const;
    inline void setFilterAndWrapParameters(GLint filter = GL_LINEAR, GLint wrap = GL_REPEAT) const;

protected:
    inline bool isFormatSet() const;
    virtual GLenum getTarget() const = 0;

    inline TextureFormat getTextureFormat() const;
    static constexpr TextureFormat getTextureFormat(FormatEnum format);

protected:
    GLuint textureName;
    FormatEnum textureFormatEnum;
};

TextureBase::TextureBase(TextureBase::FormatEnum formatEnum):
    textureName(0),
    textureFormatEnum(formatEnum)
{}

TextureBase::~TextureBase()
{
    if(!isGenerated())
        return;
    glDeleteTextures(1, &textureName);
}

GLuint TextureBase::getName() const
{
    return textureName;
}

void TextureBase::gen()
{
    if(isGenerated())
        return;
    glGenTextures(1, &textureName);
}

void TextureBase::bind() const
{
    glBindTexture(getTarget(), textureName);
}

void TextureBase::unbind() const
{
    glBindTexture(getTarget(), 0);
}

void TextureBase::activeAndBind(GLuint textureUnitOffset) const
{
    glActiveTexture(GL_TEXTURE0 + textureUnitOffset);
    glBindTexture(getTarget(), textureName);
}

void TextureBase::bindAsImageTexture(GLuint bindingUnit, GLenum access) const
{
    glBindImageTexture(bindingUnit, textureName, 0, GL_FALSE, 0, access, getTextureFormat().internalFormat);
}

void TextureBase::setFilterAndWrapParameters(GLint filter, GLint wrap) const
{
    if(!isGenerated())
        return;
    glTexParameteri(getTarget(), GL_TEXTURE_MIN_FILTER, filter);
    glTexParameteri(getTarget(), GL_TEXTURE_MAG_FILTER, filter);
    glTexParameteri(getTarget(), GL_TEXTURE_WRAP_S, wrap);
    glTexParameteri(getTarget(), GL_TEXTURE_WRAP_T, wrap);
}

bool TextureBase::isGenerated() const
{
    return textureName != 0;
}

bool TextureBase::isFormatSet() const
{
    return textureFormatEnum != TextureBase::INVALID_ENUM;
}

TextureBase::TextureFormat TextureBase::getTextureFormat() const
{
    return TextureBase::getTextureFormat(textureFormatEnum);
}

constexpr TextureBase::TextureFormat TextureBase::getTextureFormat(FormatEnum formatEnum)
{
    constexpr TextureBase::TextureFormat textureFormatList[] =
    {
        { GL_R8,                GL_RED,             GL_UNSIGNED_BYTE },
        { GL_RG8,               GL_RG,              GL_UNSIGNED_BYTE },
        { GL_RGB8,              GL_RGB,             GL_UNSIGNED_BYTE },
        { GL_RGBA8,             GL_RGBA,            GL_UNSIGNED_BYTE },
        { GL_R16,               GL_RED,             GL_UNSIGNED_SHORT },
        { GL_RG16,              GL_RG,              GL_UNSIGNED_SHORT },
        { GL_RGB16,             GL_RGB,             GL_UNSIGNED_SHORT },
        { GL_RGBA16,            GL_RGBA,            GL_UNSIGNED_SHORT },
        { GL_R16F,              GL_RED,             GL_HALF_FLOAT },
        { GL_RG16F,             GL_RG,              GL_HALF_FLOAT },
        { GL_RGB16F,            GL_RGB,             GL_HALF_FLOAT },
        { GL_RGBA16F,           GL_RGBA,            GL_HALF_FLOAT },
        { GL_R32F,              GL_RED,             GL_FLOAT },
        { GL_RG32F,             GL_RG,              GL_FLOAT },
        { GL_RGB32F,            GL_RGB,             GL_FLOAT },
        { GL_RGBA32F,           GL_RGBA,            GL_FLOAT },
        { GL_R16I,              GL_RED_INTEGER,     GL_SHORT },
        { GL_RG16I,             GL_RG_INTEGER,      GL_SHORT },
        { GL_RGB16I,            GL_RGB_INTEGER,     GL_SHORT },
        { GL_RGBA16I,           GL_RGBA_INTEGER,    GL_SHORT },
        { GL_R32I,              GL_RED_INTEGER,     GL_INT },
        { GL_RG32I,             GL_RG_INTEGER,      GL_INT },
        { GL_RGB32I,            GL_RGB_INTEGER,     GL_INT },
        { GL_RGBA32I,           GL_RGBA_INTEGER,    GL_INT },
        { GL_R8UI,              GL_RED_INTEGER,     GL_UNSIGNED_BYTE },
        { GL_RG8UI,             GL_RG_INTEGER,      GL_UNSIGNED_BYTE },
        { GL_RGB8UI,            GL_RGB_INTEGER,     GL_UNSIGNED_BYTE },
        { GL_RGBA8UI,           GL_RGBA_INTEGER,    GL_UNSIGNED_BYTE },
        { GL_R16UI,             GL_RED_INTEGER,     GL_UNSIGNED_SHORT },
        { GL_RG16UI,            GL_RG_INTEGER,      GL_UNSIGNED_SHORT },
        { GL_RGB16UI,           GL_RGB_INTEGER,     GL_UNSIGNED_SHORT },
        { GL_RGBA16UI,          GL_RGBA_INTEGER,    GL_UNSIGNED_SHORT },
        { GL_R32UI,             GL_RED_INTEGER,     GL_UNSIGNED_INT },
        { GL_RG32UI,            GL_RG_INTEGER,      GL_UNSIGNED_INT },
        { GL_RGB32UI,           GL_RGB_INTEGER,     GL_UNSIGNED_INT },
        { GL_RGBA32UI,          GL_RGBA_INTEGER,    GL_UNSIGNED_INT },
        { GL_RGB10_A2,          GL_RGBA,            GL_UNSIGNED_INT_2_10_10_10_REV },
        { GL_DEPTH_COMPONENT,   GL_DEPTH_COMPONENT, GL_UNSIGNED_INT },
        { GL_DEPTH_COMPONENT16, GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT },
        { GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT },
        { GL_DEPTH_COMPONENT32F,GL_DEPTH_COMPONENT, GL_FLOAT }    
    };
    return textureFormatList[formatEnum];
}




template <TextureBase::TextureType TT>
class TextureBaseChild : public TextureBase
{
protected:
    TextureBaseChild(TextureBase::FormatEnum formatEnum):
        TextureBase(formatEnum)
    {};

    GLenum getTarget() const override;
};

template <TextureBase::TextureType TT>
GLenum TextureBaseChild<TT>::getTarget() const
{
    if constexpr(TT == TextureBase::Texture1D)
        return GL_TEXTURE_1D;
    else if constexpr(TT == TextureBase::Texture2D)
        return GL_TEXTURE_2D;
    else if constexpr(TT == TextureBase::Texture3D)
        return GL_TEXTURE_3D;
}




template <TextureBase::TextureType TT, TextureBase::StorageType ST = TextureBase::Mutable>
class Texture : public TextureBaseChild<TT>
{ };



template <TextureBase::TextureType TT>
class Texture<TT, TextureBase::Mutable> : public TextureBaseChild<TT>
{
public:
    Texture(TextureBase::FormatEnum formatEnum = TextureBase::INVALID_ENUM):
        TextureBaseChild<TT>(formatEnum)
    {};

    inline void setFormat(TextureBase::FormatEnum formatEnum);
    inline void allocateAndFill(TextureBase::FormatEnum formatEnum, const void * buffer = nullptr, int width = 1, int height = 0, int depht = 0);
    inline void allocateAndFill(const void * buffer = nullptr, int width = 1, int height = 1, int depht = 1);
};

template <TextureBase::TextureType TT>
void Texture<TT, TextureBase::Mutable>::setFormat(TextureBase::FormatEnum formatEnum)
{
    this->textureFormatEnum = formatEnum;
}

template <TextureBase::TextureType TT>
void Texture<TT, TextureBase::Mutable>::allocateAndFill(TextureBase::FormatEnum formatEnum, const void * buffer, int width, int height, int depht)
{
    if(!this->isGenerated())
        return;
    
    this->textureFormatEnum = formatEnum;

    auto texFmt = this->getTextureFormat();

    if constexpr(TT == TextureBase::Texture1D)
        glTexImage1D(GL_TEXTURE_1D, 0, texFmt.internalFormat, width, 0, texFmt.pixelFormat, texFmt.pixelType, buffer);
    else if constexpr(TT == TextureBase::Texture2D)
        glTexImage2D(GL_TEXTURE_2D, 0, texFmt.internalFormat, width, height, 0, texFmt.pixelFormat, texFmt.pixelType, buffer);
    else if constexpr(TT == TextureBase::Texture3D)
        glTexImage3D(GL_TEXTURE_3D, 0, texFmt.internalFormat, width, height, depht, 0, texFmt.pixelFormat, texFmt.pixelType, buffer);
}



template <TextureBase::TextureType TT>
void Texture<TT, TextureBase::Mutable>::allocateAndFill(const void * buffer, int width, int height, int depht)
{
    if(!this->isGenerated())
        return;
    if(!this->isFormatSet())
        return;

    auto texFmt = this->getTextureFormat();

    if constexpr(TT == TextureBase::Texture1D)
        glTexImage1D(GL_TEXTURE_1D, 0, texFmt.internalFormat, width, 0, texFmt.pixelFormat, texFmt.pixelType, buffer);
    else if constexpr(TT == TextureBase::Texture2D)
        glTexImage2D(GL_TEXTURE_2D, 0, texFmt.internalFormat, width, height, 0, texFmt.pixelFormat, texFmt.pixelType, buffer);
    else if constexpr(TT == TextureBase::Texture3D)
        glTexImage3D(GL_TEXTURE_3D, 0, texFmt.internalFormat, width, height, depht, 0, texFmt.pixelFormat, texFmt.pixelType, buffer);
}

template <TextureBase::TextureType TT>
class Texture<TT, TextureBase::Immutable> : public TextureBaseChild<TT>
{
public:
    Texture():
        TextureBaseChild<TT>(TextureBase::INVALID_ENUM)
    {};

    inline void allocate(TextureBase::FormatEnum formatEnum, int width = 1, int height = 1, int depht = 1);
    inline void fill(const void * buffer);
private:
    glm::ivec3 textureSize;
    bool isAllocated = false;
};

template <TextureBase::TextureType TT>
void Texture<TT, TextureBase::Immutable>::allocate(TextureBase::FormatEnum formatEnum, int width, int height, int depht)
{
    if(!this->isGenerated())
        return;
    if(isAllocated)
        return;
    
    this->textureFormatEnum = formatEnum;
    textureSize = glm::vec3(width, height, depht);

    auto texFmt = this->getTextureFormat();

    if constexpr(TT == TextureBase::Texture1D)
        glTexStorage1D(GL_TEXTURE_1D, 1, texFmt.internalFormat, textureSize.x);
    else if constexpr(TT == TextureBase::Texture2D)
        glTexStorage2D(GL_TEXTURE_2D, 1, texFmt.internalFormat, textureSize.x, textureSize.y);
    else if constexpr(TT == TextureBase::Texture3D)
        glTexStorage3D(GL_TEXTURE_3D, 1, texFmt.internalFormat, textureSize.x, textureSize.y, textureSize.z);
    
    isAllocated = true;
}

template <TextureBase::TextureType TT>
void Texture<TT, TextureBase::Immutable>::fill(const void * buffer)
{
    if(!this->isGenerated())
        return;
    if(!this->isFormatSet())
        return;

    auto texFmt = this->getTextureFormat();

    if constexpr(TT == TextureBase::Texture1D)
        glTexSubImage1D(GL_TEXTURE_1D, 0, 0, textureSize.x, texFmt.pixelFormat, texFmt.pixelType, buffer);
    else if constexpr(TT == TextureBase::Texture2D)
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, textureSize.x, textureSize.y, texFmt.pixelFormat, texFmt.pixelType, buffer);
    else if constexpr(TT == TextureBase::Texture3D)
        glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, textureSize.x, textureSize.y, textureSize.z, texFmt.pixelFormat, texFmt.pixelType, buffer);
}

#endif //LVRC_TEXTURE_HPP