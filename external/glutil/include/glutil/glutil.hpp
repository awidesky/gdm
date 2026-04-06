#ifndef GLUTIL_GLUTIL
#define GLUTIL_GLUTIL
#pragma once

#include <string>

// namespace glutil {
struct LogEntry {
    std::string type;    // Shader / Texture
    std::string status;  // Success / Failed / Warning
    std::string path;    // 파일 경로
    std::string message; // 상세 내용
};

struct PathResolveResult {
    bool success = false;
    std::string originalPath;
    std::string resolvedPath;
    std::string message;
};

PathResolveResult PathResolve(const std::string& inputPath);

class ResourceLogger {
public:
    static void AddLog(const LogEntry& entry);
    static void PrintLogs();
    static void ClearLogs();
    static const std::vector<LogEntry>& GetLogs();

private:
    static std::vector<LogEntry> Logs;
};

class Shader {
public:
    unsigned int ID = 0;

public:
    Shader() = default;
    Shader(const char* vertexPath, const char* fragmentPath);

    bool Load(const char* vertexPath, const char* fragmentPath);
    void Use() const;
    void Release();

    void SetBool(const std::string& name, bool value) const;
    void SetInt(const std::string& name, int value) const;
    void SetFloat(const std::string& name, float value) const;

private:
    static bool ReadFileToString(const std::string& path, std::string& outText);
    static GLuint CompileShader(GLenum shaderType, const std::string& source, const std::string& path,
                                const std::string& shaderLabel);
};

class Texture {
public:
    unsigned int ID = 0;
    int Width = 0;
    int Height = 0;
    int Channels = 0;
    bool bCompressed = false;
    std::string LoadedPath;

public:
    Texture() = default;
    explicit Texture(const char* texturePath, bool bFlipVertically = true);

    bool Load(const char* texturePath, bool bFlipVertically = true);
    void Bind(int unit = 0) const;
    void Unbind() const;
    void Release();

private:
    bool LoadStandardImage(const std::string& path, bool bFlipVertically);
    bool LoadDDS(const std::string& path);
    bool CreateTexture2DFromRaw(unsigned char* data, int width, int height, int channels, const std::string& path);
    std::string ToLower(std::string str) const;
    std::string GetExtension(const std::string& path) const;
};
//}

class Model {
public:
    std::vector<float> Positions; // x,y,z 반복
    std::vector<float> Texcoords; // u,v 반복
    std::vector<float> Normals;   // x,y,z 반복

    std::string LoadedPath;
    bool bHasTexcoord = false;
    bool bHasNormal = false;

public:
    Model() = default;
    explicit Model(const char* modelPath, bool bFlipV = true);

    bool Load(const char* modelPath, bool bFlipV = true);
    void Clear();

    size_t GetVertexCount() const;

private:
    bool LoadOBJ(const std::string& path, bool bFlipV);
};
#endif // GLUTIL_GLUTIL
