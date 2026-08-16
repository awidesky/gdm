/*
 * Encoding and replacement benchmark for shader text sanitization.
 *
 * This test measures the cost of glutil::hasNonASCII and
 * glutil::replaceNonASCIIWithSpace against a real MS949-derived shader source,
 * then sanity-checks that the no-op, check-only, and replacement paths behave
 * correctly as the input is duplicated at x1, x4, and x8 sizes.
 *
 * You can check how much overhead does it take to check the encoding or 
 * replace all non-ASCII characters with spaces in a shader source. *
 * Following is the summary of the benchmark results in my MacBook Air (M2, 2022) 8GB with Apple clang 17.0.0:

Case x1
  size: 1954 bytes, iterations: 100000
  1) no-op        : total: 4292.72 us, average: 42.9272 ns/call
  2) check-only   : total: 11982.3 us, average: 119.823 ns/call
  3) qword replace : total: 114816 us, average: 1148.16 ns/call
  4) dword replace : total: 161734 us, average: 1617.34 ns/call
  5) byte replace : total: 515138 us, average: 5151.38 ns/call
  no-op=1x, check=2.79131x, qword=26.7467x, dword=37.6762x, byte=120.003x (4.3ms, 12.0ms, 114.8ms, 161.7ms, 515.1ms)

Case x4
  size: 7816 bytes, iterations: 100000
  1) no-op        : total: 3894.14 us, average: 38.9414 ns/call
  2) check-only   : total: 11629.8 us, average: 116.298 ns/call
  3) qword replace : total: 518957 us, average: 5189.57 ns/call
  4) dword replace : total: 924943 us, average: 9249.43 ns/call
  5) byte replace : total: 1.70019e+06 us, average: 17001.9 ns/call
  no-op=1x, check=2.98647x, qword=133.266x, dword=237.522x, byte=436.6x (3.9ms, 11.6ms, 519.0ms, 924.9ms, 1.7s)

Case x8
  size: 15632 bytes, iterations: 100000
  1) no-op        : total: 3178.49 us, average: 31.7849 ns/call
  2) check-only   : total: 10242.4 us, average: 102.424 ns/call
  3) qword replace : total: 763583 us, average: 7635.83 ns/call
  4) dword replace : total: 1.09756e+06 us, average: 10975.6 ns/call
  5) byte replace : total: 3.30801e+06 us, average: 33080.1 ns/call
  no-op=1x, check=3.22241x, qword=240.235x, dword=345.31x, byte=1040.75x (3.2ms, 10.2ms, 763.6ms, 1.1s, 3.3s)

 * Replacing non-ASCII characters with spaces is significantly slower even with the optimized qword approach,
 * and the overhead increases with larger shader sources. 
 * Still, considering that shader sources are typically small and the replacement is done only once per shader load,
 * the actual overhead in a real application is likely to be just a couple of microseconds.
 */

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
