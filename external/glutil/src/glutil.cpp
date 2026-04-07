#define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING
#define STB_IMAGE_IMPLEMENTATION
#include "glutil/glutil.hpp"
//#include "stb_image.h"
//#include <GL/glew.h>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <experimental/filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>
namespace fs = std::experimental::filesystem;

// ----------------------------------
// ResourceLogger
// ----------------------------------
std::vector<LogEntry> ResourceLogger::Logs;

void ResourceLogger::AddLog(const LogEntry& entry) { Logs.push_back(entry); }

void ResourceLogger::PrintLogs() {
    for (const auto& log : Logs) {
        std::string fileName = log.path;

        // 경로 → 파일명만 추출
        try {
            fileName = fs::path(log.path).filename().string();
        } catch (...) {
        }

        std::cout << "[" << log.type << "]"
                  << " | "
                  << "[" << log.status << "]"
                  << " | "
                  << "[" << fileName << "]"
                  << " | " << log.message << "\n";
    }
}

void ResourceLogger::ClearLogs() { Logs.clear(); }

const std::vector<LogEntry>& ResourceLogger::GetLogs() { return Logs; }

// ----------------------------------
// PathResolve
// ----------------------------------
PathResolveResult PathResolve(const std::string& inputPath) {
    PathResolveResult result;
    result.originalPath = inputPath;

    fs::path input(inputPath);

    // 1. 절대 경로
    if (input.is_absolute()) {
        if (fs::exists(input)) {
            result.success = true;
            result.resolvedPath = fs::absolute(input).string();
            result.message = "절대 경로 확인 성공";
            return result;
        }

        result.message = "절대 경로 파일 없음";
        return result;
    }

    // 2. LearnOpenGL 스타일
    fs::path fsPath = fs::current_path() / "../../../" / input;
    if (fs::exists(fsPath)) {
        result.success = true;
        result.resolvedPath = fs::absolute(fsPath).string();
        result.message = "FileSystem 방식 경로 성공";
        return result;
    }

    // 3. 일반 후보
    std::vector<fs::path> candidates = {fs::current_path() / input,
                                        fs::current_path() / "resources" / input,
                                        fs::current_path() / "resources" / "textures" / input,
                                        fs::current_path() / "textures" / input,
                                        fs::current_path() / "models" / input,
                                        fs::current_path() / "assets" / input,
                                        fs::current_path() / "assets" / "textures" / input,
                                        fs::current_path() / "assets" / "models" / input,
                                        fs::current_path() / "shaders" / input,
                                        fs::current_path() / "assets" / "shaders" / input};

    for (const auto& candidate : candidates) {
        if (fs::exists(candidate)) {
            result.success = true;
            result.resolvedPath = fs::absolute(candidate).string();
            result.message = "일반 경로 탐색 성공";
            return result;
        }
    }

    result.message = "파일을 찾지 못함";
    return result;
}

// ----------------------------------
// Shader
// ----------------------------------
Shader::Shader(const char* vertexPath, const char* fragmentPath) { Load(vertexPath, fragmentPath); }

bool Shader::Load(const char* vertexPath, const char* fragmentPath) {
    Release();

    if (vertexPath == nullptr || fragmentPath == nullptr) {
        ResourceLogger::AddLog({"Shader", "Failed", "(null)", "vertexPath 또는 fragmentPath가 nullptr임"});
        return false;
    }

    PathResolveResult vsPathResult = PathResolve(vertexPath);
    if (!vsPathResult.success) {
        ResourceLogger::AddLog(
          {"Shader", "Failed", vertexPath, "Vertex shader 경로 확인 실패: " + vsPathResult.message});
        return false;
    }
    ResourceLogger::AddLog({"Shader", "Success", vsPathResult.resolvedPath, "Vertex shader 경로 확인 성공"});

    PathResolveResult fsPathResult = PathResolve(fragmentPath);
    if (!fsPathResult.success) {
        ResourceLogger::AddLog(
          {"Shader", "Failed", fragmentPath, "Fragment shader 경로 확인 실패: " + fsPathResult.message});
        return false;
    }
    ResourceLogger::AddLog({"Shader", "Success", fsPathResult.resolvedPath, "Fragment shader 경로 확인 성공"});

    std::string vertexCode;
    std::string fragmentCode;

    if (!ReadFileToString(vsPathResult.resolvedPath, vertexCode)) {
        ResourceLogger::AddLog({"Shader", "Failed", vsPathResult.resolvedPath, "Vertex shader 파일 읽기 실패"});
        return false;
    }
    ResourceLogger::AddLog({"Shader", "Success", vsPathResult.resolvedPath, "Vertex shader 파일 읽기 성공"});

    if (!ReadFileToString(fsPathResult.resolvedPath, fragmentCode)) {
        ResourceLogger::AddLog({"Shader", "Failed", fsPathResult.resolvedPath, "Fragment shader 파일 읽기 실패"});
        return false;
    }
    ResourceLogger::AddLog({"Shader", "Success", fsPathResult.resolvedPath, "Fragment shader 파일 읽기 성공"});

    if (vertexCode.empty()) {
        ResourceLogger::AddLog({"Shader", "Failed", vsPathResult.resolvedPath, "Vertex shader 소스가 비어 있음"});
        return false;
    }

    if (fragmentCode.empty()) {
        ResourceLogger::AddLog({"Shader", "Failed", fsPathResult.resolvedPath, "Fragment shader 소스가 비어 있음"});
        return false;
    }

    GLuint vertex = CompileShader(GL_VERTEX_SHADER, vertexCode, vsPathResult.resolvedPath, "Vertex shader");
    if (vertex == 0)
        return false;

    GLuint fragment = CompileShader(GL_FRAGMENT_SHADER, fragmentCode, fsPathResult.resolvedPath, "Fragment shader");
    if (fragment == 0) {
        glDeleteShader(vertex);
        return false;
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);

    GLint success = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &success);

    if (success != GL_TRUE) {
        char infoLog[1024];
        glGetProgramInfoLog(program, 1024, NULL, infoLog);

        ResourceLogger::AddLog({"Shader", "Failed", vsPathResult.resolvedPath + " + " + fsPathResult.resolvedPath,
                                "프로그램 링크 실패: " + std::string(infoLog)});

        glDeleteShader(vertex);
        glDeleteShader(fragment);
        glDeleteProgram(program);
        return false;
    }

    ResourceLogger::AddLog(
      {"Shader", "Success", vsPathResult.resolvedPath + " + " + fsPathResult.resolvedPath, "프로그램 링크 성공"});

    glDeleteShader(vertex);
    glDeleteShader(fragment);

    ID = program;
    return true;
}

void Shader::Use() const {
    if (ID != 0)
        glUseProgram(ID);
}

void Shader::Release() {
    if (ID != 0) {
        glDeleteProgram(ID);
        ID = 0;
    }
}

void Shader::SetBool(const std::string& name, bool value) const {
    glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value);
}

void Shader::SetInt(const std::string& name, int value) const {
    glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
}

void Shader::SetFloat(const std::string& name, float value) const {
    glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
}

bool Shader::ReadFileToString(const std::string& path, std::string& outText) {
    std::ifstream file(path);
    if (!file.is_open())
        return false;

    std::stringstream ss;
    ss << file.rdbuf();
    outText = ss.str();
    return true;
}

GLuint Shader::CompileShader(GLenum shaderType, const std::string& source, const std::string& path,
                             const std::string& shaderLabel) {
    GLuint shader = glCreateShader(shaderType);
    const char* src = source.c_str();

    glShaderSource(shader, 1, &src, NULL);
    glCompileShader(shader);

    GLint success = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

    if (success == GL_TRUE) {
        ResourceLogger::AddLog({"Shader", "Success", path, shaderLabel + " 컴파일 성공"});
        return shader;
    }

    char infoLog[1024];
    glGetShaderInfoLog(shader, 1024, NULL, infoLog);

    ResourceLogger::AddLog({"Shader", "Failed", path, shaderLabel + " 컴파일 실패: " + std::string(infoLog)});

    glDeleteShader(shader);
    return 0;
}

// ----------------------------------
// Texture          DDS 랑 jpg 지원
// ----------------------------------
Texture::Texture(const char* texturePath, bool bFlipVertically) { Load(texturePath, bFlipVertically); }

bool Texture::Load(const char* texturePath, bool bFlipVertically) {
    Release();

    if (texturePath == nullptr) {
        ResourceLogger::AddLog({"Texture", "Failed", "(null)", "입력 경로가 nullptr임"});
        return false;
    }

    PathResolveResult pathResult = PathResolve(texturePath);
    if (!pathResult.success) {
        ResourceLogger::AddLog({"Texture", "Failed", texturePath, "Texture 경로 확인 실패: " + pathResult.message});
        return false;
    }

    LoadedPath = pathResult.resolvedPath;
    ResourceLogger::AddLog({"Texture", "Success", LoadedPath, "Texture 경로 확인 성공"});

    const std::string ext = GetExtension(LoadedPath);

    if (ext == ".dds")
        return LoadDDS(LoadedPath);

    return LoadStandardImage(LoadedPath, bFlipVertically);
}

bool Texture::LoadStandardImage(const std::string& path, bool bFlipVertically) {
    stbi_set_flip_vertically_on_load(bFlipVertically ? 1 : 0);

    int width = 0;
    int height = 0;
    int channels = 0;

    unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 0);
    if (!data) {
        std::string reason = stbi_failure_reason() ? stbi_failure_reason() : "알 수 없는 stb_image 오류";
        ResourceLogger::AddLog({"Texture", "Failed", path, "이미지 로딩 실패: " + reason});
        return false;
    }

    ResourceLogger::AddLog({"Texture", "Success", path,
                            "이미지 로딩 성공 (" + std::to_string(width) + "x" + std::to_string(height) +
                              ", channels=" + std::to_string(channels) +
                              ", flip=" + std::string(bFlipVertically ? "true" : "false") + ")"});

    const bool bOk = CreateTexture2DFromRaw(data, width, height, channels, path);
    stbi_image_free(data);
    return bOk;
}

bool Texture::CreateTexture2DFromRaw(unsigned char* data, int width, int height, int channels,
                                     const std::string& path) {
    if (data == nullptr) {
        ResourceLogger::AddLog({"Texture", "Failed", path, "입력 이미지 데이터가 nullptr임"});
        return false;
    }

    GLenum format = GL_RGB;
    GLenum internalFormat = GL_RGB8;

    switch (channels) {
        case 1:
            format = GL_RED;
            internalFormat = GL_R8;
            break;
        case 3:
            format = GL_RGB;
            internalFormat = GL_RGB8;
            break;
        case 4:
            format = GL_RGBA;
            internalFormat = GL_RGBA8;
            break;
        default:
            ResourceLogger::AddLog({"Texture", "Failed", path, "지원하지 않는 채널 수: " + std::to_string(channels)});
            return false;
    }

    glGenTextures(1, &ID);
    if (ID == 0) {
        ResourceLogger::AddLog({"Texture", "Failed", path, "glGenTextures 실패: texture ID가 0임"});
        return false;
    }

    glBindTexture(GL_TEXTURE_2D, ID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);

    Width = width;
    Height = height;
    Channels = channels;
    bCompressed = false;

    ResourceLogger::AddLog({"Texture", "Success", path,
                            "GPU 업로드 성공 (internalFormat=" + std::to_string(internalFormat) +
                              ", format=" + std::to_string(format) + ", size=" + std::to_string(width) + "x" +
                              std::to_string(height) + ")"});

    return true;
}

#define FOURCC_DXT1 0x31545844
#define FOURCC_DXT3 0x33545844
#define FOURCC_DXT5 0x35545844

#define FOURCC_DXT1 0x31545844
#define FOURCC_DXT3 0x33545844
#define FOURCC_DXT5 0x35545844

bool Texture::LoadDDS(const std::string& path) {
    FILE* fp = fopen(path.c_str(), "rb");
    if (fp == nullptr) {
        ResourceLogger::AddLog({"Texture", "Failed", path, "DDS 파일 열기 실패"});
        return false;
    }

    // 1. 파일 시그니처 확인
    char filecode[4];
    if (fread(filecode, 1, 4, fp) != 4 || strncmp(filecode, "DDS ", 4) != 0) {
        fclose(fp);
        ResourceLogger::AddLog({"Texture", "Failed", path, "올바른 DDS 파일이 아님"});
        return false;
    }

    // 2. 헤더 읽기
    unsigned char header[124];
    if (fread(header, 1, 124, fp) != 124) {
        fclose(fp);
        ResourceLogger::AddLog({"Texture", "Failed", path, "DDS 헤더 읽기 실패"});
        return false;
    }

    unsigned int height = *(unsigned int*)&(header[8]);
    unsigned int width = *(unsigned int*)&(header[12]);
    unsigned int linearSize = *(unsigned int*)&(header[16]);
    unsigned int mipMapCount = *(unsigned int*)&(header[24]);
    unsigned int fourCC = *(unsigned int*)&(header[80]);

    if (mipMapCount == 0)
        mipMapCount = 1;

    ResourceLogger::AddLog({"Texture", "Success", path,
                            "DDS 헤더 읽기 성공 (width=" + std::to_string(width) +
                              ", height=" + std::to_string(height) + ", linearSize=" + std::to_string(linearSize) +
                              ", mipMapCount=" + std::to_string(mipMapCount) + ", fourCC=" + std::to_string(fourCC) +
                              ")"});

    // 3. 남은 파일 전체 크기 계산
    if (fseek(fp, 0, SEEK_END) != 0) {
        fclose(fp);
        ResourceLogger::AddLog({"Texture", "Failed", path, "DDS 파일 끝 이동 실패"});
        return false;
    }

    long fileSize = ftell(fp);
    if (fileSize < 0) {
        fclose(fp);
        ResourceLogger::AddLog({"Texture", "Failed", path, "DDS 파일 크기 확인 실패"});
        return false;
    }

    // DDS: 4바이트("DDS ") + 124바이트(header) = 128바이트
    long dataOffset = 128;
    long remainingSize = fileSize - dataOffset;

    if (remainingSize <= 0) {
        fclose(fp);
        ResourceLogger::AddLog({"Texture", "Failed", path, "DDS 데이터 영역이 비어 있음"});
        return false;
    }

    if (fseek(fp, dataOffset, SEEK_SET) != 0) {
        fclose(fp);
        ResourceLogger::AddLog({"Texture", "Failed", path, "DDS 데이터 시작 위치 이동 실패"});
        return false;
    }

    unsigned char* buffer = (unsigned char*)malloc((size_t)remainingSize);
    if (buffer == nullptr) {
        fclose(fp);
        ResourceLogger::AddLog({"Texture", "Failed", path, "DDS 버퍼 메모리 할당 실패"});
        return false;
    }

    size_t readBytes = fread(buffer, 1, (size_t)remainingSize, fp);
    fclose(fp);

    if (readBytes != (size_t)remainingSize) {
        free(buffer);
        ResourceLogger::AddLog(
          {"Texture", "Failed", path,
           "DDS 본문 읽기 실패: expected=" + std::to_string(remainingSize) + ", actual=" + std::to_string(readBytes)});
        return false;
    }

    // 4. 포맷 판별
    GLenum format = 0;
    std::string formatName;

    switch (fourCC) {
        case FOURCC_DXT1:
            format = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
            formatName = "DXT1";
            break;
        case FOURCC_DXT3:
            format = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
            formatName = "DXT3";
            break;
        case FOURCC_DXT5:
            format = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
            formatName = "DXT5";
            break;
        default:
            free(buffer);
            ResourceLogger::AddLog({"Texture", "Failed", path, "지원하지 않는 DDS 압축 포맷"});
            return false;
    }

    // 5. 텍스처 생성
    glGenTextures(1, &ID);
    if (ID == 0) {
        free(buffer);
        ResourceLogger::AddLog({"Texture", "Failed", path, "glGenTextures 실패: texture ID가 0임"});
        return false;
    }

    glBindTexture(GL_TEXTURE_2D, ID);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    unsigned int blockSize = (format == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT) ? 8 : 16;
    size_t offset = 0;
    size_t totalSize = readBytes;

    unsigned int currentWidth = width;
    unsigned int currentHeight = height;
    unsigned int uploadedMipCount = 0;

    // 6. mipmap 업로드
    for (unsigned int level = 0; level < mipMapCount && (currentWidth || currentHeight); ++level) {
        unsigned int size = ((currentWidth + 3) / 4) * ((currentHeight + 3) / 4) * blockSize;

        if (offset + size > totalSize) {
            glBindTexture(GL_TEXTURE_2D, 0);
            glDeleteTextures(1, &ID);
            ID = 0;
            free(buffer);

            ResourceLogger::AddLog({"Texture", "Failed", path,
                                    "DDS mipmap 데이터 범위 초과: level=" + std::to_string(level) +
                                      ", need=" + std::to_string(size) + ", offset=" + std::to_string(offset) +
                                      ", total=" + std::to_string(totalSize)});
            return false;
        }

        glCompressedTexImage2D(GL_TEXTURE_2D, level, format, currentWidth, currentHeight, 0, size, buffer + offset);

        offset += size;
        currentWidth /= 2;
        currentHeight /= 2;

        if (currentWidth < 1)
            currentWidth = 1;
        if (currentHeight < 1)
            currentHeight = 1;

        ++uploadedMipCount;
    }

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, mipMapCount > 1 ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    free(buffer);

    Width = static_cast<int>(width);
    Height = static_cast<int>(height);
    Channels = 4;
    bCompressed = true;

    ResourceLogger::AddLog({"Texture", "Success", path,
                            "DDS GPU 업로드 성공 (" + formatName + ", size=" + std::to_string(width) + "x" +
                              std::to_string(height) + ", mipmaps=" + std::to_string(uploadedMipCount) + ")"});

    return true;
}

void Texture::Bind(int unit) const {
    if (ID == 0) {
        ResourceLogger::AddLog({"Texture", "Warning", LoadedPath, "Bind 호출 실패: texture ID가 0임"});
        return;
    }

    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, ID);
}

void Texture::Unbind() const { glBindTexture(GL_TEXTURE_2D, 0); }

void Texture::Release() {
    if (ID != 0) {
        glDeleteTextures(1, &ID);
        ID = 0;
    }

    Width = 0;
    Height = 0;
    Channels = 0;
    bCompressed = false;
    LoadedPath.clear();
}

std::string Texture::ToLower(std::string str) const {
    std::transform(str.begin(), str.end(), str.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return str;
}

std::string Texture::GetExtension(const std::string& path) const {
    fs::path p(path);
    return ToLower(p.extension().string());
}

// ----------------------------------
// Model
// ----------------------------------
Model::Model(const char* modelPath, bool bFlipV) { Load(modelPath, bFlipV); }

void Model::Clear() {
    Positions.clear();
    Texcoords.clear();
    Normals.clear();
    LoadedPath.clear();
    bHasTexcoord = false;
    bHasNormal = false;
}

size_t Model::GetVertexCount() const { return Positions.size() / 3; }

bool Model::Load(const char* modelPath, bool bFlipV) {
    Clear();

    if (modelPath == nullptr) {
        ResourceLogger::AddLog({"Model", "Failed", "(null)", "입력 경로가 nullptr임"});
        return false;
    }

    PathResolveResult pathResult = PathResolve(modelPath);
    if (!pathResult.success) {
        ResourceLogger::AddLog({"Model", "Failed", modelPath, "Model 경로 확인 실패: " + pathResult.message});
        return false;
    }

    LoadedPath = pathResult.resolvedPath;
    ResourceLogger::AddLog({"Model", "Success", LoadedPath, "경로 확인 성공"});

    return LoadOBJ(LoadedPath, bFlipV);
}

bool Model::LoadOBJ(const std::string& path, bool bFlipV) {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn;
    std::string err;

    std::string baseDir;
    try {
        baseDir = fs::path(path).parent_path().string() + "/";
    } catch (...) {
        baseDir = "";
    }

    bool ok = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.c_str(), baseDir.c_str());

    if (!warn.empty()) {
        ResourceLogger::AddLog({"Model", "Warning", path, warn});
    }

    if (!ok) {
        ResourceLogger::AddLog({"Model", "Failed", path, "OBJ 파싱 실패: " + err});
        return false;
    }

    if (attrib.vertices.empty()) {
        ResourceLogger::AddLog({"Model", "Failed", path, "정점 데이터가 비어 있음"});
        return false;
    }

    for (const auto& shape : shapes) {
        size_t indexOffset = 0;

        for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); ++f) {
            int fv = shape.mesh.num_face_vertices[f];

            // 우선 삼각형만 처리
            if (fv != 3) {
                ResourceLogger::AddLog(
                  {"Model", "Warning", path,
                   "삼각형이 아닌 face 발견 - 현재는 삼각형만 지원 (face vertex count=" + std::to_string(fv) + ")"});

                indexOffset += fv;
                continue;
            }

            for (size_t v = 0; v < 3; ++v) {
                tinyobj::index_t idx = shape.mesh.indices[indexOffset + v];

                // position
                if (idx.vertex_index < 0) {
                    ResourceLogger::AddLog({"Model", "Failed", path, "vertex index가 음수임"});
                    return false;
                }

                Positions.push_back(attrib.vertices[3 * idx.vertex_index + 0]);
                Positions.push_back(attrib.vertices[3 * idx.vertex_index + 1]);
                Positions.push_back(attrib.vertices[3 * idx.vertex_index + 2]);

                // texcoord
                if (idx.texcoord_index >= 0 && !attrib.texcoords.empty()) {
                    float u = attrib.texcoords[2 * idx.texcoord_index + 0];
                    float vCoord = attrib.texcoords[2 * idx.texcoord_index + 1];

                    if (bFlipV) {
                        vCoord = 1.0f - vCoord;
                    }

                    Texcoords.push_back(u);
                    Texcoords.push_back(vCoord);
                    bHasTexcoord = true;
                } else {
                    Texcoords.push_back(0.0f);
                    Texcoords.push_back(0.0f);
                }

                // normal
                if (idx.normal_index >= 0 && !attrib.normals.empty()) {
                    Normals.push_back(attrib.normals[3 * idx.normal_index + 0]);
                    Normals.push_back(attrib.normals[3 * idx.normal_index + 1]);
                    Normals.push_back(attrib.normals[3 * idx.normal_index + 2]);
                    bHasNormal = true;
                } else {
                    Normals.push_back(0.0f);
                    Normals.push_back(0.0f);
                    Normals.push_back(0.0f);
                }
            }

            indexOffset += fv;
        }
    }

    ResourceLogger::AddLog(
      {"Model", "Success", path,
       "OBJ 파싱 성공 (shapes=" + std::to_string(shapes.size()) + ", materials=" + std::to_string(materials.size()) +
         ", vertices=" + std::to_string(GetVertexCount()) + ", texcoords=" + std::string(bHasTexcoord ? "yes" : "no") +
         ", normals=" + std::string(bHasNormal ? "yes" : "no") + ")"});

    return true;
}
// #include <iostream>
// #include <string>
// #include <fstream>
// #include <sstream>
// #include <vector>
// #include <filesystem>
//
// namespace fs = std::filesystem;
//
// struct LogEntry {
//     std::string type;    // Shader / Texture / Model
//     std::string status;  // Success / Failed
//     std::string path;    // 파일 경로
//     std::string message; // 상세 내용
// };
//
// struct PathResolveResult {
//     bool success = false;
//     std::string originalPath;
//     std::string resolvedPath;
//     std::string message;
// };
// PathResolveResult PathResolve(const std::string& inputPath) {
//     PathResolveResult result;
//     result.originalPath = inputPath;
//
//     fs::path input(inputPath);
//
//     // 1. 절대 경로
//     if (input.is_absolute()) {
//         if (fs::exists(input)) {
//             result.success = true;
//             result.resolvedPath = fs::absolute(input).string();
//             result.message = "절대 경로 확인 성공";
//             return result;
//         }
//         result.message = "절대 경로 파일 없음";
//         return result;
//     }
//
//     fs::path fsPath = fs::current_path() / "../../../" / input;
//     if (fs::exists(fsPath)) {
//         result.success = true;
//         result.resolvedPath = fs::absolute(fsPath).string();
//         result.message = "FileSystem 방식 경로 성공";
//         return result;
//     }
//
//     // 3. 일반 후보들
//     std::vector<fs::path> candidates = {fs::current_path() / input,
//                                         fs::current_path() / "resources" / "textures" / input,
//                                         fs::current_path() / "textures" / input,
//                                         fs::current_path() / "assets" / "textures" / input,
//                                         fs::current_path() / "shaders" / input,
//                                         fs::current_path() / "assets" / "shaders" / input};
//
//     for (const auto& candidate : candidates) {
//         if (fs::exists(candidate)) {
//             result.success = true;
//             result.resolvedPath = fs::absolute(candidate).string();
//             result.message = "일반 경로 탐색 성공";
//             return result;
//         }
//     }
//
//     result.message = "파일을 찾지 못함";
//     return result;
// }
// class Shader {
// public:
//     unsigned int ID = 0;
//
//     // 로그 저장소
//     static std::vector<LogEntry> Logs;
//
//     Shader(const char* vertexPath, const char* fragmentPath) {
//         // 1. 경로 확인
//         PathResolveResult vsPathResult = PathResolve(vertexPath);
//         if (!vsPathResult.success) {
//             AddLog({"Shader", "Failed", vertexPath, "Vertex shader 경로 확인 실패: " + vsPathResult.message});
//             return;
//         }
//         AddLog({"Shader", "Success", vsPathResult.resolvedPath, "Vertex shader 경로 확인 성공"});
//
//         PathResolveResult fsPathResult = PathResolve(fragmentPath);
//         if (!fsPathResult.success) {
//             AddLog({"Shader", "Failed", fragmentPath, "Fragment shader 경로 확인 실패: " + fsPathResult.message});
//             return;
//         }
//         AddLog({"Shader", "Success", fsPathResult.resolvedPath, "Fragment shader 경로 확인 성공"});
//
//         // 2. 파일 읽기
//         std::string vertexCode;
//         std::string fragmentCode;
//
//         if (!ReadFileToString(vsPathResult.resolvedPath, vertexCode)) {
//             AddLog({"Shader", "Failed", vsPathResult.resolvedPath, "Vertex shader 파일 읽기 실패"});
//             return;
//         }
//         AddLog({"Shader", "Success", vsPathResult.resolvedPath, "Vertex shader 파일 읽기 성공"});
//
//         if (!ReadFileToString(fsPathResult.resolvedPath, fragmentCode)) {
//             AddLog({"Shader", "Failed", fsPathResult.resolvedPath, "Fragment shader 파일 읽기 실패"});
//             return;
//         }
//         AddLog({"Shader", "Success", fsPathResult.resolvedPath, "Fragment shader 파일 읽기 성공"});
//
//         if (vertexCode.empty()) {
//             AddLog({"Shader", "Failed", vsPathResult.resolvedPath, "Vertex shader 소스가 비어 있음"});
//             return;
//         }
//
//         if (fragmentCode.empty()) {
//             AddLog({"Shader", "Failed", fsPathResult.resolvedPath, "Fragment shader 소스가 비어 있음"});
//             return;
//         }
//
//         // 3. 컴파일
//         GLuint vertex = CompileShader(GL_VERTEX_SHADER, vertexCode, vsPathResult.resolvedPath, "Vertex shader");
//         if (vertex == 0)
//             return;
//
//         GLuint fragment = CompileShader(GL_FRAGMENT_SHADER, fragmentCode, fsPathResult.resolvedPath, "Fragment
//         shader"); if (fragment == 0) {
//             glDeleteShader(vertex);
//             return;
//         }
//
//         // 4. 링크
//         GLuint program = glCreateProgram();
//         glAttachShader(program, vertex);
//         glAttachShader(program, fragment);
//         glLinkProgram(program);
//
//         GLint success = GL_FALSE;
//         glGetProgramiv(program, GL_LINK_STATUS, &success);
//
//         if (success != GL_TRUE) {
//             char infoLog[1024];
//             glGetProgramInfoLog(program, 1024, NULL, infoLog);
//
//             AddLog({"Shader", "Failed", vsPathResult.resolvedPath + " + " + fsPathResult.resolvedPath,
//                     "프로그램 링크 실패: " + std::string(infoLog)});
//
//             glDeleteShader(vertex);
//             glDeleteShader(fragment);
//             glDeleteProgram(program);
//             return;
//         }
//
//         AddLog(
//           {"Shader", "Success", vsPathResult.resolvedPath + " + " + fsPathResult.resolvedPath, "프로그램 링크
//           성공"});
//
//         // 5. 링크 끝났으니 shader object 삭제
//         glDeleteShader(vertex);
//         glDeleteShader(fragment);
//
//         ID = program;
//     }
//
//     void use() {
//         if (ID != 0)
//             glUseProgram(ID);
//     }
//
//     void setBool(const std::string& name, bool value) const {
//         glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value);
//     }
//
//     void setInt(const std::string& name, int value) const {
//         glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
//     }
//
//     void setFloat(const std::string& name, float value) const {
//         glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
//     }
//
//     static void PrintLogs() {
//         for (const auto& log : Logs) {
//             std::cout << "[" << log.type << "] "
//                       << "[" << log.status << "] "
//                       << "[" << log.path << "] " << log.message << "\n";
//         }
//     }
//
//     static void ClearLogs() { Logs.clear(); }
//
// private:
//     static void AddLog(const LogEntry& entry) { Logs.push_back(entry); }
//
//     static bool ReadFileToString(const std::string& path, std::string& outText) {
//         std::ifstream file(path);
//         if (!file.is_open())
//             return false;
//
//         std::stringstream ss;
//         ss << file.rdbuf();
//         outText = ss.str();
//         return true;
//     }
//
//     static GLuint CompileShader(GLenum shaderType, const std::string& source, const std::string& path,
//                                 const std::string& shaderLabel) {
//         GLuint shader = glCreateShader(shaderType);
//         const char* src = source.c_str();
//
//         glShaderSource(shader, 1, &src, NULL);
//         glCompileShader(shader);
//
//         GLint success = GL_FALSE;
//         glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
//
//         if (success == GL_TRUE) {
//             AddLog({"Shader", "Success", path, shaderLabel + " 컴파일 성공"});
//             return shader;
//         }
//
//         char infoLog[1024];
//         glGetShaderInfoLog(shader, 1024, NULL, infoLog);
//
//         AddLog({"Shader", "Failed", path, shaderLabel + " 컴파일 실패: " + std::string(infoLog)});
//
//         glDeleteShader(shader);
//         return 0;
//     }
// };
//
//// static 멤버 정의
// std::vector<LogEntry> Shader::Logs;
//
// class Texture {
// public:
//     unsigned int ID = 0;
//
//     Texture(const char* texturePath) {
//         // 1. 경로 확인
//         PathResolveResult pathResult = PathResolve(texturePath);
//         if (!pathResult.success) {
//             Shader::Logs.push_back({"Texture", "Failed", texturePath, "Texture 경로 확인 실패: " +
//             pathResult.message}); return;
//         }
//
//         Shader::Logs.push_back({"Texture", "Success", pathResult.resolvedPath, "Texture 경로 확인 성공"});
//
//         // 2. 이미지 로딩
//         int width, height, channels;
//         stbi_set_flip_vertically_on_load(true);
//
//         unsigned char* data = stbi_load(pathResult.resolvedPath.c_str(), &width, &height, &channels, 0);
//
//         if (!data) {
//             Shader::Logs.push_back({"Texture", "Failed", pathResult.resolvedPath, "이미지 로딩 실패"});
//             return;
//         }
//
//         Shader::Logs.push_back({"Texture", "Success", pathResult.resolvedPath,
//                                 "이미지 로딩 성공 (" + std::to_string(width) + "x" + std::to_string(height) +
//                                   ", channels=" + std::to_string(channels) + ")"});
//
//         // 3. 포맷 결정
//         GLenum format;
//         if (channels == 1)
//             format = GL_RED;
//         else if (channels == 3)
//             format = GL_RGB;
//         else if (channels == 4)
//             format = GL_RGBA;
//         else {
//             Shader::Logs.push_back({"Texture", "Failed", pathResult.resolvedPath, "지원하지 않는 채널 수"});
//             stbi_image_free(data);
//             return;
//         }
//
//         // 4. OpenGL 텍스처 생성
//         glGenTextures(1, &ID);
//         glBindTexture(GL_TEXTURE_2D, ID);
//
//         // 옵션
//         glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
//         glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
//         glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
//         glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//
//         // 업로드
//         glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
//         glGenerateMipmap(GL_TEXTURE_2D);
//
//         Shader::Logs.push_back({"Texture", "Success", pathResult.resolvedPath, "GPU 업로드 성공"});
//
//         stbi_image_free(data);
//         glBindTexture(GL_TEXTURE_2D, 0);
//     }
//
//     void Bind(int unit = 0) {
//         glActiveTexture(GL_TEXTURE0 + unit);
//         glBindTexture(GL_TEXTURE_2D, ID);
//     }
// };
