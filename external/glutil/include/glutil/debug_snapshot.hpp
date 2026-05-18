#ifndef GLUTIL_DEBUG_SNAPSHOT_HPP
#define GLUTIL_DEBUG_SNAPSHOT_HPP

#include <string>
#include <iostream>
#include <set>


namespace glutil::debug {

class snapshot {
public:
    explicit snapshot(bool printAll = true);

    snapshot& shaderStatus(bool v);
    snapshot& shaderUniform(bool v);
    snapshot& textureInfo(bool v, bool includeSampler = false);
    snapshot& bufferVAOInfo(bool v, bool includeUnbound = false, bool includeDisabled = false,
                            bool includeData = false);
    snapshot& allVBOInfo(bool v, bool includeData = false);
    snapshot& rendererState(bool v);
    snapshot& framebufferInfo(bool v);
    snapshot& boundInfo(bool v);
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

snapshot snapshotOnError();
snapshot snapshotShaderDebug();
snapshot snapshotBufferDebug();

// TODO :  정의 예정
std::set<GLuint> getAllVBO();

} // namespace glutil::debug
#endif // GLUTIL_SNAPSHOT_HPP