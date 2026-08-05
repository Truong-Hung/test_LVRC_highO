#ifndef LVRC_MATH_HPP
#define LVRC_MATH_HPP

#include <algorithm>
#include <sstream>
#include <cmath>

/*!
 * Linear interpolation between two points.
 * \param a Start value.
 * \param b And value.
 * \param t Interpolation value.
 * \return The interpolation between a and b. Equal to a + (b - a) * t.
 * \sa invLerp
 */
inline float lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

/*!
 * Inverse of the lerp function. Return t with v = lerp(a, b, t).
 * \param a Start value
 * \param b End value
 * \param v Value between a and b
 * \return Ration between a and b
 */
inline float invLerp(float a, float b, float v) {
    return (v - a) / (b - a);
}

/*!
 * Change size in bytes to a formatted string. For example: 1024 -> 1KiB.
 * \param size The size in bytes to format.
 * \param precision The precision of the formatted number.
 * \return The formatted number.
 */
inline std::string formatSize(int size, unsigned int precision = 2) {
    if (size == 0) {
        return "0B";
    }

    constexpr int k = 1024;
    static const std::string sizes[] = {"B", "KiB", "MiB", "GiB", "TiB", "PiB"};

    const int i = static_cast<int>(std::floor(std::log(size) / std::log(k)));

    std::ostringstream out;
    out.precision(precision);
    out << std::fixed << size / std::pow(k, i)
        << sizes[i];

    return out.str();
}

#endif //LVRC_MATH_HPP
