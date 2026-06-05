#ifndef GLUTIL_MATH_HPP
#define GLUTIL_MATH_HPP

#include <type_traits>
#include <iterator>
#include <vector>

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace glutil {

using vec2 = glm::vec2;
using vec3 = glm::vec3;
using mat4 = glm::mat4;

#define GLUTIL_VALIDATE_VERTEX(T, FLOAT_COUNT) \
    static_assert(std::is_standard_layout<T>::value); \
    static_assert(std::is_trivially_copyable<T>::value); \
    static_assert(sizeof(T) == sizeof(float) * FLOAT_COUNT)

struct VertexP {
    float x, y, z;
};
GLUTIL_VALIDATE_VERTEX(VertexP, 3);

struct VertexPC {
    float x, y, z;
    float r, g, b;
};
GLUTIL_VALIDATE_VERTEX(VertexPC, 6);

struct VertexPT {
    float x, y, z;
    float u, v;
};
GLUTIL_VALIDATE_VERTEX(VertexPT, 5);

struct VertexPNT {
    float x, y, z;
    float nx, ny, nz;
    float u, v;
};
GLUTIL_VALIDATE_VERTEX(VertexPNT, 8);

struct VertexPNCT {
    float x, y, z;
    float nx, ny, nz;
    float r, g, b;
    float u, v;
};
GLUTIL_VALIDATE_VERTEX(VertexPNCT, 11);

#undef GLUTIL_VALIDATE_VERTEX

inline constexpr bool operator==(const VertexP& a, const VertexP& b) noexcept {
    return a.x == b.x && a.y == b.y && a.z == b.z;
}

inline constexpr bool operator==(const VertexPC& a, const VertexPC& b) noexcept {
    return a.x == b.x && a.y == b.y && a.z == b.z &&
           a.r == b.r && a.g == b.g && a.b == b.b;
}

inline constexpr bool operator==(const VertexPT& a, const VertexPT& b) noexcept {
    return a.x == b.x && a.y == b.y && a.z == b.z &&
           a.u == b.u && a.v == b.v;
}

inline constexpr bool operator==(const VertexPNCT& a, const VertexPNCT& b) noexcept {
    return a.x == b.x && a.y == b.y && a.z == b.z &&
           a.nx == b.nx && a.ny == b.ny && a.nz == b.nz &&
           a.r == b.r && a.g == b.g && a.b == b.b &&
           a.u == b.u && a.v == b.v;
}

inline constexpr bool operator==(const VertexPNT& a, const VertexPNT& b) noexcept {
    return a.x == b.x && a.y == b.y && a.z == b.z &&
           a.nx == b.nx && a.ny == b.ny && a.nz == b.nz &&
           a.u == b.u && a.v == b.v;
}

inline constexpr vec3 position(const VertexP& vertex) noexcept { return vec3{vertex.x, vertex.y, vertex.z}; }
inline constexpr vec3 position(const VertexPC& vertex) noexcept { return vec3{vertex.x, vertex.y, vertex.z}; }
inline constexpr vec3 position(const VertexPT& vertex) noexcept { return vec3{vertex.x, vertex.y, vertex.z}; }
inline constexpr vec3 position(const VertexPNT& vertex) noexcept { return vec3{vertex.x, vertex.y, vertex.z}; }
inline constexpr vec3 position(const VertexPNCT& vertex) noexcept { return vec3{vertex.x, vertex.y, vertex.z}; }

inline constexpr vec3 color(const VertexPC& vertex) noexcept { return vec3{vertex.r, vertex.g, vertex.b}; }
inline constexpr vec3 color(const VertexPNCT& vertex) noexcept { return vec3{vertex.r, vertex.g, vertex.b}; }

inline constexpr vec3 normal(const VertexPNT& vertex) noexcept { return vec3{vertex.nx, vertex.ny, vertex.nz}; }
inline constexpr vec3 normal(const VertexPNCT& vertex) noexcept { return vec3{vertex.nx, vertex.ny, vertex.nz}; }

inline constexpr vec2 uv(const VertexPT& vertex) noexcept { return vec2{vertex.u, vertex.v}; }
inline constexpr vec2 uv(const VertexPNT& vertex) noexcept { return vec2{vertex.u, vertex.v}; }
inline constexpr vec2 uv(const VertexPNCT& vertex) noexcept { return vec2{vertex.u, vertex.v}; }


template <typename InputIt>
inline auto positions(InputIt first, InputIt last) {
    std::vector<vec3> result;
    result.reserve(std::distance(first, last));

    for (; first != last; ++first)
        result.push_back(position(*first));

    return result;
}

template <typename InputIt>
inline auto colors(InputIt first, InputIt last) {
    std::vector<vec3> result;
    result.reserve(std::distance(first, last));

    for (; first != last; ++first)
        result.push_back(color(*first));

    return result;
}

template <typename InputIt>
inline auto normals(InputIt first, InputIt last) {
    std::vector<vec3> result;
    result.reserve(std::distance(first, last));

    for (; first != last; ++first)
        result.push_back(normal(*first));

    return result;
}

template <typename InputIt>
inline auto uvs(InputIt first, InputIt last) {
    std::vector<vec2> result;
    result.reserve(std::distance(first, last));

    for (; first != last; ++first)
        result.push_back(uv(*first));

    return result;
}
} // namespace glutil

#endif // GLUTIL_MATH_HPP