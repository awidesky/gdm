#ifdef GDM_HAS_GLEW
#include <GL/glew.h>
#endif

#ifdef GDM_HAS_GLAD
#include <glad/gl.h>
#endif

#ifdef GDM_HAS_GLFW
#include <GLFW/glfw3.h>
#elif defined(GDM_HAS_FREEGLUT)
#include <GL/freeglut.h>
#endif

#ifdef GDM_HAS_GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#else
#error "GLM is required for examples/02_manyCubes.cpp"
#endif

#include <array>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <cmath>
#include <random>
#include <string>
#include <vector>
#include <algorithm>

#ifdef GDM_HAS_FREEGLUT
bool g_shouldClose = false;
void onFreeglutKeyboard(unsigned char key, int, int) {
    if (key == 27) g_shouldClose = true;
}
#endif

static const float kCubeVertices[] = {
    // position         // color
    // front
    -0.5f,-0.5f, 0.5f,  0,1,0,  0.5f,-0.5f, 0.5f, 0,1,0,  0.5f, 0.5f, 0.5f, 1,1,1,
    -0.5f,-0.5f, 0.5f,  0,1,0,  0.5f, 0.5f, 0.5f, 1,1,1, -0.5f, 0.5f, 0.5f, 1,1,0,
    // back
    -0.5f,-0.5f,-0.5f,  0,0,0, -0.5f, 0.5f,-0.5f, 1,0,0,  0.5f, 0.5f,-0.5f, 1,0,1,
    -0.5f,-0.5f,-0.5f,  0,0,0,  0.5f, 0.5f,-0.5f, 1,0,1,  0.5f,-0.5f,-0.5f, 0,0,1,
    // left
    -0.5f,-0.5f,-0.5f,  0,0,0, -0.5f,-0.5f, 0.5f, 0,1,0, -0.5f, 0.5f, 0.5f, 1,1,0,
    -0.5f,-0.5f,-0.5f,  0,0,0, -0.5f, 0.5f, 0.5f, 1,1,0, -0.5f, 0.5f,-0.5f, 1,0,0,
    // right
     0.5f,-0.5f,-0.5f,  0,0,1,  0.5f, 0.5f,-0.5f, 1,0,1,  0.5f, 0.5f, 0.5f, 1,1,1,
     0.5f,-0.5f,-0.5f,  0,0,1,  0.5f, 0.5f, 0.5f, 1,1,1,  0.5f,-0.5f, 0.5f, 0,1,0,
    // top
    -0.5f, 0.5f,-0.5f,  1,0,0, -0.5f, 0.5f, 0.5f, 1,1,0,  0.5f, 0.5f, 0.5f, 1,1,1,
    -0.5f, 0.5f,-0.5f,  1,0,0,  0.5f, 0.5f, 0.5f, 1,1,1,  0.5f, 0.5f,-0.5f, 1,0,1,
    // bottom
    -0.5f,-0.5f,-0.5f,  0,0,0,  0.5f,-0.5f,-0.5f, 0,0,1,  0.5f,-0.5f, 0.5f, 0,1,0,
    -0.5f,-0.5f,-0.5f,  0,0,0,  0.5f,-0.5f, 0.5f, 0,1,0, -0.5f,-0.5f, 0.5f, 0,1,0,
};

GLuint compileShader(GLenum type, const char* source) {
    const GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint success = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (success == GL_TRUE) return shader;

    GLint length = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
    std::string log(static_cast<size_t>(length), '\0');
    glGetShaderInfoLog(shader, length, nullptr, log.data());
    std::cerr << "Shader compile failed:\n" << log << std::endl;
    glDeleteShader(shader);
    return 0;
}

GLuint createProgram(const char* vs_src, const char* fs_src) {
    const GLuint vs = compileShader(GL_VERTEX_SHADER, vs_src);
    const GLuint fs = compileShader(GL_FRAGMENT_SHADER, fs_src);
    if (vs == 0 || fs == 0) {
        if (vs) glDeleteShader(vs);
        if (fs) glDeleteShader(fs);
        return 0;
    }

    const GLuint program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);

    glDeleteShader(vs);
    glDeleteShader(fs);

    GLint success = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (success == GL_TRUE) return program;

    GLint length = 0;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
    std::string log(static_cast<size_t>(length), '\0');
    glGetProgramInfoLog(program, length, nullptr, log.data());
    std::cerr << "Program link failed:\n" << log << std::endl;
    glDeleteProgram(program);
    return 0;
}

void printRuntimeInfo() {
    std::cout << "=== Runtime Dependency Info ===\n";

#ifdef GDM_HAS_GLFW
    int major = 0, minor = 0, rev = 0;
    glfwGetVersion(&major, &minor, &rev);
    std::cout << "[GLFW] version: " << major << "." << minor << "." << rev
              << ", string: " << glfwGetVersionString() << "\n";
#endif

#ifdef GDM_HAS_FREEGLUT
    const int glutVersion = glutGet(GLUT_VERSION);
    std::cout << "[FreeGLUT] GLUT_VERSION(raw): " << glutVersion << "\n";
#endif

#ifdef GDM_HAS_GLAD
    std::cout << "[GLAD] Generator version : " << GLAD_GENERATOR_VERSION << ", Debug option "
    #ifdef GLAD_OPTION_GL_DEBUG
        << "ON"
    #else
        << "OFF"
    #endif
    << "\n";
#endif

#ifdef GDM_HAS_GLM
    std::cout << "[GLM]  version: "
              << GLM_VERSION_MAJOR << "."
              << GLM_VERSION_MINOR << "."
              << GLM_VERSION_PATCH << "."
              << GLM_VERSION_REVISION << "\n";
#endif

    std::cout << "\n";

    const char* version = reinterpret_cast<const char*>(glGetString(GL_VERSION));
    const char* slVersion = reinterpret_cast<const char*>(glGetString(GL_SHADING_LANGUAGE_VERSION));
    const char* vendor = reinterpret_cast<const char*>(glGetString(GL_VENDOR));
    const char* renderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));

    std::cout << "[OpenGL] version: " << (version ? version : "(null)") << "\n";
    std::cout << "[OpenGL] GLSL: " << (slVersion ? slVersion : "(null)") << "\n";
    std::cout << "[OpenGL] vendor: " << (vendor ? vendor : "(null)")
              << ", renderer: " << (renderer ? renderer : "(null)") << "\n";
    std::cout << "===============================\n";
}

#ifdef GDM_HAS_GLAD
#ifdef GLAD_OPTION_GL_DEBUG
#include <sstream>
std::string glErrorToString(GLenum err) {
    std::stringstream ss;
    switch (err) {
        case GL_NO_ERROR: ss << "GL_NO_ERROR"; break;
        case GL_INVALID_ENUM: ss << "GL_INVALID_ENUM"; break;
        case GL_INVALID_VALUE: ss << "GL_INVALID_VALUE"; break;
        case GL_INVALID_OPERATION: ss << "GL_INVALID_OPERATION"; break;
        case GL_OUT_OF_MEMORY: ss << "GL_OUT_OF_MEMORY"; break;
        case GL_INVALID_FRAMEBUFFER_OPERATION: ss << "GL_INVALID_FRAMEBUFFER_OPERATION"; break;
            // Framebuffer state
        case GL_FRAMEBUFFER_COMPLETE: return "GL_FRAMEBUFFER_COMPLETE";
        case GL_FRAMEBUFFER_UNDEFINED: return "GL_FRAMEBUFFER_UNDEFINED";
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT: return "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT";
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT: return "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT";
        case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER: return "GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER";
        case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER: return "GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER";
        case GL_FRAMEBUFFER_UNSUPPORTED: return "GL_FRAMEBUFFER_UNSUPPORTED";
        case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE: return "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE";
        case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS: return "GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS";
        default: ss << "UNKNOWN_ERROR";
    }
    ss << '(' << err << ')';
    return ss.str();
}
static void gl_error_callback(void *ret, const char *name, GLADapiproc apiproc, int len_args, ...) {
    GLenum error = glad_glGetError();
    if (error != GL_NO_ERROR) {
        std::cerr << "[GL Error] " << glErrorToString(error) << " in function " << name << std::endl;
    }
}
#endif
#endif

bool initializeLoader() {
#ifdef GDM_HAS_GLAD
    #ifdef GDM_HAS_GLFW
    int gladVersion = gladLoadGL(glfwGetProcAddress);
    #elif defined(GDM_HAS_FREEGLUT)
    int gladVersion = gladLoadGL(reinterpret_cast<GLADloadfunc>(glutGetProcAddress));
    #endif
    if (gladVersion == 0) {
        std::cerr << "Failed to initialize GLAD context." << std::endl;
        return false;
    }
    #ifdef GLAD_OPTION_GL_DEBUG
    gladSetGLPostCallback(gl_error_callback);
    std::cout << "GL error checking enabled via GLAD post-callback" << std::endl;
    #endif
    return true;
#elif defined(GDM_HAS_GLEW)
    glewExperimental = GL_TRUE;
    const GLenum glewErr = glewInit();
    if (glewErr != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW: " << glewGetErrorString(glewErr) << std::endl;
        return false;
    }
    return true;
#else
    std::cerr << "No loader selected. (expected GLAD or GLEW)" << std::endl;
    return false;
#endif
}

static float randf(float a, float b) { return a + (b - a) * (float(rand()) / float(RAND_MAX)); }

std::vector<glm::vec3> generateRandomPositions(int num_cubes, const glm::vec3& eye, const glm::vec3& center, const glm::vec3& up, float zNear, float zFar, float fovY_deg, float aspect) {
    std::vector<glm::vec3> positions;
    positions.reserve(num_cubes);
    float tanHalfFov = tanf(fovY_deg * 0.5f * 3.14159265f / 180.0f);

    const glm::vec3 forward = glm::normalize(center - eye);
    const glm::vec3 right = glm::normalize(glm::cross(forward, up));
    const glm::vec3 camUp = glm::cross(right, forward);

    for (int i = 0; i < num_cubes; ++i) {
        float minDepth = zNear + 5.0f;
        float d = randf(minDepth, zFar - 1.0f);
        float h = 2.0f * d * tanHalfFov;
        float w = h * aspect;

        float rx = sqrtf(sqrtf(randf(0.0f, 1.0f)));
        float ry = sqrtf(sqrtf(randf(0.0f, 1.0f)));

        float x = randf(-1.0f, 1.0f) * (w * 0.5f) * rx;
        float y = randf(-1.0f, 1.0f) * (h * 0.5f) * ry;

        positions.push_back(eye + forward * d + right * x + camUp * y);
    }
    return positions;
}

#ifdef GDM_HAS_GLFW
void updateFramebufferDependentState(GLFWwindow* window, float fovY, float zNear, float zFar, GLint projLoc) {
    int fbW = 0;
    int fbH = 0;
    glfwGetFramebufferSize(window, &fbW, &fbH);
    if (fbW <= 0 || fbH <= 0) {
        return;
    }

    glViewport(0, 0, fbW, fbH);

    const float aspect = static_cast<float>(fbW) / static_cast<float>(fbH);
    const glm::mat4 proj = glm::perspective(glm::radians(fovY), aspect, zNear, zFar);
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(proj));
}
#endif

int main(int argc, char** argv) {
    int num_cubes = 300;
    if (argc == 2) num_cubes = std::stoi(argv[1]);

#ifdef GDM_HAS_GLFW
    if (!glfwInit()) { std::cerr << "Failed to initialize GLFW" << std::endl; return 1; }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    #ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    #endif

    GLFWwindow* window = glfwCreateWindow(1280, 720, "GDM Example 02 - Many Cubes", nullptr, nullptr);
    if (!window) { std::cerr << "Failed to create GLFW window" << std::endl; glfwTerminate(); return 1; }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
#elif defined(GDM_HAS_FREEGLUT)
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutInitWindowSize(1280, 720);
    glutInitContextVersion(3, 3);
    #ifdef GLUT_CORE_PROFILE
    glutInitContextProfile(GLUT_CORE_PROFILE);
    #endif
    int win = glutCreateWindow("GDM Example 02 - Many Cubes");
    if (win <= 0) { std::cerr << "Failed to create FreeGLUT window" << std::endl; return 1; }
    glutKeyboardFunc(onFreeglutKeyboard);
#else
    std::cerr << "No window backend selected. (expected GLFW or FreeGLUT)" << std::endl; return 1;
#endif

    if (!initializeLoader()) {
#ifdef GDM_HAS_GLFW
        glfwDestroyWindow(window);
        glfwTerminate();
#elif defined(GDM_HAS_FREEGLUT)
        glutDestroyWindow(glutGetWindow());
#endif
        return 1;
    }

    printRuntimeInfo();

    std::cout << "Example 02 - Many Cubes\n";

    // GL state
    glEnable(GL_DEPTH_TEST);

    GLuint vao = 0, vbo = 0;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(kCubeVertices), kCubeVertices, GL_STATIC_DRAW);

    // layout: pos (3 floats) then color (3 floats)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), reinterpret_cast<void*>(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    const char* vertexShaderSrc = R"(
        #version 330 core
        layout(location = 0) in vec3 aPos;
        layout(location = 1) in vec3 aColor;

        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 proj;

        out vec3 vColor;
        out vec3 vNormal;
        out vec3 vPosition;

        void main() {
            vColor = aColor;
            // compute normal using inverse-transpose (intentionally heavy for demo)
            mat3 normalMat = transpose(inverse(mat3(model)));
            vNormal = normalize(normalMat * aPos);
            vPosition = vec3(model * vec4(aPos, 1.0));
            gl_Position = proj * view * model * vec4(aPos, 1.0);
        }
    )";

    const char* fragmentShaderSrc = R"(
        #version 330 core
        in vec3 vColor;
        in vec3 vNormal;
        in vec3 vPosition;
        out vec4 FragColor;

        uniform float time;
        uniform vec3 viewPos;
        uniform float lightDensity;

        // configurable heavy work
        const int NUM_LIGHTS = 8;

        vec3 computeLight(in vec3 pos, in vec3 normal) {
            vec3 result = vec3(0.0);
            for (int i = 0; i < NUM_LIGHTS; ++i) {
                // animate light positions in a meaningful way
                float fi = float(i);
                vec3 lightPos = vec3(5.0 * sin(time * (0.2 + 0.05 * fi) + fi),
                                     3.0 * cos(time * (0.12 + 0.03 * fi) - fi),
                                     5.0 * sin(time * (0.15 + 0.04 * fi) + fi * 0.7));

                vec3 L = normalize(lightPos - pos);
                float diff = max(dot(normal, L), 0.0);
                // simple Phong-like specular
                vec3 V = normalize(viewPos - pos);
                vec3 H = normalize(V + L);
                float spec = pow(max(dot(normal, H), 0.0), 32.0);

                // distance attenuation
                float dist = length(lightPos - pos);
                float att = 1.0 / (1.0 + 0.2 * dist + 0.05 * dist * dist);

                vec3 lightCol = vec3(0.25 + 0.35 * sin(fi + time), 0.20 + 0.35 * cos(fi * 0.7 + time * 0.6), 0.18 + 0.30 * sin(fi * 1.3 - time * 0.4));
                result += (diff * vec3(0.28) + spec * vec3(0.08)) * lightCol * att;
            }

            // additional per-pixel procedural work to amplify fragment cost
            float heavy = 0.0;
            for (int k = 1; k <= 16; ++k) {
                heavy += abs(sin(dot(pos, vec3(float(k) * 0.13, float(k) * 0.17, float(k) * 0.19)) + time * 2.0));
                heavy += abs(cos(dot(normal, vec3(float(k) * 0.11, float(k) * 0.07, float(k) * 0.09)) * 1.3 - time * 0.7));
            }
            heavy = heavy / 32.0;

            return result + vec3(heavy * 0.12) * lightDensity;
        }

        void main() {
            vec3 n = normalize(vNormal);
            vec3 base = mix(vColor, vec3(dot(vColor, vec3(0.3333))), 0.1);
            base = base * 0.95 + vec3(0.03, 0.04, 0.05);
            vec3 light = computeLight(vPosition, n);
            // mix base color with computed lighting
            vec3 color = clamp(base * 0.80 + light * lightDensity, 0.0, 1.0);
            // slight darkening to avoid washed-out highlights
            color = pow(color, vec3(1.18));
            FragColor = vec4(color, 1.0);
        }
    )";

    const GLuint program = createProgram(vertexShaderSrc, fragmentShaderSrc);
    if (program == 0) {
        glDeleteBuffers(1, &vbo);
        glDeleteVertexArrays(1, &vao);
#ifdef GDM_HAS_GLFW
        glfwDestroyWindow(window);
        glfwTerminate();
#elif defined(GDM_HAS_FREEGLUT)
        glutDestroyWindow(glutGetWindow());
#endif
        return 1;
    }

    // camera
    glm::vec3 camPos(4.0f, 3.0f, -3.0f);
    glm::vec3 camTarget(0.0f, 0.0f, 0.0f);
    glm::vec3 camUp(0.0f, 1.0f, 0.0f);
    float fovY = 45.0f;
    float zNear = 0.1f;
    float zFar = 100.0f;
    int winW = 1280, winH = 720;
    float aspect = float(winW) / float(winH);

    // generate positions and rotations
    auto positions = generateRandomPositions(num_cubes, camPos, camTarget, camUp, zNear, zFar, fovY, aspect);
    std::vector<glm::vec3> rotations; rotations.reserve(num_cubes);
    std::mt19937 rng(12345);
    std::uniform_real_distribution<float> rotDist(0.0f, 3.14159f);
    for (int i = 0; i < num_cubes; ++i) rotations.emplace_back(rotDist(rng), rotDist(rng), rotDist(rng));

    // per-instance velocities for CPU-side simulation (meaningful work: basic collision/response)
    std::vector<glm::vec3> velocities; velocities.reserve(num_cubes);
    std::uniform_real_distribution<float> velDist(-0.5f, 0.5f);
    for (int i = 0; i < num_cubes; ++i) velocities.emplace_back(velDist(rng), velDist(rng), velDist(rng));

    glUseProgram(program);
    const GLint modelLoc = glGetUniformLocation(program, "model");
    const GLint viewLoc = glGetUniformLocation(program, "view");
    const GLint projLoc = glGetUniformLocation(program, "proj");
    const GLint timeLoc = glGetUniformLocation(program, "time");
    const GLint viewPosLoc = glGetUniformLocation(program, "viewPos");
    const GLint lightDensityLoc = glGetUniformLocation(program, "lightDensity");

    // prepare static view/proj
    glm::mat4 view = glm::lookAt(camPos, camTarget, camUp);
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
#ifdef GDM_HAS_GLFW
    updateFramebufferDependentState(window, fovY, zNear, zFar, projLoc);
#else
    glm::mat4 proj = glm::perspective(glm::radians(fovY), aspect, zNear, zFar);
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(proj));
#endif

    auto lastPrint = std::chrono::steady_clock::now();
    unsigned int frameCount = 0;
    double renderTime = 0.0;
    double cpuTime = 0.0;
    double gpuTime = 0.0;
    auto simLastTime = std::chrono::steady_clock::now();
    auto simStartTime = simLastTime;

#ifdef GDM_HAS_GLFW
    while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS && !glfwWindowShouldClose(window)) {
        glfwPollEvents();
#elif defined(GDM_HAS_FREEGLUT)
    while (!g_shouldClose) {
        glutMainLoopEvent();
#endif
        const auto frameBegin = std::chrono::steady_clock::now();

        // --- CPU-heavy meaningful work: per-frame simulation (velocities + pairwise collision)
        const auto simNow = frameBegin;
        const double dt = std::chrono::duration<double>(simNow - simLastTime).count();
        // protect against huge dt on pause
        const float fdt = std::min(0.05, dt * 1.0);
        simLastTime = simNow;

        // integrate velocities
        for (int i = 0; i < num_cubes; ++i) positions[i] += velocities[i] * static_cast<float>(fdt);

        // naive O(N^2) pairwise collisions (meaningful CPU work; scales badly with num_cubes)
        const float collisionRadius = 1.0f; // cubes treated as radius-1 spheres for collision
        const float minDist = collisionRadius;
        const float minDist2 = minDist * minDist;
        for (int i = 0; i < num_cubes; ++i) {
            for (int j = i + 1; j < num_cubes; ++j) {
                glm::vec3 d = positions[j] - positions[i];
                float dist2 = glm::dot(d, d);
                if (dist2 < minDist2) {
                    float dist = sqrtf(dist2) + 1e-6f;
                    glm::vec3 dir = d / dist;
                    float overlap = 0.5f * (minDist - dist);
                    // positional correction
                    positions[i] -= dir * overlap;
                    positions[j] += dir * overlap;
                    // exchange a bit of velocity to push them apart (simple impulse)
                    const float impulse = 10.0f;
                    velocities[i] -= dir * impulse * static_cast<float>(fdt);
                    velocities[j] += dir * impulse * static_cast<float>(fdt);
                }
            }
        }

        // slight damping to stabilize simulation
        for (int i = 0; i < num_cubes; ++i) velocities[i] *= 0.995f;

        // rotate some cubes (visual)
        for (int i = 0; i < num_cubes; ++i) {
            if (i & 1) rotations[i].y += randf(0.007f, 0.013f);
            else rotations[i].y -= randf(0.007f, 0.013f);
            rotations[i].x += randf(0.012f, 0.018f);
        }

    #ifdef GDM_HAS_GLFW
        updateFramebufferDependentState(window, fovY, zNear, zFar, projLoc);
    #else
        glViewport(0, 0, winW, winH);
    #endif

        const auto cpuEnd = std::chrono::steady_clock::now();
    
        glClearColor(0.0f, 0.0f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glBindVertexArray(vao);
        glUseProgram(program);

        // set per-frame uniforms (time, view position)
        const double timeSec = std::chrono::duration<double>(frameBegin - simStartTime).count();
        if (timeLoc >= 0) glUniform1f(timeLoc, static_cast<float>(timeSec));
        if (viewPosLoc >= 0) glUniform3fv(viewPosLoc, 1, glm::value_ptr(camPos));
        if (lightDensityLoc >= 0) glUniform1f(lightDensityLoc, 2.0f);

        for (int i = 0; i < num_cubes; ++i) {
            glm::mat4 model(1.0f);
            model = glm::translate(model, positions[i]);
            model = glm::rotate(model, rotations[i].x, glm::vec3(1,0,0));
            model = glm::rotate(model, rotations[i].y, glm::vec3(0,1,0));
            model = glm::rotate(model, rotations[i].z, glm::vec3(0,0,1));
            model = glm::scale(model, glm::vec3(1.0f));

            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        const auto renderEnd = std::chrono::steady_clock::now();

#ifdef GDM_HAS_GLFW
        glfwSwapBuffers(window);
#elif defined(GDM_HAS_FREEGLUT)
        glutSwapBuffers();
#endif

        const std::chrono::duration<double> cpuElapsed = cpuEnd - frameBegin;
        const std::chrono::duration<double> gpuElapsed = renderEnd - cpuEnd;
        const std::chrono::duration<double> renderElapsed = cpuElapsed + gpuElapsed;

        renderTime += renderElapsed.count();
        cpuTime += cpuElapsed.count();
        gpuTime += gpuElapsed.count();
        frameCount++;

        const auto printNow = std::chrono::steady_clock::now();
        const std::chrono::duration<double> printElapsed = printNow - lastPrint;
        if (printElapsed.count() >= 5.0) {
            const double frameTimeAvgMs = (printElapsed.count() / static_cast<double>(frameCount)) * 1000.0;
            const double renderTimeAvgMs = (renderTime / static_cast<double>(frameCount)) * 1000.0;
            const double cpuTimeAvgMs = (cpuTime / static_cast<double>(frameCount)) * 1000.0;
            const double gpuTimeAvgMs = (gpuTime / static_cast<double>(frameCount)) * 1000.0;

            std::cout << "[FPS] " << static_cast<int>(frameCount / printElapsed.count())
                      << ", [Render Time / Frame Time] "
                      << std::fixed << std::setprecision(4)
                      << renderTimeAvgMs << " / " << frameTimeAvgMs << " ms ("
                      << std::setprecision(2)
                      << (100.0 * renderTimeAvgMs / frameTimeAvgMs) << "%), [CPU / GPU] "
                      << std::setprecision(4)
                      << cpuTimeAvgMs << " / " << gpuTimeAvgMs << " ms"
                      << std::endl;

            lastPrint = printNow;
            frameCount = 0;
            renderTime = 0.0;
            cpuTime = 0.0;
            gpuTime = 0.0;
        }
    }

    glDeleteProgram(program);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);

#ifdef GDM_HAS_GLFW
    glfwDestroyWindow(window);
    glfwTerminate();
#elif defined(GDM_HAS_FREEGLUT)
    glutDestroyWindow(glutGetWindow());
#endif

    return 0;
}
