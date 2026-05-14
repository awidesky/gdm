#include <glutil/gl.hpp>
#include <glutil/glToString.hpp>

#include <glutil/glutil.hpp>
#include <glutil/debug_snapshot.hpp>

#include <iostream>
#include <map>

#define LOG LOG_ERROR()

//
//
//
//
//    // TODO : 아래에 대한 TODO를 수행해야 함. 한번에 모든 걸 하려고 하지 말고, 하나하나 건당 정확하게 해결 후 각각을 확인받은 후에 커밋할 것.
//    // 
//    // 
//    // 1. static thread_local bool insideSnapshot = false;등 이용해서 recursion 방지
//    // 2. stacktrace처럼 builder함수를 이용한 객체로 다시 구조화. 필요한 정보만 골라서 출력할 수 있도록
//    //   예시:
//    // snapshot{true} //snapshot 타입 객체 생성, 생성자는 bool printAll = true, 모든 스냅샷 항목을 defualt로 true로 놓는가? 라는 의미
//    //          .shaderStatus(false) // 셰이더 관련 내용 출력할까?
//    //          .textureInfo(false) // 텍스쳐 관련 내용 출력할까?
//    //          .currntBufferOnly(true) // 현재 바인드된 VAO, VBO, EBO 관련 내용 출력할까? false면 현존하는 모든 VAO 출력(debug.hpp에 모든 VAO나 VBO를 관리하는 레지스트리가 있다고 가정)
//    //          .bufferContent(false) // VBO attribute 및 내부 데이터 출력할까? (glGetBufferSubData호출하므로 시간 오래 걸릴 수 있음)
//    //          .disabledAttribVBO(false) //VERTEX_ATTRIB_ARRAY_ENABLED가 아닌 것, 즉 glDisableVertexAttribArray(0);된 VBO도 출력
//    //          .shaderUniform(false) // shader 유니폼 값 출력할까?
//    //          .rendererState(false) // 렌더러 상태(뷰포트, depth-test enbale등등 여부..) 출력할까?
//    //          .boundInfo(false) // 기타 바인딩 정보 출력(GL_ARRAY_BUFFER_BINDING, GL_ELEMENT_ARRAY_BUFFER_BINDING, GL_UNIFORM_BUFFER_BINDING, GL_SHADER_STORAGE_BUFFER_BINDING, GL_PIXEL_PACK_BUFFER_BINDING, GL_PIXEL_UNPACK_BUFFER_BINDING, GL_TEXTURE_BINDING_2D,GL_SAMPLER_BINDING)
//    //          .capture(std::cerr); // 주어진 스트림(콘솔, 파일...)으로 출력
//    // 위와 같은 코드는 예시이며, 추가하거나 삭제하는 것이 필요한 경우 알맞게 처리할 것.
//    // 
//    // 필요한 경우 snapshot.hpp에는 일종의 프리셋처럼 설정된(오류가 났을 때 유용한 스냅샷, 셰이더 디버깅에 유용한 스냅샷, 버퍼 데이터 디버깅에 유용한 스냅샷 등등..) 다양한 객체를 리턴하는 헬퍼 함수가 존재한다
//    // 
//    // 그냥 glutil::debug::snapshot{}.capture(...)하면 모든 내용 다 출력
//    // 각 함수는 bool값만 바꾸고, print시에 실제 스냅샷을 찍고, 현재 snapshot함수처럼 분기하여 알맞게 출력하도록
//    // 
//    // 3. "=========== VBO Status ========="과 같이 구분선과 공백 출력하여, 잘 보이게 변경
//    // 
//    // 4. 유니폼이나 버텍스 데이터 출력할 때, 알맞게 길이 포매팅하여 한눈에 잘 들어올 수 있도록 해야 한다.
//    // 
//    // 5. 코드 내부에 있는 TODO도 수행해야 함!
//    // 
//    // 6. 함수 실행 중에 gl state를 오염시키지 않도록 주의해야 함. 실행 이후로는 모든 게 원래 상태여야 함.(VAO binding가 바뀌지 않도록)
//    // 7. glGetBufferSubData가 실패하지 않도록 GL_BUFFER_MAPPED확인
//    // 8. Texture Sampler State도 확인 (GL_TEXTURE_MIN_FILTER, GL_TEXTURE_MAG_FILTER, GL_TEXTURE_WRAP_S, GL_TEXTURE_COMPARE_FUNC 등등)
//    // 
//    //
//
//
//    
//    // Check Frame Buffer
//    const GLenum fbStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
//    LOG << "[OpenGL state dump]";
//    LOG << "  Framebuffer status: " << glutil::glErrorToString(fbStatus) << '(' << fbStatus << ')';
//
//    // Check Shader progame Link
//    GLint bound = 0;
//    glGetIntegerv(GL_CURRENT_PROGRAM, &bound);
//    if (bound != 0) {
//        GLint linkStatus = 0;
//        glGetProgramiv(bound, GL_LINK_STATUS, &linkStatus);
//        GLint infoLogLength = 0;
//        glGetProgramiv(bound, GL_INFO_LOG_LENGTH, &infoLogLength);
//        LOG << "  Current shader program ID: " << bound
//                    << ", LinkStatus: " << (linkStatus == GL_TRUE ? "OK" : "FAIL");
//        std::string infoLog(infoLogLength, '\0');
//        if (infoLogLength > 0) {
//            glGetProgramInfoLog(bound, infoLogLength, nullptr, infoLog.data());
//            LOG << "  Program InfoLog: " << infoLog;
//        }
//    } else {
//        LOG << "  No shader program bound";
//    }
//
//    // Check Texture
//    GLint currentUnit = 0;
//    glGetIntegerv(GL_ACTIVE_TEXTURE, &currentUnit);
//
//    GLint maxUnits = 0;
//    glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &maxUnits);
//
//
//    struct TexType {
//        GLenum binding;
//        GLenum target;
//        const char* name;
//    };
//    TexType types[] = {
//      {GL_TEXTURE_BINDING_2D, GL_TEXTURE_2D, "2D"},
//      {GL_TEXTURE_BINDING_CUBE_MAP, GL_TEXTURE_CUBE_MAP, "CubeMap"},
//      {GL_TEXTURE_BINDING_3D, GL_TEXTURE_3D, "3D"},
//    };
//
//
//    for (int i = 0; i < maxUnits; i++) {
//        glActiveTexture(GL_TEXTURE0 + i);
//        for (auto& t : types) {
//            GLint texId = 0;
//            glGetIntegerv(t.binding, &texId);
//            if (texId == 0)
//                continue;
//
//            GLint width = 0, height = 0, internalFormat = 0;
//            glGetTexLevelParameteriv(t.target, 0, GL_TEXTURE_WIDTH, &width);
//            glGetTexLevelParameteriv(t.target, 0, GL_TEXTURE_HEIGHT, &height);
//            glGetTexLevelParameteriv(t.target, 0, GL_TEXTURE_INTERNAL_FORMAT, &internalFormat);
//
//            LOG << "  Unit " << i << " (" << t.name << ")"
//                        << " ID=" << texId << " Size=" << width << "x" << height
//                        << " Format=" << glTextureFormatToString(internalFormat);
//        }
//    }
//    glActiveTexture(currentUnit);
//
//
//     // Check VAO/VBO limit 10
//    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &bound);
//    if (bound != 0) {
//        GLint vboBound = 0, bufSize = 0, usage = 0;
//        glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &vboBound);
//        if (vboBound != 0) {
//            glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &bufSize);
//            glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_USAGE, &usage);
//        }
//
//        const char* usageStr = "UNKNOWN";
//        switch (usage) {
//            case GL_STATIC_DRAW: usageStr = "GL_STATIC_DRAW"; break;
//            case GL_DYNAMIC_DRAW: usageStr = "GL_DYNAMIC_DRAW"; break;
//            case GL_STREAM_DRAW: usageStr = "GL_STREAM_DRAW"; break;
//        }
//
//        LOG << "  Current VAO bound: " << bound << "  Current global ARRAY_BUFFER binding (setup state)=" << vboBound << "  " << bufSize << " bytes"
//                    << "  " << usageStr;
//
//        GLint maxAttribs = 0;
//        glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &maxAttribs);
//
//        GLint prevVboId = -1;
//        std::vector<unsigned char> data;
//        GLint curBufSize = 0;
//
//        for (int i = 0; i < maxAttribs; i++) {
//            GLint enabled = 0;
//            glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &enabled);
//            if (!enabled)  //TODO : 디버깅을 위해 disable된 것도 출력하는 옵션 추가(disabledAttribVBO())
//                continue;
//
//            GLint vboId = 0, size = 0, type = 0, stride = 0;
//            void* offset = nullptr;
//            glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING, &vboId);
//            glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_SIZE, &size);
//            glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_TYPE, &type);
//            glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_STRIDE, &stride);
//            glGetVertexAttribPointerv(i, GL_VERTEX_ATTRIB_ARRAY_POINTER, &offset);
//            uintptr_t off = reinterpret_cast<uintptr_t>(offset);
//
//            // if VBO is changed, read another one
//            if (vboId != prevVboId) {
//                glBindBuffer(GL_ARRAY_BUFFER, vboId);
//
//                GLint newUsage = 0;
//                glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &curBufSize);
//                glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_USAGE, &newUsage);
//
//                const char* usageStr = "UNKNOWN";
//                switch (newUsage) {
//                    case GL_STATIC_DRAW: usageStr = "GL_STATIC_DRAW"; break;
//                    case GL_DYNAMIC_DRAW: usageStr = "GL_DYNAMIC_DRAW"; break;
//                    case GL_STREAM_DRAW: usageStr = "GL_STREAM_DRAW"; break;
//                }
//
//                LOG << "  [VBO ID=" << vboId << "  " << curBufSize << " bytes"
//                            << "  " << usageStr << "]";
//
//                data.resize(curBufSize);
//                glGetBufferSubData(GL_ARRAY_BUFFER, 0, curBufSize, data.data());
//                prevVboId = vboId;
//            }
//
//            LOG << "    attrib[" << i << "]"
//                        << " vbo=" << vboId << " size=" << size << " type=" << glTypeToString(type)
//                        << " stride=" << stride << " offset=" << off;
//
//            // TODO : float가 아닌 경우 stride계산 잘못됨
//            int s = stride == 0 ? size * (int)sizeof(float) : stride;
//            int numVerts = curBufSize / s;
//            int printNum = std::min(numVerts, 10);
//
//            for (int v = 0; v < printNum; v++) {
//                // TODO : float아닌 값 있을 수도 있음!
//                const float* ptr = reinterpret_cast<const float*>(data.data() + v * s + off);
//                std::ostringstream oss;
//                oss << "      vertex[" << v << "]: (";
//                for (int c = 0; c < size; c++) {
//                    oss << ptr[c];
//                    if (c < size - 1)
//                        oss << ", ";
//                }
//                oss << ")";
//                LOG << oss.str();
//            }
//            if (numVerts > 10)
//                LOG << "      ... (" << numVerts - 10 << " more)";
//        }
//        glBindBuffer(GL_ARRAY_BUFFER, vboBound);
//    } else {
//        LOG << "  No VAO bound: " << bound;
//    }
//
//
//    //Check EBO
//    GLint eboBound = 0;
//    glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &eboBound);
//    if (eboBound != 0) {
//        GLint eboSize = 0, eboUsage = 0;
//        glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &eboSize);
//        glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_USAGE, &eboUsage);
//
//        const char* eboUsageStr = "UNKNOWN";
//        switch (eboUsage) {
//            case GL_STATIC_DRAW: eboUsageStr = "GL_STATIC_DRAW"; break;
//            case GL_DYNAMIC_DRAW: eboUsageStr = "GL_DYNAMIC_DRAW"; break;
//            case GL_STREAM_DRAW: eboUsageStr = "GL_STREAM_DRAW"; break;
//        }
//
//        LOG << "  [EBO dump] ID=" << eboBound << "  " << eboSize << " bytes"
//                    << "  " << eboUsageStr;
//
//        // VRAM -> CPU 복사
//        // TODO : GL_UNSIGNED_SHORT 등 다양한 여러 다른 타입일 수 있음!
//        std::vector<unsigned int> indices(eboSize / sizeof(unsigned int));
//        glGetBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, eboSize, indices.data());
//
//        // 최대 30개 출력
//        int printNum = std::min((int)indices.size(), 30);
//        std::ostringstream oss;
//        oss << "    indices: ";
//        for (int i = 0; i < printNum; i++) {
//            oss << indices[i];
//            if (i < printNum - 1)
//                oss << ", ";
//        }
//        if ((int)indices.size() > 30)
//            oss << " ... (" << indices.size() - 30 << " more)";
//        LOG << oss.str();
//    } else {
//        LOG << "  EBO: (none)";
//    }
//
//    //Check Shader Uniform Value
//    GLint program = 0;
//    glGetIntegerv(GL_CURRENT_PROGRAM, &program);
//    if (program != 0) {
//        GLint count = 0;
//        glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &count);
//
//        GLint maxLen = 0;
//        glGetProgramiv(program, GL_ACTIVE_UNIFORM_MAX_LENGTH, &maxLen);
//
//        LOG << "  [Uniforms] Shader ID=" << program << " count=" << count;
//
//        std::vector<char> name(maxLen);
//
//        for (GLint i = 0; i < count; i++) {
//            GLint size = 0;
//            GLenum type = 0;
//            glGetActiveUniform(program, i, maxLen, nullptr, &size, &type, name.data());
//
//            GLint loc = glGetUniformLocation(program, name.data());
//            if (loc == -1)
//                continue;
//
//            std::ostringstream oss;
//            oss << "    " << name.data() << " (";
//
//            switch (type) {
//                case GL_FLOAT: {
//                    GLfloat v;
//                    glGetUniformfv(program, loc, &v);
//                    oss << "float) = " << v;
//                    break;
//                }
//                case GL_FLOAT_VEC2: {
//                    GLfloat v[2];
//                    glGetUniformfv(program, loc, v);
//                    oss << "vec2) = (" << v[0] << ", " << v[1] << ")";
//                    break;
//                }
//                case GL_FLOAT_VEC3: {
//                    GLfloat v[3];
//                    glGetUniformfv(program, loc, v);
//                    oss << "vec3) = (" << v[0] << ", " << v[1] << ", " << v[2] << ")";
//                    break;
//                }
//                case GL_FLOAT_VEC4: {
//                    GLfloat v[4];
//                    glGetUniformfv(program, loc, v);
//                    oss << "vec4) = (" << v[0] << ", " << v[1] << ", " << v[2] << ", " << v[3] << ")";
//                    break;
//                }
//                case GL_FLOAT_MAT4: {
//                    GLfloat v[16];
//                    glGetUniformfv(program, loc, v);
//                    oss << "mat4) =\n";
//                    for (int row = 0; row < 4; row++) {
//                        oss << "                    [ ";
//                        for (int col = 0; col < 4; col++)
//                            oss << v[col * 4 + row] << " ";
//                        oss << "]";
//                        if (row < 3)
//                            oss << "\n";
//                    }
//                    break;
//                }
//                case GL_INT: {
//                    GLint v;
//                    glGetUniformiv(program, loc, &v);
//                    oss << "int) = " << v;
//                    break;
//                }
//                case GL_SAMPLER_2D: {
//                    GLint v;
//                    glGetUniformiv(program, loc, &v);
//                    oss << "sampler2D) = unit " << v;
//                    break;
//                }
//                default: oss << "type=0x" << std::hex << type << std::dec << ") = ?";
//            }
//            LOG << oss.str();
//        }
//    } else {
//        LOG << "  [Uniforms] No shader bound";
//    }
//
//    
//
//    // Check Render State
//    //TODO : 더 많은 Framebuffer 정보(mip level, multisample 여부, internal format) 제공
//    LOG << "  [Render State]";
//    // Check Viewport
//    GLint vp[4];
//    glGetIntegerv(GL_VIEWPORT, vp);
//    LOG << "    Viewport: x=" << vp[0] << ", y=" << vp[1] << ", w=" << vp[2] << ", h=" << vp[3];
//    LOG << "    Depth Test: " << (glIsEnabled(GL_DEPTH_TEST) ? "ON" : "OFF");
//    LOG << "    Blend     : " << (glIsEnabled(GL_BLEND) ? "ON" : "OFF");
//    LOG << "    Cull Face : " << (glIsEnabled(GL_CULL_FACE) ? "ON" : "OFF");
//}
