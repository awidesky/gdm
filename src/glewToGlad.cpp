// gdm: glew-glad bridge
// - Provides GLEW-facing init that also loads GLAD (debug wrapper version)
// - User code calls glewInit(), but shim header maps that to gdm_glew_glad_glewInit()

#ifdef GDM_HAS_GLEW_GLAD

#include <GL/glew.h>  // should resolve to real GLEW via PRIVATE include dirs for this target
#include <glad/gl.h>

#ifdef GDM_HAS_GLFW
#  include <GLFW/glfw3.h>
#endif

#ifdef GDM_HAS_FREEGLUT
#  include <GL/freeglut.h>
#endif

extern "C" GLenum GLEWAPIENTRY gdm_glew_glad_glewInit(void) {
// If the shim header was accidentally included first, avoid recursion.
#ifdef glewInit
#  undef glewInit
#endif

    GLenum glew_err = glewInit();
    if (glew_err != GLEW_OK)
        return glew_err;

    int glad_ok = 0;

#if defined(GDM_HAS_GLFW)
    glad_ok = gladLoadGL(reinterpret_cast<GLADloadfunc>(glfwGetProcAddress));
#elif defined(GDM_HAS_FREEGLUT)
    glad_ok = gladLoadGL(reinterpret_cast<GLADloadfunc>(glutGetProcAddress));
#else
#  error "glew-glad requires GDM_WINDOW_BACKEND=glfw or freeglut."
#endif

    if (!glad_ok)
        return GLEW_ERROR_NO_GL_VERSION;

    return GLEW_OK;
}

#endif // GDM_HAS_GLEW_GLAD