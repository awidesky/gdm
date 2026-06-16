#ifndef GLUTIL_MATH_HPP
#define GLUTIL_MATH_HPP

#include <iterator>
#include <type_traits>
#include <vector>

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace glutil {

using vec2 = glm::vec2;
using vec3 = glm::vec3;
using mat4 = glm::mat4;

/**
 * Compile-time validation macro for vertex layout types.
 *
 * Ensures:
 * - Standard layout type (C-compatible memory layout)
 * - Trivially copyable (safe for GPU upload / memcpy)
 * - Exact float-size matching for attribute stride validation
 */
#define GLUTIL_VALIDATE_VERTEX(T, FLOAT_COUNT) \
    static_assert(std::is_standard_layout<T>::value); \
    static_assert(std::is_trivially_copyable<T>::value); \
    static_assert(sizeof(T) == sizeof(float) * FLOAT_COUNT)

/**
 * Position-only vertex (x, y, z).
 * Intended for simple geometry without attributes.
 */
struct VertexP {
    float x, y, z;
};
GLUTIL_VALIDATE_VERTEX(VertexP, 3);

/** Position + Color vertex (RGB). */
struct VertexPC {
    float x, y, z;
    float r, g, b;
};
GLUTIL_VALIDATE_VERTEX(VertexPC, 6);

/** Position + Texture coordinate vertex (UV). */
struct VertexPT {
    float x, y, z;
    float u, v;
};
GLUTIL_VALIDATE_VERTEX(VertexPT, 5);

/** Position + Normal + Texture coordinate vertex. */
struct VertexPNT {
    float x, y, z;
    float nx, ny, nz;
    float u, v;
};
GLUTIL_VALIDATE_VERTEX(VertexPNT, 8);

/** Position + Normal + Color + Texture coordinate vertex. */
struct VertexPNCT {
    float x, y, z;
    float nx, ny, nz;
    float r, g, b;
    float u, v;
};
GLUTIL_VALIDATE_VERTEX(VertexPNCT, 11);

#undef GLUTIL_VALIDATE_VERTEX

/** Equality comparison for position-only vertex. */
inline constexpr bool operator==(const VertexP& a, const VertexP& b) noexcept {
    return a.x == b.x && a.y == b.y && a.z == b.z;
}

/** Equality comparison for position-color vertex. */
inline constexpr bool operator==(const VertexPC& a, const VertexPC& b) noexcept {
    return a.x == b.x && a.y == b.y && a.z == b.z &&
           a.r == b.r && a.g == b.g && a.b == b.b;
}

/** Equality comparison for position-UV vertex. */
inline constexpr bool operator==(const VertexPT& a, const VertexPT& b) noexcept {
    return a.x == b.x && a.y == b.y && a.z == b.z &&
           a.u == b.u && a.v == b.v;
}

/** Equality comparison for position-normal-color-uv vertex. */
inline constexpr bool operator==(const VertexPNCT& a, const VertexPNCT& b) noexcept {
    return a.x == b.x && a.y == b.y && a.z == b.z &&
           a.nx == b.nx && a.ny == b.ny && a.nz == b.nz &&
           a.r == b.r && a.g == b.g && a.b == b.b &&
           a.u == b.u && a.v == b.v;
}

/** Extracts position vector from vertex. */
inline constexpr bool operator==(const VertexPNT& a, const VertexPNT& b) noexcept {
    return a.x == b.x && a.y == b.y && a.z == b.z &&
           a.nx == b.nx && a.ny == b.ny && a.nz == b.nz &&
           a.u == b.u && a.v == b.v;
}

inline constexpr vec3 position(const VertexP& vertex) noexcept { return vec3{vertex.x, vertex.y, vertex.z}; }
/** Base position accessor for colored vertex. */
inline constexpr vec3 position(const VertexPC& vertex) noexcept { return vec3{vertex.x, vertex.y, vertex.z}; }
/** Base position accessor for textured vertex. */
inline constexpr vec3 position(const VertexPT& vertex) noexcept { return vec3{vertex.x, vertex.y, vertex.z}; }
/** Base position accessor for normal/texture vertex. */
inline constexpr vec3 position(const VertexPNT& vertex) noexcept { return vec3{vertex.x, vertex.y, vertex.z}; }
/** Base position accessor for full vertex layout. */
inline constexpr vec3 position(const VertexPNCT& vertex) noexcept { return vec3{vertex.x, vertex.y, vertex.z}; }

/** Extracts RGB color from vertex. */
inline constexpr vec3 color(const VertexPC& vertex) noexcept { return vec3{vertex.r, vertex.g, vertex.b}; }
/** Color accessor for full vertex layout. */
inline constexpr vec3 color(const VertexPNCT& vertex) noexcept { return vec3{vertex.r, vertex.g, vertex.b}; }

/** Extracts normal vector from vertex. */
inline constexpr vec3 normal(const VertexPNT& vertex) noexcept { return vec3{vertex.nx, vertex.ny, vertex.nz}; }
/** Normal accessor for full vertex layout. */
inline constexpr vec3 normal(const VertexPNCT& vertex) noexcept { return vec3{vertex.nx, vertex.ny, vertex.nz}; }

/** Extracts UV coordinates from vertex. */
inline constexpr vec2 uv(const VertexPT& vertex) noexcept { return vec2{vertex.u, vertex.v}; }
/** UV accessor for normal-textured vertex. */
inline constexpr vec2 uv(const VertexPNT& vertex) noexcept { return vec2{vertex.u, vertex.v}; }
/** UV accessor for full vertex layout. */
inline constexpr vec2 uv(const VertexPNCT& vertex) noexcept { return vec2{vertex.u, vertex.v}; }

/**
 * Extracts positions from a vertex range.
 *
 * @tparam InputIt Iterator over vertex types supporting position().
 * @return Vector of glm::vec3 positions.
 */
template <typename InputIt>
inline auto positions(InputIt first, InputIt last) {
    std::vector<vec3> result;
    result.reserve(std::distance(first, last));

    for (; first != last; ++first)
        result.push_back(position(*first));

    return result;
}

/** Extracts colors from a vertex range. */
template <typename InputIt>
inline auto colors(InputIt first, InputIt last) {
    std::vector<vec3> result;
    result.reserve(std::distance(first, last));

    for (; first != last; ++first)
        result.push_back(color(*first));

    return result;
}

/** Extracts normals from a vertex range. */
template <typename InputIt> inline auto normals(InputIt first, InputIt last) {
    std::vector<vec3> result;
    result.reserve(std::distance(first, last));

    for (; first != last; ++first)
        result.push_back(normal(*first));

    return result;
}

/** Extracts UV coordinates from a vertex range. */
template <typename InputIt> inline auto uvs(InputIt first, InputIt last) {
    std::vector<vec2> result;
    result.reserve(std::distance(first, last));

    for (; first != last; ++first)
        result.push_back(uv(*first));

    return result;
}
} // namespace glutil

#endif // GLUTIL_MATH_HPP