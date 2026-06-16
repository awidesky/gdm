#ifndef GLUTIL_HASHING_HPP
#define GLUTIL_HASHING_HPP

#include <cstdint>
#include <ctime>
#include <string>
#include <string_view>
#include <unordered_map>

namespace glutil::debug {

/** Stable 64-bit identifier for deduplicating error events. */
using ErrorHash = uint64_t;
/** Timestamp in seconds (epoch-based). */
using ErrorTime = uint32_t;

struct ErrorStat {
    ErrorTime firstOccurrence = 0; /** Timestamp of first error in current burst window. */
    uint32_t count = 0;            /** Number of occurrences in current burst window. */
};

using ErrorMap = std::unordered_map<ErrorHash, ErrorStat>; /** Hash -> aggregated error statistics map. */

inline ErrorMap g_glDebugMessageMap; /** Global aggregation map for OpenGL debug message callback errors. */
inline ErrorMap g_gladCallbackMap;   /** Global aggregation map for GLAD callback error tracking. */

// FNV-1a 64bit hash
inline constexpr ErrorHash combineHash(ErrorHash seed, ErrorHash value) {
    return seed ^ (value + 0x9e3779b97f4a7c15ull + (seed << 6) + (seed >> 2));
}

inline constexpr ErrorHash hashString(std::string_view str) {
    constexpr ErrorHash OffsetBasis = 14695981039346656037ull;
    constexpr ErrorHash Prime = 1099511628211ull;

    ErrorHash hash = OffsetBasis;
    for (unsigned char c : str) {
        hash ^= c;
        hash *= Prime;
    }
    return hash;
}

template <typename T> inline ErrorHash hashValue(const T& value) {
    return static_cast<ErrorHash>(value); // Generic fallback hash for integral-like GLenum, GLuint, int, etc..
}

/** Hash C-string safely. */
inline ErrorHash hashValue(const char* str) { return hashString(str ? str : ""); }
/** Hash string view. */
inline ErrorHash hashValue(std::string_view str) { return hashString(str); }
/** Hash std::string. */
inline ErrorHash hashValue(const std::string& str) { return hashString(str); }


template <typename... Args> inline ErrorHash hashError(const Args&... args) {
    ErrorHash seed = 0;
    ((seed = combineHash(seed, hashValue(args))), ...);
    return seed;
}

// Return structure
struct ErrorReport {
    int32_t intervalSec; /** Time elapsed since first occurrence in current burst window. */
    uint32_t count;      /** Number of occurrences within the burst window. */
};
/*
    Log an error occurrence.
    Returns:
      - intervalSec = 0 (or negative) and count = 1 -> completely new error
      - intervalSec > 0 and count > 0 -> number of occurrences and duration in seconds
      - intervalSec = 0 and count = 0 -> reported just before, ignore
*/
inline ErrorReport logErrorOccurrence(ErrorHash hash, ErrorMap& map, ErrorTime intervalSec = 3) {
    const ErrorTime now = static_cast<ErrorTime>(std::time(nullptr));

    auto it = map.find(hash);

    // 1. first registered
    if (it == map.end()) {
        map.emplace(hash, ErrorStat{now, 1});
        return ErrorReport{-1, 1}; // first ever occurrence
    }

    auto& stat = it->second;

    // 2. reported just before (count == 0)
    if (stat.count == 0) {
        // more than 1 sec elapsed since last report -> treat as new error
        if (now - stat.firstOccurrence >= 1) {
            stat.firstOccurrence = now;
            stat.count = 1;
            return ErrorReport{-1, 1}; // new burst
        }

        // reported very recently, ignore
        stat.firstOccurrence = now;
        stat.count = 1;
        return ErrorReport{0, 0}; /** Suppressed duplicate within short interval. */
    }

    ++stat.count;

    // 3. multiple occurrences within intervalSec seconds -> report
    if (now - stat.firstOccurrence >= intervalSec) {
        ErrorReport result{static_cast<int32_t>(now - stat.firstOccurrence), stat.count};
        /** Reset burst counter after reporting. */
        stat.count = 0; // reset for next burst
        return result;
    }

    // 4. still accumulating
    return ErrorReport{0, 0}; /** Accumulating errors within current window. */
}

} // namespace glutil::debug

#endif // GLUTIL_HASHING_HPP