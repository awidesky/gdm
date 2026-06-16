#ifndef GLUTIL_DEBUG_SNAPSHOT_HPP
#define GLUTIL_DEBUG_SNAPSHOT_HPP

#if GDM_DEBUG
#include <memory>
#include <set>
#include <string>
#endif
#include <filesystem>
#include <iostream>

#include <glutil/gl.hpp>

namespace glutil::debug {

#if GDM_DEBUG
class SnapshotSink;
struct SnapshotAsyncState;

/**
 * Async handle for snapshot output worker thread.
 * Wraps shared state controlling background queue processing.
 */
struct SnapshotAsyncHandle {
    SnapshotAsyncHandle() = default;
    explicit SnapshotAsyncHandle(std::shared_ptr<SnapshotAsyncState> state);
    ~SnapshotAsyncHandle();

    /** Block until snapshot worker thread finishes processing all queued output */
    void wait() const;
    /** Check whether async snapshot output has completed */
    bool finished() const;
    explicit operator bool() const { return static_cast<bool>(m_state); }

private:
    std::shared_ptr<SnapshotAsyncState> m_state;
};
#else // simple empty structs for release build
/** Dummy state for release build */
struct SnapshotAsyncState {};
/** No-op async handle in release builds */
struct SnapshotAsyncHandle {
    SnapshotAsyncHandle() noexcept = default;
    void wait() const noexcept {}
    bool finished() const noexcept { return true; }
    constexpr explicit operator bool() const noexcept { return false; }
};
#endif

/**
 * Main OpenGL snapshot capture controller.
 *
 * Captures current GL state including:
 * - Framebuffer status
 * - Shader program state + uniforms
 * - Texture bindings
 * - VAO/VBO/EBO layout and data
 * - Renderer pipeline state
 * - GL object binding state
 *
 * Supports synchronous and async streaming output.
 */
class Snapshot {
public:
    /**
     * Create snapshot configuration.
     * @param printAll default enable flag for most categories
     */
    explicit Snapshot(bool printAll = true)
        : m_bufferIncludeUnbound(false), m_alreadyCaptured(false), m_shaderStatus(printAll), m_shaderUniform(printAll),
          m_textureInfo(printAll), m_textureIncludeSampler(printAll), m_bufferVAOInfo(printAll),
          m_bufferIncludeDisabled(false), m_allVBOInfo(printAll), m_bufferIncludeData(false), m_rendererState(printAll),
          m_framebufferInfo(printAll), m_boundInfo(printAll), m_Once(true) {}

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
    /** Capture snapshot and stream output to ostream (async optional) */
    SnapshotAsyncHandle capture(std::ostream& out = std::cerr, bool printAsync = true) const;
    /**
     * Capture snapshot into file directory
     * @param dumpVertexData export raw VBO/VAO data into separate files
     */
    SnapshotAsyncHandle capture(const std::filesystem::path& dir, bool dumpVertexData = false,
                                bool printAsync = false) const;
private:
    /** Internal snapshot orchestration entry */
    void captureInternal(SnapshotSink& out) const;
    /** Framebuffer status + attachments */
    void captureFramebuffer(SnapshotSink& out) const;
    /** Shader program link status + info log */
    void captureShaderStatus(SnapshotSink& out) const;
    /** Active uniform values of current program */
    void captureShaderUniforms(SnapshotSink& out) const;
    /** Texture unit + binding inspection */
    void captureTextureInfo(SnapshotSink& out) const;
    /** VAO + VBO + EBO layout inspection */
    void captureBufferVAOInfo(SnapshotSink& out) const;
    /** Global VBO inspection independent of VAO */
    void captureAllVBOInfo(SnapshotSink& out) const;
    /** Render pipeline state (depth/blend/cull/etc.) */
    void captureRendererState(SnapshotSink& out) const;
    /** GL object binding state */
    void captureBoundInfo(SnapshotSink& out) const;
    /** Dump raw buffer binary data into files */
    void saveBufferInfoToFile(const std::filesystem::path& dir) const;
    /** Write VAO layout + vertex stream to file */
    void saveVAOInfoToFile(SnapshotSink& out, GLuint vaoId) const;


    mutable SnapshotAsyncHandle m_lastAsyncHandle;

    /** Include VAOs not currently bound in capture */
    mutable bool m_bufferIncludeUnbound;
    /** Prevent repeated capture in single-frame mode */
    mutable bool m_alreadyCaptured = false;
#else
    /** No-op capture (ostream) in release build */
    inline SnapshotAsyncHandle capture(std::ostream& out = std::cerr, bool printAsync = true) const {
        (void)out;
        (void)printAsync;
        return {};
    }
    /** No-op capture (file output) in release build */
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
    /** Shader program link / validation status output toggle */
    bool m_shaderStatus;
    /** Active uniform dump toggle */
    bool m_shaderUniform;
    /** Texture binding inspection toggle */
    bool m_textureInfo;
    /** Include sampler parameters (filter/wrap/compare) */
    bool m_textureIncludeSampler;
    /** VAO/VBO/EBO inspection toggle */
    bool m_bufferVAOInfo;
    /** Include disabled vertex attributes */
    bool m_bufferIncludeDisabled;
    /** Global VBO inspection toggle */
    bool m_allVBOInfo;
    /** Include raw buffer data dump */
    bool m_bufferIncludeData; // share bufferVAOInfo, allVBOInfo
    /** Renderer state dump toggle */
    bool m_rendererState;
    /** Framebuffer inspection toggle */
    bool m_framebufferInfo;
    /** GL binding state dump toggle */
    bool m_boundInfo;
    /** Timing measurement enable flag */
    bool m_enableTiming = true;
    /** Execute snapshot only once (unless overridden) */
    bool m_Once = false;
};

} // namespace glutil::debug
#endif // GLUTIL_SNAPSHOT_HPP