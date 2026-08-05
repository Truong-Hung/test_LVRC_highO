#ifndef LVRC_SHADERSTORAGEBUFFEROBJECT_HPP
#define LVRC_SHADERSTORAGEBUFFEROBJECT_HPP

#include <iostream>
#include <cstring>
#include <stdexcept>
#include <glad/glad.h>
#include <renderer/opengl/utils/OpenGL.hpp>

/*!
 * Class that can handle OpenGL's SSBO.
 */
class ShaderStorageBufferObject {
public:
    /*!
     * Constructor that take the SSBO binding point as a parameter.
     * \param binding The SSBO binding point.
     * \sa getBinding
     */
    explicit ShaderStorageBufferObject(GLuint binding);

    /*!
     * Destructor.
     */
    ~ShaderStorageBufferObject();

    /*!
     * Get the binding point of the SSBO.
     * \return The binding point.
     * \sa ShaderStorageBufferObject
     */
    [[nodiscard]] GLuint getBinding() const;

    /*!
     * Get the name of the SSBO's OpenGL buffer.
     * \return The SSBO's buffer name.
     */
    [[nodiscard]] GLuint getName() const;

    /*!
     * Allocate the SSBO memory. If the memory have already been allocated, it
     * will be reallocated with the new size and all the previous data will
     * lost.
     * \param size The size in Bytes to allocate.
     * \sa isAllocated
     */
    void allocate(size_t size);

    /*!
     * Get the size allocated to this SSBO's OpenGL buffer.
     * \return The SSBO's allocated size.
     */
    [[nodiscard]] size_t getAllocatedSize() const;

    /*!
     * Check if the SSBO have been allocated (if \ref allocate has been called).
     * \return True if it has been allocated. False otherwise.
     */
    [[nodiscard]] bool isAllocated() const;

    /*!
     * Start using the SSBO. This function should be called after the SSBO has
     * been allocated and before any call to \ref write.
     * \sa release, write, write(const void*, size_t)
     */
    void use();

    /*!
     * Release the SSBO. It should be called after the use function, and before
     * any call to use of any other SSBO instance.
     * \sa use, isUsed
     */
    void release();

    /*!
     * Check if the SSBO us currently used
     * \return
     * \sa use, release
     */
    [[nodiscard]] bool isUsed() const;

    /*!
     * Write \p value to the buffer.
     * \tparam T The type of \p value.
     * \param value The value to write.
     */
    template<class T>
    void write(const T& value) {
#ifdef LVRC_DEBUG
        if (!used) {
            std::cerr << "SSBO must be used before you write on it."
                      << std::endl;
            return;
        }
#endif

        memcpy(mappedData, &value, sizeof(T));
        mappedData = reinterpret_cast<T*>(mappedData) + 1;
    }

    /*!
     * Write \p data of size \p size to the buffer. If \p data is nullptr, it
     * skip \p size bytes in the buffer, and the skipped data are undefined.
     * \param data A pointer to the data to write.
     * \param size The size of the data to write.
     */
    void write(const void* data, size_t size);

private:
    GLuint binding; /*!< The OpenGL binding point */
    GLuint name; /*!< The buffer OpenGL name */

    bool used; /*!< Store if the SSBO is currently used */
    size_t allocated_size; /*!< Store if the SSBO has been allocated (!= 0), and if true its size */

    void* mappedData; /*!< The mapped data, nullptr if \ref use has not been
                       * called */
};

#endif //LVRC_SHADERSTORAGEBUFFEROBJECT_HPP
