#ifndef GLUTIL_GL_HPP
#define GLUTIL_GL_HPP

#if defined(GDM_HAS_GLEW)
// In glew-glad mode, <GL/glew.h> is a generated shim that pulls in both
// the GLEW-facing API and GLAD-backed function mapping.
#include <GL/glew.h>
#elif defined(GDM_HAS_GLAD)
#include <glad/gl.h>
    #if GDM_DEBUG
    namespace glutil::debug { void init(); } // debug function declaration
    // Helper functions hijacks the glad loading function, and initialize glutil::debug if leading was successful.
    // So that user doesn't have to call it manually, making it easier to embed glutil debug functionality into existing lagacy codebase.

    /* This function acts if it's gladLoadGL, run real gladLoadGL, and init glutil debug engine */
    static inline int glutil_gladLoadGL(GLADloadfunc load) {
        const int ret = gladLoadGL(load);
        if(ret) glutil::debug::init();
        return ret;
    }
    /* This function acts if it's gladLoadGLUserPtr, run real gladLoadGLUserPtr, and init glutil debug engine */
    static inline int glutil_gladLoadGLUserPtr(GLADuserptrloadfunc load, void* userptr) {
        const int ret = gladLoadGLUserPtr(load, userptr);
        if(ret) glutil::debug::init();
        return ret;
    }

    #define gladLoadGL glutil_gladLoadGL
    #define gladLoadGLUserPtr glutil_gladLoadGLUserPtr

        #ifdef GLAD_OPTION_GL_LOADER
        /* This function acts if it's gladLoaderLoadGL, run real gladLoaderLoadGL, and init glutil debug engine */
        static inline int glutil_gladLoaderLoadGL(void) {
            const int ret = gladLoaderLoadGL();
            if (ret) glutil::debug::init();
            return ret;
        }

        #define gladLoaderLoadGL glutil_gladLoaderLoadGL
        #endif // GLAD_OPTION_GL_LOADER
    #endif // GDM_DEBUG
#endif


#ifdef GDM_HAS_GLFW
#include <GLFW/glfw3.h>
#elif defined(GDM_HAS_FREEGLUT)
#include <GL/freeglut.h>
#endif

#endif // GLUTIL_GL_HPP