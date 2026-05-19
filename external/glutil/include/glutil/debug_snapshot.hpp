#ifndef GLUTIL_DEBUG_SNAPSHOT_HPP
#define GLUTIL_DEBUG_SNAPSHOT_HPP

#include <string>
#include <iostream>
#include <set>


namespace glutil::debug {

// TODO : snapshot --> Snapshot
class snapshot {
public:
    explicit snapshot(bool printAll = true);

    /// @brief Set whether to output shader program link status and info log
    snapshot& shaderStatus(bool v);
    /// @brief Set whether to output all active uniform values of the currently bound shader
    snapshot& shaderUniform(bool v);

    /// @brief Set whether to output texture binding information for all active texture units
    /// @param v true to enable output
    /// @param includeSampler true to also output sampler parameters (MIN/MAG filter, WRAP mode, etc.)
    snapshot& textureInfo(bool v, bool includeSampler = false);

    /// @brief Set whether to output VAO/VBO/EBO information
    /// @param v true to enable output
    /// @param includeData true to output raw vertex/index data (may be slow)
    /// @param includeUnbound true to also output VAOs not currently bound 
    /// @param includeDisabled true to also output attribs not enabled 
    snapshot& bufferVAOInfo(bool v, bool includeData = false, bool includeDisabled = false,
                            bool includeUnbound = false);

    /// @brief Set whether to output all VBO info regardless of VAO association
    /// @param includeData true to output raw buffer data (may be slow)
    snapshot& allVBOInfo(bool v, bool includeData = false);

    /// @brief Set whether to output renderer state (viewport, depth test, blend, cull face, etc.)
    snapshot& rendererState(bool v);

    /// @brief Set whether to output framebuffer status and attachment info
    snapshot& framebufferInfo(bool v);

    /// @brief Set whether to output current GL binding state
    /// (GL_ARRAY_BUFFER_BINDING, GL_CURRENT_PROGRAM, GL_FRAMEBUFFER_BINDING, etc.)
    snapshot& boundInfo(bool v);

    /// @brief Set whether to print a snapshot on every tracked GL call (via GLAD post callback)
    /// @param v true to enable per-call snapshot output (warning: extremely verbose)
    snapshot& printPerCall(bool v);

    void capture(std::ostream& out = std::cerr) const;

private:
    void captureFramebuffer(std::ostream& out) const;
    void captureShaderStatus(std::ostream& out) const;
    void captureShaderUniforms(std::ostream& out) const;
    void captureTextureInfo(std::ostream& out) const;
    void captureBufferVAOInfo(std::ostream& out) const;
    void captureAllVBOInfo(std::ostream& out) const;
    void captureRendererState(std::ostream& out) const;
    void captureBoundInfo(std::ostream& out) const;

    bool m_shaderStatus;
    bool m_shaderUniform;
    bool m_textureInfo;
    bool m_textureIncludeSampler;
    bool m_bufferVAOInfo;
    bool m_bufferIncludeUnbound;
    bool m_bufferIncludeDisabled;
    bool m_allVBOInfo;
    bool m_bufferIncludeData; // bufferVAOInfo, allVBOInfo 공유
    bool m_rendererState;
    bool m_framebufferInfo;
    bool m_boundInfo;
    bool m_Once = false;
    mutable bool m_flag = false;
};


} // namespace glutil::debug
#endif // GLUTIL_SNAPSHOT_HPP