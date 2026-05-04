#ifndef GLUTIL_MATH_HPP
#define GLUTIL_MATH_HPP

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace glutil {

using vec2 = glm::vec2;
using vec3 = glm::vec3;
using mat4 = glm::mat4;

// TODO : add padding?
struct VertexP {
    float x, y, z;
};

struct VertexPC {
    float x, y, z;
    float r, g, b;
};

struct VertexPNT {
    float x, y, z;
    float nx, ny, nz;
    float u, v;
};

struct VertexPNCT {
    float x, y, z;
    float nx, ny, nz;
    float r, g, b;
    float u, v;
};

inline constexpr vec3 position(const VertexP& vertex) noexcept { return vec3{vertex.x, vertex.y, vertex.z}; }
inline constexpr vec3 position(const VertexPC& vertex) noexcept { return vec3{vertex.x, vertex.y, vertex.z}; }
inline constexpr vec3 position(const VertexPNT& vertex) noexcept { return vec3{vertex.x, vertex.y, vertex.z}; }
inline constexpr vec3 position(const VertexPNCT& vertex) noexcept { return vec3{vertex.x, vertex.y, vertex.z}; }

inline constexpr vec3 color(const VertexPC& vertex) noexcept { return vec3{vertex.r, vertex.g, vertex.b}; }
inline constexpr vec3 color(const VertexPNCT& vertex) noexcept { return vec3{vertex.r, vertex.g, vertex.b}; }

inline constexpr vec3 normal(const VertexPNT& vertex) noexcept { return vec3{vertex.nx, vertex.ny, vertex.nz}; }
inline constexpr vec3 normal(const VertexPNCT& vertex) noexcept { return vec3{vertex.nx, vertex.ny, vertex.nz}; }

inline constexpr vec2 uv(const VertexPNT& vertex) noexcept { return vec2{vertex.u, vertex.v}; }
inline constexpr vec2 uv(const VertexPNCT& vertex) noexcept { return vec2{vertex.u, vertex.v}; }

} // namespace glutil

#endif // GLUTIL_MATH_HPP