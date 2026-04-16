#ifndef GLUTIL_GLUTIL
#define GLUTIL_GLUTIL

#include <glutil/gl.hpp>
#include <glutil/inspector.hpp>
#include <glutil/shader.hpp>
#include <glutil/logging.hpp>

#include <vector>
#include <string>

namespace glutil {

struct LogEntry {
    std::string type;    // Shader / Texture
    std::string status;  // Success / Failed / Warning
    std::string path;    // 파일 경로
    std::string message; // 상세 내용
};

struct PathResolveResult { // TODO : fix. move to own header. LogEntry too
    bool success = false;
    std::string originalPath;
    std::string resolvedPath;
    std::string message;
};

PathResolveResult pathResolve(const std::string& inputPath);

class ResourceLogger {
public:
    static void addLog(const LogEntry& entry);
    static void printLogs();
    static void clearLogs();
    static const std::vector<LogEntry>& getLogs();

private:
    static std::vector<LogEntry> logs;
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

    bool load(const char* texturePath, bool bFlipVertically = true);
    void bind(int unit = 0) const;
    void unbind() const;
    void release();

private:
    bool loadStandardImage(const std::string& path, bool bFlipVertically);
    bool loadDDS(const std::string& path);
    bool createTexture2DFromRaw(unsigned char* data, int width, int height, int channels, const std::string& path);
    std::string toLower(std::string str) const;
    std::string getExtension(const std::string& path) const;
};

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

    bool load(const char* modelPath, bool bFlipV = true);
    void clear();

    size_t getVertexCount() const;

private:
    bool loadOBJ(const std::string& path, bool bFlipV);
};

} // namespace glutil

#endif // GLUTIL_GLUTIL
