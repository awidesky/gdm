#ifndef GLUTIL_MATH_HPP
#define GLUTIL_MATH_HPP

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace glutil {

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

} // namespace glutil

#endif // GLUTIL_MATH_HPP