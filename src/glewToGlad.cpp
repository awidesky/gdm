// gdm: glew-glad bridge
// - Provides GLEW-facing init that also loads GLAD (debug wrapper version)
// - User code calls glewInit(), but shim header maps that to gdm_glew_glad_glewInit()

#ifdef GDM_HAS_GLEW_GLAD

#include <GL/glew.h>

#ifdef GDM_HAS_GLFW
#  include <GLFW/glfw3.h>
#endif

#ifdef GDM_HAS_FREEGLUT
#  include <GL/freeglut.h>
#endif

#include <stdio.h>

// If the glew-glad target linked glutil, that would close a circular link:
// glutil -> gdm::gdm -> gdm::loader -> gdm_glew_glad -> glutil,
// So glew-glad doesn't link glutil; it only names the symbol it needs in a debug build,
// and the consumer must links glutil if glutil debug utilities are needed.
#if defined(GDM_HAS_GLUTIL) && GDM_DEBUG
// the real definition lives in glutil's debug.cpp
namespace glutil { namespace debug { void init(); } }
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
    glad_ok = gladLoadGL((GLADloadfunc)glfwGetProcAddress);
#elif defined(GDM_HAS_FREEGLUT)
    glad_ok = gladLoadGL((GLADloadfunc)glutGetProcAddress);
#else
#  error "glew-glad requires GDM_WINDOW_BACKEND=glfw or freeglut."
#endif

    if (!glad_ok) {
        printf("GLEW loading is successful, but gladLoadGL failed!");
        return GLEW_ERROR_NO_GL_VERSION;
    }

#if defined(GDM_HAS_GLUTIL) && GDM_DEBUG
    // Start glutil's debug engine (callbacks, error tracking, auto-labelling)
    // for this context. Only reachable when the consumer also links glutil.
    glutil::debug::init();
#endif

    return GLEW_OK;
}

#endif // GDM_HAS_GLEW_GLAD