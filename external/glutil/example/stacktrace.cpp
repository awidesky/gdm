#include "config.hpp"
#include <glutil/glutil.hpp>
#include <iostream>

void static framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    (void)window;
    glViewport(0, 0, width, height);
}

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

void static postCallback(void* ret, const char* name, GLADapiproc apiproc, int len_args, ...) {
    (void)ret; (void)apiproc; (void)len_args;
    GLenum err = glad_glGetError();
    if (err == GL_NO_ERROR)
        return;

    std::cerr << "\n[GL ERROR] from custom postcallback : " << name << " (0x" << std::hex << err << std::dec << ")\n";

    glutil::debug::printStackTrace();
}
int main() {

    glfwInit();
    auto version = glutil::debug::availableGLversion();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, version.major);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, version.minor);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "GLUtil_stacktrace_test", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    if (!gladLoadGL(glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // normal texture binding
    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    // erroneous texture binding
    glBindTexture(GL_TEXTURE_2D, 99999); // should generate an error

// check if GLAD debug callback is enabled by GDM_DEBUG macro.
// 1 if enabled(build type is Debug or RelWithDebInfo), 0 if not.
#if !GDM_DEBUG
    // GLAD debug callback is not enabled, call the custom postCallback manually.
    postCallback(NULL, "glBindTexture", (GLADapiproc)glad_glBindTexture, 2, GL_TEXTURE_2D, 99999);
#endif

    while (!glfwWindowShouldClose(window)) {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);

        //inside of rendering loop; test aggregated GL error logging.
        glBindVertexArray(0);
        glDrawArrays(GL_TRIANGLES, 0, 3); 

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
