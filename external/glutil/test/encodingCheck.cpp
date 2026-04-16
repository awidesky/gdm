#include <glutil/shader.hpp>

#include <chrono>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "config.hpp"

static std::string readFileRaw(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open())
        return {};

    return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
}

static std::string duplicateText(const std::string& src, size_t factor) {
    std::string out;
    out.reserve(src.size() * factor);
    for (size_t i = 0; i < factor; ++i)
        out += src;
    return out;
}

static std::string format_us(double microseconds) {
    std::ostringstream os;
    os << std::fixed;

    if (microseconds >= 1'000'000.0) {
        os << std::setprecision(1) << (microseconds / 1'000'000.0) << "s";
    } else if (microseconds >= 1'000.0) {
        os << std::setprecision(1) << (microseconds / 1'000.0) << "ms";
    } else {
        os << std::setprecision(1) << microseconds << "us";
    }

    return os.str();
}

// 1) replaceUnknownNonASCII == false (No-op)
static void algoNoOp(char* /*data*/, size_t /*size*/) {
}

// 3) byte-wise test
static void algoByteReplace(char* data, size_t size) {
    for (size_t i = 0; i < size; ++i) {
        if (static_cast<unsigned char>(data[i]) & 0x80)
            data[i] = ' ';
    }
}

// 4) 32-bit word-wise test
static void algoDwordReplace(char* data, size_t size) {
    constexpr uint32_t HIGH_BIT_MASK_32 = 0x80808080U;

    size_t i = 0;
    for (; i + 4 <= size; i += 4) {
        uint32_t word;
        std::memcpy(&word, data + i, sizeof(word));
        if ((word & HIGH_BIT_MASK_32) != 0) {
            unsigned char* bytes = reinterpret_cast<unsigned char*>(data + i);
            for (size_t j = 0; j < 4; ++j) {
                if (bytes[j] & 0x80)
                    bytes[j] = ' ';
            }
        }
    }

    for (; i < size; ++i) {
        if (static_cast<unsigned char>(data[i]) & 0x80)
            data[i] = ' ';
    }
}

struct BenchResult {
    double totalUs = 0.0;
    double averageNs = 0.0;
    bool sane = true;
    std::string mismatchDetail;
};

template<typename F>
static BenchResult benchmarkMutating(const std::string& source,
                                     size_t iterations,
                                     F&& fn) {
    using clock = std::chrono::steady_clock;

    std::string work;
    work.reserve(source.size());

    std::chrono::duration<double, std::micro> algoElapsed{0};
    for (size_t i = 0; i < iterations; ++i) {
        // copy 먼저 수행, 그 다음부터 타이머 측정
        work = source;
        const auto t0 = clock::now();
        fn(work.data(), work.size());
        const auto t1 = clock::now();
        algoElapsed += (t1 - t0);
    }

    BenchResult r;
    r.totalUs = algoElapsed.count();
    r.averageNs = (r.totalUs * 1000.0) / static_cast<double>(iterations);
    return r;
}

template<typename F>
static BenchResult benchmarkChecking(const std::string& source,
                                     size_t iterations,
                                     F&& fn) {
    using clock = std::chrono::steady_clock;

    std::string work;
    work.reserve(source.size());

    std::chrono::duration<double, std::micro> algoElapsed{0};
    volatile size_t trueCount = 0;
    for (size_t i = 0; i < iterations; ++i) {
        // copy before measuring
        work = source;
        const auto t0 = clock::now();
        const bool has = fn(work.data(), work.size());
        const auto t1 = clock::now();
        algoElapsed += (t1 - t0);
        if (has)
            ++trueCount;
    }

    BenchResult r;
    r.totalUs = algoElapsed.count();
    r.averageNs = (r.totalUs * 1000.0) / static_cast<double>(iterations);
    (void)trueCount;
    return r;
}

int main() {
#ifdef GLUTIL_ENABLE_BENCHMARKS
    const std::filesystem::path ms949Path = glutil::TEST_ASSET_DIR / "shader" / "MS949.fs";
    const std::string ms949 = readFileRaw(ms949Path);

    if (ms949.empty()) {
        std::cerr << "Failed to read benchmark source: " << ms949Path << "\n";
        return 1;
    }

    std::cout << "Encoding check/replace benchmark (MS949 source)\n";
    std::cout << "----------------------------------------------\n";
    std::cout << "source: " << ms949Path << "\n";
    std::cout << "base size: " << ms949.size() << " bytes\n\n";

    const std::vector<size_t> factors = {1, 4, 8};
    for (size_t factor : factors) {
        const std::string data = duplicateText(ms949, factor);
        const size_t iterations = 100000;

        std::cout << "Case x" << factor << "\n";
        std::cout << "  size: " << data.size() << " bytes, iterations: " << iterations << "\n";

        BenchResult noOp = benchmarkMutating(data, iterations, algoNoOp);
        BenchResult check = benchmarkChecking(data, iterations, glutil::hasNonASCII);
        BenchResult qword = benchmarkMutating(data, iterations, glutil::replaceNonASCIIWithSpace);
        BenchResult dword = benchmarkMutating(data, iterations, algoDwordReplace);
        BenchResult byte = benchmarkMutating(data, iterations, algoByteReplace);

        // sanity checks (checked once)
        {
            std::string work = data;
            algoNoOp(work.data(), work.size());
            noOp.sane = (work == data);
            if (!noOp.sane)
                noOp.mismatchDetail = "no-op modified buffer unexpectedly";
        }
        {
            std::string work = data;
            glutil::replaceNonASCIIWithSpace(work.data(), work.size());
            qword.sane = !glutil::hasNonASCII(work.data(), work.size());
            if (!qword.sane)
                qword.mismatchDetail = "qword replace still has non-ASCII bytes";
        }
        {
            std::string work = data;
            algoDwordReplace(work.data(), work.size());
            dword.sane = !glutil::hasNonASCII(work.data(), work.size());
            if (!dword.sane)
                dword.mismatchDetail = "dword replace still has non-ASCII bytes";
        }
        {
            std::string work = data;
            algoByteReplace(work.data(), work.size());
            byte.sane = !glutil::hasNonASCII(work.data(), work.size());
            if (!byte.sane)
                byte.mismatchDetail = "byte replace still has non-ASCII bytes";
        }
        {
            const bool has = glutil::hasNonASCII(data.data(), data.size());
            check.sane = has;
            if (!check.sane)
                check.mismatchDetail = "check-only returned false on MS949-derived data";
        }

        auto printResult = [](const std::string& label, const BenchResult& r) {
            std::cout << "  " << label << " : total: " << r.totalUs << " us, average: " << r.averageNs << " ns/call\n";
            if (!r.sane) std::cout << "    sanity: [MISMATCH] " << r.mismatchDetail << "\n";
        };

        printResult("1) no-op       ", noOp);
        printResult("2) check-only  ", check);
        printResult("3) qword replace", qword);
        printResult("4) dword replace", dword);
        printResult("5) byte replace", byte);

        if (noOp.averageNs > 0.0) {
            std::cout << "  no-op=1x, "
                      << "check=" << (check.averageNs / noOp.averageNs) << "x, "
                      << "qword=" << (qword.averageNs / noOp.averageNs) << "x, "
                      << "dword=" << (dword.averageNs / noOp.averageNs) << "x, "
                      << "byte=" << (byte.averageNs / noOp.averageNs) << "x "
                      << "(" << format_us(noOp.totalUs) << ", "
                      << format_us(check.totalUs) << ", "
                      << format_us(qword.totalUs) << ", "
                      << format_us(dword.totalUs) << ", "
                      << format_us(byte.totalUs) << ")\n";
        }
        std::cout << "\n";
    }

    return 0;
#else
    std::cerr << "GLUTIL_ENABLE_BENCHMARKS is not enabled.\n";
    return 1;
#endif
}
