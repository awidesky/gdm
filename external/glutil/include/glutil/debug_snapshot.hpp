#ifndef GLUTIL_DEBUG_SNAPSHOT_HPP
#define GLUTIL_DEBUG_SNAPSHOT_HPP

#if GDM_DEBUG
#include <string>
#include <memory>
#include <set>
#endif
#include <iostream>
#include <filesystem>

#include <glutil/gl.hpp>

namespace glutil::debug {

#if GDM_DEBUG
class SnapshotSink;
struct SnapshotAsyncState;

struct SnapshotAsyncHandle {
    SnapshotAsyncHandle() = default;
    explicit SnapshotAsyncHandle(std::shared_ptr<SnapshotAsyncState> state);
    ~SnapshotAsyncHandle();

    void wait() const;
    bool finished() const;
    explicit operator bool() const { return static_cast<bool>(m_state); }

private:
    std::shared_ptr<SnapshotAsyncState> m_state;
};
#else // simple empty structs for release build
struct SnapshotAsyncState {};
struct SnapshotAsyncHandle {
    SnapshotAsyncHandle() noexcept = default;
    void wait() const noexcept {}
    bool finished() const noexcept { return true; }
    constexpr explicit operator bool() const noexcept { return false; }
};
#endif

class Snapshot {
public:
    explicit Snapshot(bool printAll = true)
        : m_shaderStatus(printAll), m_shaderUniform(printAll), m_textureInfo(printAll),
          m_textureIncludeSampler(printAll), m_bufferVAOInfo(printAll), m_bufferIncludeUnbound(false),
          m_bufferIncludeDisabled(false), m_allVBOInfo(printAll), m_bufferIncludeData(false), m_rendererState(printAll),
          m_framebufferInfo(printAll), m_boundInfo(printAll), m_Once(true), m_alreadyCaptured(false) {}

    /// @brief Set whether to output shader program link status and info log
    Snapshot& shaderStatus(bool v) {
        m_shaderStatus = v;
        return *this;
    }
    /// @brief Set whether to output all active uniform values of the currently bound shader
    Snapshot& shaderUniform(bool v) {
        m_shaderUniform = v;
        return *this;
    }

    /// @brief Set whether to output texture binding information for all active texture units
    /// @param v true to enable output
    /// @param includeSampler true to also output sampler parameters (MIN/MAG filter, WRAP mode, etc.)
    Snapshot& textureInfo(bool v, bool includeSampler = false) {
        m_textureInfo = v;
        m_textureIncludeSampler = includeSampler;
        return *this;
    }

    /// @brief Set whether to output VAO/VBO/EBO information
    /// @param v true to enable output
    /// @param includeData true to output raw vertex/index data (may be slow)
    /// @param includeUnbound true to also output VAOs not currently bound
    /// @param includeDisabled true to also output attribs not enabled
    Snapshot& bufferVAOInfo(bool v, bool includeData = false, bool includeUnbound = false,
                            bool includeDisabled = false) {
        m_bufferVAOInfo = v;
        m_bufferIncludeUnbound = includeUnbound;
        m_bufferIncludeDisabled = includeDisabled;
        m_bufferIncludeData = includeData;
        return *this;
    }

    /// @brief Set whether to output all VBO info regardless of VAO association
    /// @param includeData true to output raw buffer data (may be slow)
    Snapshot& allVBOInfo(bool v) {
        m_allVBOInfo = v;
        return *this;
    }

    /// @brief Set whether to output renderer state (viewport, depth test, blend, cull face, etc.)
    Snapshot& rendererState(bool v) {
        m_rendererState = v;
        return *this;
    }

    /// @brief Set whether to output framebuffer status and attachment info
    Snapshot& framebufferInfo(bool v) {
        m_framebufferInfo = v;
        return *this;
    }

    /// @brief Set whether to output current GL binding state
    /// (GL_ARRAY_BUFFER_BINDING, GL_CURRENT_PROGRAM, GL_FRAMEBUFFER_BINDING, etc.)
    Snapshot& boundInfo(bool v) {
        m_boundInfo = v;
        return *this;
    }

    /// @brief Set whether to print a snapshot on every tracked GL call (via GLAD post callback)
    /// @param v true to enable per-call snapshot output (warning: extremely verbose)
    Snapshot& printPerCall(bool v) {
        m_Once = !v;
        return *this;
    }

    /// @brief Set whether to print elapsed time while taking snapshot.
    /// @param v true to print elapsed time ofsnapshot.
    Snapshot& enableTiming(bool v) {
        m_enableTiming = v;
        return *this;
    }

#if GDM_DEBUG
    SnapshotAsyncHandle capture(std::ostream& out = std::cerr, bool printAsync = true) const;
    SnapshotAsyncHandle capture(const std::filesystem::path& dir, bool dumpVertexData = false,
                                bool printAsync = false) const;
private:
    void captureInternal(SnapshotSink& out) const;
    void captureFramebuffer(SnapshotSink& out) const;
    void captureShaderStatus(SnapshotSink& out) const;
    void captureShaderUniforms(SnapshotSink& out) const;
    void captureTextureInfo(SnapshotSink& out) const;
    void captureBufferVAOInfo(SnapshotSink& out) const;
    void captureAllVBOInfo(SnapshotSink& out) const;
    void captureRendererState(SnapshotSink& out) const;
    void captureBoundInfo(SnapshotSink& out) const;
    void saveBufferInfoToFile(const std::filesystem::path& dir) const;
    void saveVAOInfoToFile(SnapshotSink& out, GLuint vaoId) const;


    mutable SnapshotAsyncHandle m_lastAsyncHandle;

    mutable bool m_bufferIncludeUnbound;
    mutable bool m_alreadyCaptured = false;
#else
    inline SnapshotAsyncHandle capture(std::ostream& out = std::cerr, bool printAsync = true) const {
        (void)out;
        (void)printAsync;
        return {};
    }
    inline SnapshotAsyncHandle capture(const std::filesystem::path& dir, bool dumpVertexData = false,
                                       bool printAsync = false) const {
        (void)dir;
        (void)dumpVertexData;
        (void)printAsync;
        return {};
    }

private:
    bool m_bufferIncludeUnbound;
    bool m_alreadyCaptured = false;
#endif
private:
    bool m_shaderStatus;
    bool m_shaderUniform;
    bool m_textureInfo;
    bool m_textureIncludeSampler;
    bool m_bufferVAOInfo;
    bool m_bufferIncludeDisabled;
    bool m_allVBOInfo;
    bool m_bufferIncludeData; // bufferVAOInfo, allVBOInfo 공유
    bool m_rendererState;
    bool m_framebufferInfo;
    bool m_boundInfo;
    bool m_enableTiming = true;
    bool m_Once = false;
};

} // namespace glutil::debug
#endif // GLUTIL_SNAPSHOT_HPP