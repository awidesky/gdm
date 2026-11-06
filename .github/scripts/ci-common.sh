#!/usr/bin/env bash
# Shared helpers for .github/workflows/ci.yml. Sourced by every build step, so
# the per-OS details (generator, x64 arch, MSVC runtime library, multi-config
# builds, the run-and-kill pattern) live here once instead of in the workflow.

# Warning flags for the build that collects warnings: gdm compiles with these
# once (Debug), because a Debug build compiles the superset of the source (every
# `#if GDM_DEBUG` branch), and Release repeats it.
# MSVC's warning level is spelled with a dash on purpose: git-bash rewrites
# arguments that look like Unix paths when it hands them to a native program, so
# "/W4" reaches cl.exe as "C:/Program Files/Git/W4" - a source file that cannot be
# opened. "-W4" is the same flag (cl accepts either form) and is left alone.
if [ "${RUNNER_OS:-}" = "Windows" ]; then
    WARN_FLAGS="-W4"
else
    WARN_FLAGS="-Wall -Wextra -Wpedantic"
fi

# General guard for the same class of problem: any argument that looks like a
# Unix path is left alone here, so a later "/"-prefixed compiler or generator
# option cannot be turned into a Windows path either.
if [ "${RUNNER_OS:-}" = "Windows" ]; then
    export MSYS_NO_PATHCONV=1
    export MSYS2_ARG_CONV_EXCL='*'
fi

# gdm_configure <build-dir> <Debug|Release> [extra cmake args...]
# Configures a fresh build directory and tees the configure log to
# cfg-<build-dir>.log, so the assertions can grep what gdm resolved.
gdm_configure() {
    local bd="$1" cfg="$2"
    shift 2
    local gen=() runtime=()
    if [ "${RUNNER_OS:-}" = "Windows" ]; then
        # vswhere resolves the installed toolset (see the workflow): windows-latest
        # carries only VS2026, which needs CMake >= 4.1.
        gen=(-G "${VS_GENERATOR:-Visual Studio 17 2022}" -A x64)
        # Ship the artifact without a VC++ redistributable dependency.
        case "$cfg" in
            Debug) runtime=(-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDebug) ;;
            *)     runtime=(-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded) ;;
        esac
    else
        gen=(-G Ninja)
    fi
    # gdm reads its options as cache variables with defaults
    # (set(GDM_GLFW_VERSION ... CACHE STRING ...)), and CMake does not take a
    # value for those from the environment - only from the command line. The
    # workflow pins the versions in env (and the installed job rewrites them from
    # what the package manager actually has), so forward them as -D here, once,
    # instead of repeating every name in every step. Callers' own -D comes last
    # and therefore still wins.
    # Only the variables that apply to this configuration are forwarded: a
    # variable gdm would not use for the selected provider sits in the cache
    # unused, and CMake reports "Manually-specified variables were not used by
    # the project" for every one of them.
    local versions=() var value
    local backend="${GDM_WINDOW_BACKEND:-glfw}" loader="${GDM_GL_LOADER:-glad}"
    local have_glm="${GDM_USE_GLM:-ON}" have_glutil="${GDM_USE_GLUTIL:-ON}"
    local arg
    # The provider selection can also arrive on this very command line (the
    # workflow's steps pass -DGDM_WINDOW_BACKEND=freeglut -DGDM_GL_LOADER=glew),
    # and a command-line value overrides the environment for what is relevant.
    for arg in "$@"; do
        case "${arg}" in
            -DGDM_WINDOW_BACKEND=*) backend="${arg#-DGDM_WINDOW_BACKEND=}" ;;
            -DGDM_GL_LOADER=*)      loader="${arg#-DGDM_GL_LOADER=}" ;;
            -DGDM_USE_GLM=*)        have_glm="${arg#-DGDM_USE_GLM=}" ;;
            -DGDM_USE_GLUTIL=*)     have_glutil="${arg#-DGDM_USE_GLUTIL=}" ;;
        esac
    done

    forward() {   # forward <name>: only when set, and only when apply says yes
        eval "value=\${${1}:-}"
        [ -n "${value}" ] || return 0
        local want=yes
        case "${1}" in
            GDM_GLFW_VERSION|GDM_GLFW_STATIC) [ "$backend" = glfw ] || want=no ;;
            GDM_FREEGLUT_VERSION)  [ "$backend" = freeglut ] || want=no ;;
            GDM_GLM_VERSION)       [ "$have_glm" != "OFF" ] || want=no ;;
            GDM_CPPTRACE_VERSION)  [ "$have_glutil" != "OFF" ] || want=no ;;
            GDM_GLEW_VERSION|GDM_GLEW_STATIC)
                case "$loader" in glew|glew-glad) ;; *) want=no ;; esac ;;
            GDM_GLAD_API|GDM_GLAD_PROFILE|GDM_GLAD_EXTENSION)
                [ "$loader" = glad ] || want=no ;;
        esac
        if [ "$want" = yes ]; then
            versions+=("-D${1}=${value}")
        fi
        return 0
    }

    for var in GDM_WINDOW_BACKEND GDM_GL_LOADER GDM_USE_GLM GDM_USE_GLUTIL \
               GDM_BUILD_EXAMPLES GDM_BUILD_TESTS GDM_EXTERNAL_DIR \
               GDM_GLFW_VERSION GDM_FREEGLUT_VERSION GDM_GLM_VERSION \
               GDM_CPPTRACE_VERSION GDM_GLEW_VERSION GDM_GLEW_STATIC \
               GDM_GLAD_API GDM_GLAD_PROFILE GDM_GLAD_EXTENSION GDM_TLS_VERIFY; do
        forward "${var}"
    done
    # The installed job's probe resolves the packages through $PREFIX, so gdm has
    # to search the same one - on macOS it is /opt/homebrew, which CMake already
    # knows (Darwin's system prefix list), but on Windows vcpkg's triplet directory
    # is not in any default list, and gdm would download what the probe just found.
    if [ -n "${PREFIX:-}" ]; then
        versions+=("-DCMAKE_PREFIX_PATH=${PREFIX}")
    fi
    # The ${array[@]+...} form is not decoration: an empty array expansion is an
    # "unbound variable" error under `set -u` on the bash 3.2 macOS still ships,
    # which is what a developer sourcing this file can be running.
    cmake -S . -B "${bd}" ${gen[@]+"${gen[@]}"} -DCMAKE_BUILD_TYPE="${cfg}" \
          ${runtime[@]+"${runtime[@]}"} ${versions[@]+"${versions[@]}"} \
          "$@" 2>&1 | tee "cfg-$(basename "${bd}").log"
}

# gdm_build <build-dir> <Debug|Release> [target...]
# Builds every target, or the named ones, and tees to build-<build-dir>.log.
gdm_build() {
    local bd="$1" cfg="$2"
    shift 2
    local args=(--build "${bd}" --parallel)
    [ "${RUNNER_OS:-}" = "Windows" ] && args+=(--config "${cfg}")
    local target
    for target in "$@"; do
        args+=(--target "${target}")
    done
    cmake "${args[@]}" 2>&1 | tee "build-$(basename "${bd}").log"
}

# gdm_exe <build-dir> <name-without-extension>
# Prints the built executable, wherever this generator put it.
gdm_exe() {
    # The exact name, on purpose: a glob like "name*" also matches the MSBuild
    # sidecars that carry the target's name (gdm_example_1_HelloWindow.exe's
    # .lastbuildstate and .tlog files), and running one of those is a bizarre
    # failure rather than a clear one.
    local bd="$1" name="$2" hit
    hit=$(find "${bd}" -type f \( -name "${name}" -o -name "${name}.exe" \) 2>/dev/null \
          | head -n 1 || true)
    printf '%s\n' "${hit}"
}

# gdm_run_example <build-dir>
# Runs the built gdm example. A run that is attempted and fails fails the step/
# It is attempted on Linux only, because that is where the hosted runners can
# actually create an OpenGL context (Xvfb + Mesa's software renderer):
# the macOS runners report "NSGL: Failed to find a suitable pixel format" for every
# context version, and the Windows runner's driver reports "The driver does not
# appear to support OpenGL". Build and link are still checked on all three, so the
# skip costs no coverage of gdm's own code.
gdm_run_example() {
    local bd="$1"
    if [ "${RUNNER_OS:-}" = "Linux" ]; then
        gdm_run "$(gdm_exe "${bd}" gdm_example_1_HelloWindow)" 5 "run-$(basename "${bd}").log" "${bd}"
    else
        echo "SKIP: this runner cannot create an OpenGL context, so the example is"
        echo "      not run here (the build above is what this platform checks)"
    fi
}

# gdm_run <exe> <seconds> <log-file> [build-dir]
# Starts the executable, lets it run, then decides:
#   still alive after <seconds>  -> PASS, it reached its render loop
#   exited before that           -> FAIL, its exit code is the failure
# The executables loop until Esc/window close, so they are meant to be killed;
# exiting at all is either a real error (bad context, missing assets, ...) or a
# clean run that finished on its own, which is reported separately.
gdm_run() {
    local exe="$1" secs="$2" log="$3" bd="${4:-}"
    [ -n "${exe}" ] || { echo "gdm_run: no executable"; return 1; }
    [ -x "${exe}" ] || [ -f "${exe}" ] || { echo "gdm_run: ${exe} not found"; return 1; }

    # Windows: dependencies built as shared libraries (freeglut, and GLEW when the
    # probe chose a shared package) are resolved from the executable's own
    # directory only, so their DLLs have to sit next to it. They come from the
    # build tree and, for the installed job, from the package prefix's bin dir.
    if [ "${RUNNER_OS:-}" = "Windows" ]; then
        local dll src
        for src in "${bd}" "${PREFIX:-/nonexistent}/bin"; do
            [ -d "${src}" ] || continue
            while IFS= read -r dll; do
                [ -n "${dll}" ] || continue
                cp -f "${dll}" "$(dirname "${exe}")" 2>/dev/null || true
            done < <(find "${src}" -type f -name "*.dll" 2>/dev/null)
        done
    fi

    "${exe}" >"${log}" 2>&1 &
    local pid=$!
    sleep "${secs}"
    local rc=0
    if kill -0 "${pid}" 2>/dev/null; then
        # disown before killing: bash otherwise reports every terminated
        # background job ("Terminated: 15"), which buries the actual result
        disown "${pid}" 2>/dev/null || true
        kill "${pid}" 2>/dev/null || true
        sleep 1
        kill -9 "${pid}" 2>/dev/null || true
        wait "${pid}" 2>/dev/null || true
        echo "PASS: still running after ${secs}s (render loop reached)"
    else
        wait "${pid}" || rc=$?
        if [ "${rc}" -eq 0 ]; then
            echo "PASS: exited cleanly on its own"
        else
            echo "FAIL: exited early with code ${rc}"
        fi
    fi
    cat "${log}"
    [ "${rc}" -eq 0 ]
}

# gdm_require <file> <pattern> <message>
gdm_require() {
    if grep -q "$2" "$1"; then
        echo "ok: $3"
    else
        echo "FAILED: $3 (no /$2/ in $1)" >&2
        return 1
    fi
}

# gdm_forbid <file> <pattern> <message>
gdm_forbid() {
    if grep -q "$2" "$1"; then
        echo "FAILED: $3 (found /$2/ in $1)" >&2
        grep -n "$2" "$1" >&2
        return 1
    fi
    echo "ok: $3"
}

# gdm_collect_warnings <build-log>
# Collects compiler warnings that belong to gdm itself (gdm's sources, gdm's
# example, glutil) into warnings.txt and appends a section to the job summary.
# Warnings of fetched dependencies (glad's generated gl.c, glfw, glew-cmake,
# glm, cpptrace, stb, tiny_obj_loader, dds-ktx) are not gdm's to fix.
gdm_collect_warnings() {
    local log="$1"
    # One pipeline with one trailing "|| true": grep exits 1 when a filter matches
    # nothing, and a step running under `bash -e` aborts on that. Note that `||`
    # must come after the whole pipeline - inside it, "A | B || true | C" parses
    # as "(A | B) || (true | C)" and C silently never runs.
    grep -E "warning" "${log}" 2>/dev/null \
        | grep -E "(include|src|examples)[/\]|glutil[/\]" \
        | grep -viE "external[/\\](glfw3|freeglut|glad-|glew-cmake|glm-|cpptrace|stb_image|tiny_obj_loader|dds-ktx)" \
        > warnings.txt || true
    local count
    count=$(wc -l < warnings.txt | tr -d ' ')
    echo "gdm warnings: ${count} (see warnings.txt)"
    if [ -n "${GITHUB_STEP_SUMMARY:-}" ]; then
        {
            echo "## gdm warnings (${RUNNER_OS}, Debug): ${count}"
            echo '```'
            cat warnings.txt
            echo '```'
        } >>"${GITHUB_STEP_SUMMARY}"
    fi
}
