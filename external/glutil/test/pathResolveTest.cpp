#include <filesystem>
#include <iostream>

#include <glutil/path.hpp>

namespace fs = std::filesystem;

static std::string findExistingFile(const fs::path& dir) {
    std::error_code ec;
    if (!fs::exists(dir, ec) || !fs::is_directory(dir, ec))
        return {};

    for (const auto& entry : fs::directory_iterator(dir, ec)) {
        if (ec) return {};
        if (entry.is_regular_file(ec)) return entry.path().lexically_relative(dir).string();
    }
    return {};
}

static void printResult(const std::string& title, const std::string& testPath) {
    const glutil::PathResolveResult r = glutil::pathResolve(testPath);

    std::cout << "==============================\n"
              << title << '\n'
              << "success       : " << std::boolalpha << r.success << '\n'
              << "original path : " << testPath << '\n'
              << "resolved path : " << r.resolvedPath << '\n'
              << "message : \n" << r.message << '\n';
}

int main() {

    fs::path cwd = fs::current_path();
    std::cout << "Current Directory:\n" << cwd << "\n\n";

    // 1. Absolute path success
    printResult("1. Absolute path success", 
#ifdef _WIN32
        "C:\\Windows\\System32\\curl.exe"
#else
        "/usr/bin/curl"
#endif
    );

    // 2. Absolute path fail
    printResult("2. Absolute path fail", 
#ifdef _WIN32
        "C:\\Windows\\System32\\asdfqwer.exe"
#else
        "/usr/bin/asdfqwer"
#endif
    );

    // 3. Relative path(working directory) success
    printResult("3. Relative path success", findExistingFile(fs::current_path()));
    
    // 4. Relative path(working dir) fail
    printResult("4. Relative path fail", findExistingFile(fs::current_path()) + "asdfasdf.qwer");

    // 5. Executable directory
    printResult("5. Executable directory branch",
#ifdef _WIN32
        "glutil_pathResolveTest.exe"
#else
        "glutil_pathResolveTest"
#endif
    );

    // 6. PROJECT_ROOT
    printResult("6. PROJECT_ROOT branch", "CMakeLists.txt");

    return 0;
}