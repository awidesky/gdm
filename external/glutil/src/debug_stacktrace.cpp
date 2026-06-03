#include <glutil/gl.hpp>
#include <glutil/debug_stacktrace.hpp>

#ifdef GLAD_OPTION_GL_DEBUG
#include <cpptrace/cpptrace.hpp>
#include <cpptrace/formatting.hpp>

#include <iostream>
#include <sstream>
#include <regex>


namespace glutil::debug {
void printStackTrace(std::string header, int skip, int depth, bool snippets, int snippet_context) {
    cpptrace::formatter{}
      .header(header)                                     // header
      .colors(cpptrace::formatter::color_mode::automatic) // Color: always / none / automatic
      .addresses(cpptrace::formatter::address_mode::raw)  // Adress: raw / object / none
      .paths(cpptrace::formatter::path_mode::basename)    // Path: full / basename
      .snippets(snippets)                                 // source snippet 
      .snippet_context(snippet_context)                   // How many lines of source context to show in a snippet	
      .columns(true)                                      // column number enable
      .print(std::cerr, cpptrace::stacktrace::current(skip, depth));
}

std::string getCalledGLfunctionName(int skip) {
    std::stringstream ss;
    cpptrace::formatter{}
      .header("")
      .colors(cpptrace::formatter::color_mode::none)
      .addresses(cpptrace::formatter::address_mode::raw)
      .paths(cpptrace::formatter::path_mode::basename)
      .snippets(true)               // source snippet
      .snippet_context(0) // How many lines of source context to show in a snippet	
      .columns(true)
      .print(ss, cpptrace::stacktrace::current(skip, 1));
    std::string out = ss.str();
    // content of out:
    //# 0 0x00007ff6d850ef6a in glutil::ShaderLoader::loadShaderToGL(std::filesystem::path &) at shader.cpp : 207
    //    > 207 : GLuint shader = glCreateShader(type);

    const std::regex rgx(R"(at ([^:]+):([0-9]+)[\s\S]*?>\s*[0-9]+:\s*(.*))");
    std::smatch m;
    if (std::regex_search(out, m, rgx)) {
        std::string file = m[1];
        std::string line = m[2];
        std::string code = m[3];

        return file + ":" + line + " -> " + code;
    }

    return out;
}
} // namespace glutil::debug
#else
namespace glutil::debug {
void printStackTrace(std::string header, int skip, int depth, bool snippets, int snippet_context) {}
std::string getCalledGLfunctionName(int skip) { return ""; }
} // namespace glutil::debug
#endif