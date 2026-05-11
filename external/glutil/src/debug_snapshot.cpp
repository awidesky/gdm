#include <glutil/gl.hpp>
#include <glutil/glToString.hpp>

#include <glutil/glutil.hpp>
#include <glutil/debug_snapshot.hpp>

#include <iostream>
#include <map>

#define LOG LOG_ERROR()

void glutil::debug::snapshot()  {

    // TODO : 
    // 1. static thread_local bool insideSnapshot = false;등 이용해서 recursion 방지
    // 2. stacktrace처럼 builder함수를 이용한 객체로 다시 구조화. 필요한 정보만 골라서 출력할 수 있도록
    // 예시 :
    //    glutil::debug::snapshot{} //snapshot 타입 객체 생성
    //          .shader(false) // 셰이더 관련 내용 출력할까?
    //          .ebo(false) // EBO 관련 내용 출력할까?
    //          .vbo(true) // VBO id나 크기 출력할까?
    //          .vboAttribute(false) // VBO attribute 데이터 출력할까? (glGetBufferSubData호출하므로 시간 오래 걸릴 수 있음)
    //          .shaderUniform(false) // shader 유니폼 값 출력할까?
    //          .print(std::cerr); // 주어진 스트림(콘솔, 파일...)으로 출력
    // 
    // 그냥 glutil::debug::snapshot{}.print(...)하면 모든 내용 다 출력
    // 각 함수는 bool값만 바꾸고, print시에 알맞게 출력하도록
    // 
    // 3. "=========== VBO Status ========="과 같이 구분선과 공백 출력하여, 잘 보이게 변경
    // 
    // 4. 유니폼이나 버텍스 데이터 출력할 때, 알맞게 길이 포매팅하여 한눈에 잘 들어올 수 있도록 해야 한다.
    // 
    // 5. 코드 내부에 있는 TODO도 수행해야 함!
    // 
    // 6. 함수 실행 중에 gl state를 오염시키지 않도록 주의해야 함. 실행 이후로는 모든 게 원래 상태여야 함.

    
    // Check Frame Buffer
    const GLenum fbStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    LOG << "[OpenGL state dump]";
    LOG << "  Framebuffer status: " << glutil::glErrorToString(fbStatus) << '(' << fbStatus << ')';

    // Check Shader progame Link
    GLint bound = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &bound);
    if (bound != 0) {
        GLint linkStatus = 0;
        glGetProgramiv(bound, GL_LINK_STATUS, &linkStatus);
        GLint infoLogLength = 0;
        glGetProgramiv(bound, GL_INFO_LOG_LENGTH, &infoLogLength);
        LOG << "  Current shader program ID: " << bound
                    << ", LinkStatus: " << (linkStatus == GL_TRUE ? "OK" : "FAIL");
        std::string infoLog(infoLogLength, '\0');
        if (infoLogLength > 0) {
            glGetProgramInfoLog(bound, infoLogLength, nullptr, infoLog.data());
            LOG << "  Program InfoLog: " << infoLog;
        }
    } else {
        LOG << "  No shader program bound";
    }

    // Check Texture
    GLint currentUnit = 0;
    glGetIntegerv(GL_ACTIVE_TEXTURE, &currentUnit);

    GLint maxUnits = 0;
    glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &maxUnits);


    struct TexType {
        GLenum binding;
        GLenum target;
        const char* name;
    };
    TexType types[] = {
      {GL_TEXTURE_BINDING_2D, GL_TEXTURE_2D, "2D"},
      {GL_TEXTURE_BINDING_CUBE_MAP, GL_TEXTURE_CUBE_MAP, "CubeMap"},
      {GL_TEXTURE_BINDING_3D, GL_TEXTURE_3D, "3D"},
    };


    for (int i = 0; i < maxUnits; i++) {
        glActiveTexture(GL_TEXTURE0 + i);
        for (auto& t : types) {
            GLint texId = 0;
            glGetIntegerv(t.binding, &texId);
            if (texId == 0)
                continue;

            GLint width = 0, height = 0, internalFormat = 0;
            glGetTexLevelParameteriv(t.target, 0, GL_TEXTURE_WIDTH, &width);
            glGetTexLevelParameteriv(t.target, 0, GL_TEXTURE_HEIGHT, &height);
            glGetTexLevelParameteriv(t.target, 0, GL_TEXTURE_INTERNAL_FORMAT, &internalFormat);

            LOG << "  Unit " << i << " (" << t.name << ")"
                        << " ID=" << texId << " Size=" << width << "x" << height
                        << " Format=" << glTextureFormatToString(internalFormat);
        }
    }
    glActiveTexture(currentUnit);


     // Check VAO/VBO limit 10
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &bound);
    if (bound != 0) {
        GLint vboBound = 0, bufSize = 0, usage = 0;
        glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &vboBound);
        if (vboBound != 0) {
            glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &bufSize);
            glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_USAGE, &usage);
        }

        const char* usageStr = "UNKNOWN";
        switch (usage) {
            case GL_STATIC_DRAW: usageStr = "GL_STATIC_DRAW"; break;
            case GL_DYNAMIC_DRAW: usageStr = "GL_DYNAMIC_DRAW"; break;
            case GL_STREAM_DRAW: usageStr = "GL_STREAM_DRAW"; break;
        }

        LOG << "  Current VAO bound: " << bound << "  Current global ARRAY_BUFFER binding (setup state)=" << vboBound << "  " << bufSize << " bytes"
                    << "  " << usageStr;

        GLint maxAttribs = 0;
        glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &maxAttribs);

        GLint prevVboId = -1;
        std::vector<unsigned char> data;
        GLint curBufSize = 0;

        for (int i = 0; i < maxAttribs; i++) {
            GLint enabled = 0;
            glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &enabled);
            if (!enabled)
                continue;

            GLint vboId = 0, size = 0, type = 0, stride = 0;
            void* offset = nullptr;
            glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING, &vboId);
            glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_SIZE, &size);
            glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_TYPE, &type);
            glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_STRIDE, &stride);
            glGetVertexAttribPointerv(i, GL_VERTEX_ATTRIB_ARRAY_POINTER, &offset);
            uintptr_t off = reinterpret_cast<uintptr_t>(offset);

            // if VBO is changed, read another one
            if (vboId != prevVboId) {
                glBindBuffer(GL_ARRAY_BUFFER, vboId);

                GLint newUsage = 0;
                glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &curBufSize);
                glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_USAGE, &newUsage);

                const char* usageStr = "UNKNOWN";
                switch (newUsage) {
                    case GL_STATIC_DRAW: usageStr = "GL_STATIC_DRAW"; break;
                    case GL_DYNAMIC_DRAW: usageStr = "GL_DYNAMIC_DRAW"; break;
                    case GL_STREAM_DRAW: usageStr = "GL_STREAM_DRAW"; break;
                }

                LOG << "  [VBO ID=" << vboId << "  " << curBufSize << " bytes"
                            << "  " << usageStr << "]";

                data.resize(curBufSize);
                glGetBufferSubData(GL_ARRAY_BUFFER, 0, curBufSize, data.data());
                prevVboId = vboId;
            }

            LOG << "    attrib[" << i << "]"
                        << " vbo=" << vboId << " size=" << size << " type=" << glTypeToString(type)
                        << " stride=" << stride << " offset=" << off;

            // TODO : float가 아닌 경우 stride계산 잘못됨
            int s = stride == 0 ? size * (int)sizeof(float) : stride;
            int numVerts = curBufSize / s;
            int printNum = std::min(numVerts, 10);

            for (int v = 0; v < printNum; v++) {
                // TODO : float아닌 값 있을 수도 있음!
                const float* ptr = reinterpret_cast<const float*>(data.data() + v * s + off);
                std::ostringstream oss;
                oss << "      vertex[" << v << "]: (";
                for (int c = 0; c < size; c++) {
                    oss << ptr[c];
                    if (c < size - 1)
                        oss << ", ";
                }
                oss << ")";
                LOG << oss.str();
            }
            if (numVerts > 10)
                LOG << "      ... (" << numVerts - 10 << " more)";
        }
        glBindBuffer(GL_ARRAY_BUFFER, vboBound);
    } else {
        LOG << "  No VAO bound: " << bound;
    }


    //Check EBO
    GLint eboBound = 0;
    glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &eboBound);
    if (eboBound != 0) {
        GLint eboSize = 0, eboUsage = 0;
        glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &eboSize);
        glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_USAGE, &eboUsage);

        const char* eboUsageStr = "UNKNOWN";
        switch (eboUsage) {
            case GL_STATIC_DRAW: eboUsageStr = "GL_STATIC_DRAW"; break;
            case GL_DYNAMIC_DRAW: eboUsageStr = "GL_DYNAMIC_DRAW"; break;
            case GL_STREAM_DRAW: eboUsageStr = "GL_STREAM_DRAW"; break;
        }

        LOG << "  [EBO dump] ID=" << eboBound << "  " << eboSize << " bytes"
                    << "  " << eboUsageStr;

        // VRAM -> CPU 복사
        // TODO : GL_UNSIGNED_SHORT 등 다양한 여러 다른 타입일 수 있음!
        std::vector<unsigned int> indices(eboSize / sizeof(unsigned int));
        glGetBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, eboSize, indices.data());

        // 최대 30개 출력
        int printNum = std::min((int)indices.size(), 30);
        std::ostringstream oss;
        oss << "    indices: ";
        for (int i = 0; i < printNum; i++) {
            oss << indices[i];
            if (i < printNum - 1)
                oss << ", ";
        }
        if ((int)indices.size() > 30)
            oss << " ... (" << indices.size() - 30 << " more)";
        LOG << oss.str();
    } else {
        LOG << "  EBO: (none)";
    }

    //Check Shader Uniform Value
    GLint program = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &program);
    if (program != 0) {
        GLint count = 0;
        glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &count);

        GLint maxLen = 0;
        glGetProgramiv(program, GL_ACTIVE_UNIFORM_MAX_LENGTH, &maxLen);

        LOG << "  [Uniforms] Shader ID=" << program << " count=" << count;

        std::vector<char> name(maxLen);

        for (GLint i = 0; i < count; i++) {
            GLint size = 0;
            GLenum type = 0;
            glGetActiveUniform(program, i, maxLen, nullptr, &size, &type, name.data());

            GLint loc = glGetUniformLocation(program, name.data());
            if (loc == -1)
                continue;

            std::ostringstream oss;
            oss << "    " << name.data() << " (";

            switch (type) {
                case GL_FLOAT: {
                    GLfloat v;
                    glGetUniformfv(program, loc, &v);
                    oss << "float) = " << v;
                    break;
                }
                case GL_FLOAT_VEC2: {
                    GLfloat v[2];
                    glGetUniformfv(program, loc, v);
                    oss << "vec2) = (" << v[0] << ", " << v[1] << ")";
                    break;
                }
                case GL_FLOAT_VEC3: {
                    GLfloat v[3];
                    glGetUniformfv(program, loc, v);
                    oss << "vec3) = (" << v[0] << ", " << v[1] << ", " << v[2] << ")";
                    break;
                }
                case GL_FLOAT_VEC4: {
                    GLfloat v[4];
                    glGetUniformfv(program, loc, v);
                    oss << "vec4) = (" << v[0] << ", " << v[1] << ", " << v[2] << ", " << v[3] << ")";
                    break;
                }
                case GL_FLOAT_MAT4: {
                    GLfloat v[16];
                    glGetUniformfv(program, loc, v);
                    oss << "mat4) =\n";
                    for (int row = 0; row < 4; row++) {
                        oss << "                    [ ";
                        for (int col = 0; col < 4; col++)
                            oss << v[col * 4 + row] << " ";
                        oss << "]";
                        if (row < 3)
                            oss << "\n";
                    }
                    break;
                }
                case GL_INT: {
                    GLint v;
                    glGetUniformiv(program, loc, &v);
                    oss << "int) = " << v;
                    break;
                }
                case GL_SAMPLER_2D: {
                    GLint v;
                    glGetUniformiv(program, loc, &v);
                    oss << "sampler2D) = unit " << v;
                    break;
                }
                default: oss << "type=0x" << std::hex << type << std::dec << ") = ?";
            }
            LOG << oss.str();
        }
    } else {
        LOG << "  [Uniforms] No shader bound";
    }

    

    // Check Render State
    LOG << "  [Render State]";
    // Check Viewport
    GLint vp[4];
    glGetIntegerv(GL_VIEWPORT, vp);
    LOG << "    Viewport: x=" << vp[0] << ", y=" << vp[1] << ", w=" << vp[2] << ", h=" << vp[3];
    LOG << "    Depth Test: " << (glIsEnabled(GL_DEPTH_TEST) ? "ON" : "OFF");
    LOG << "    Blend     : " << (glIsEnabled(GL_BLEND) ? "ON" : "OFF");
    LOG << "    Cull Face : " << (glIsEnabled(GL_CULL_FACE) ? "ON" : "OFF");
}
