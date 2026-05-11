#ifndef GLUTIL_GL_HPP
#define GLUTIL_GL_HPP

#if defined(GDM_HAS_GLEW)
// In glew-glad mode, <GL/glew.h> is a generated shim that pulls in both
// the GLEW-facing API and GLAD-backed function mapping.
#include <GL/glew.h>
#elif defined(GDM_HAS_GLAD)
#include <glad/gl.h>
#endif


#ifdef GDM_HAS_GLFW
#include <GLFW/glfw3.h>
#elif defined(GDM_HAS_FREEGLUT)
#include <GL/freeglut.h>
#endif

#endif // GLUTIL_GL_HPP