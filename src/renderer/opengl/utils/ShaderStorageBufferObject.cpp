#include "renderer/opengl/utils/ShaderStorageBufferObject.hpp"


ShaderStorageBufferObject::ShaderStorageBufferObject(GLuint binding) :
        binding(binding),
        name(0),
        used(false),
        allocated_size(0),
        mappedData(nullptr) {
    glGenBuffers(1, &name);
    if (name == 0) {
        throw std::runtime_error(
            "Unable to create ShaderStorageBufferObject: " + glGetErrorsString()
        );
    }

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding, name);
}

ShaderStorageBufferObject::~ShaderStorageBufferObject() {
    if (used) {
        release();
    }
    glDeleteBuffers(1, &name);
}

GLuint ShaderStorageBufferObject::getBinding() const {
    return binding;
}

GLuint ShaderStorageBufferObject::getName() const {
    return name;
}

size_t ShaderStorageBufferObject::getAllocatedSize() const {
    return allocated_size;
}

void ShaderStorageBufferObject::allocate(size_t size) {
    if (size == allocated_size)
        return;

    if (size == 0)
	    throw std::runtime_error("Cannot allocate zero-sized SSBO");

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, name);
    glBufferData(GL_SHADER_STORAGE_BUFFER, size, nullptr, GL_STATIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    allocated_size = size;
}

bool ShaderStorageBufferObject::isAllocated() const {
    return allocated_size > 0;
}

void ShaderStorageBufferObject::use() {
    if (allocated_size == 0) {
        std::cerr << "SSBO must be allocated before used." << std::endl;
        return;
    }

    if (used) {
        std::cerr << "SSBO is already used." << std::endl;
        return;
    }

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, name);
    mappedData = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_WRITE);
    if (!mappedData) {
        const auto errors = glGetErrorsString();
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
        throw std::runtime_error(
            "Unable to map data for SSBO '"
            + std::to_string(name)
            + "' with binding '" + std::to_string(binding)
            + "': " + errors
        );
    }

    used = true;
}

void ShaderStorageBufferObject::release() {
#ifdef LVRC_DEBUG
    if (!used) {
        std::cerr << "Trying to release an unused SSBO." << std::endl;
        return;
    }
#endif

    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    mappedData = nullptr;
    used = false;
}

bool ShaderStorageBufferObject::isUsed() const {
    return used;
}

void ShaderStorageBufferObject::write(const void* data, size_t size) {
#ifdef LVRC_DEBUG
    if (!used) {
        std::cerr << "SSBO must be used before you write on it."
                  << std::endl;
        return;
    }
#endif

    if (data) {
        memcpy(mappedData, data, size);
    }
    mappedData = reinterpret_cast<char*>(mappedData) + size;
}
