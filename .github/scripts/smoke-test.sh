#!/usr/bin/env bash
# Bulk smoke test for a gdm artifact payload (macOS and Linux).
#
#   ./smoke-test.sh               # each window waits until you close it (Esc)
#   ./smoke-test.sh --timed 5     # instead: 5 seconds each, then kill it
#   ./smoke-test.sh --list        # only show what would run
#
# Runs every executable in this directory - gdm's example, the glutil examples
# and the glutil tests - and reports whether they ran. They are started from
# this directory because that is where the assets/ lives: the executables
# resolve their shaders, textures and models through the directory next to
# themselves (glutil's exampleAssetDir()/testAssetDir()), so running them from
# anywhere else is the one thing that can make them fail.
#
# Two of them are not windows: glutil_pathResolveTest and
# glutil_encodingCheckBenchMark print a report and exit on their own.
#
# Output is never redirected in either mode, so each executable's output lands in
# this console as it produces it. At the end the script waits for Enter, so that
# output does not vanish when the payload is opened by a double-click.
#
# Interactive (the default): no timeout and no kill - the script waits until you
# close each window (Esc), then moves on to the next one. This is the mode for
# actually using the examples, and for reading what gdm_example_1_HelloWindow
# prints about this machine's OpenGL.
#
# Timed (--timed, or a number, or $WAIT): let each run for a few seconds and kill
# it - a quick pass over the whole set, for when nobody is watching. Surviving
# there means an executable started, found its assets and loaded its libraries.
#
# See README.txt for what the machine needs: on Linux, X11 + Mesa (run it under
# xvfb-run when headless); on macOS nothing at all.

set -u

here="$(cd -- "$(dirname -- "$0")" && pwd)"
cd "$here" || exit 2

timed=0
list_only=0
wait_seconds=5
usage="usage: $0 [--list] [--timed [seconds]]"
for argument in "$@"; do
    case "$argument" in
        --list)   list_only=1 ;;
        --timed)  timed=1 ;;
        --*)      echo "$usage" >&2; exit 2 ;;
        *)        wait_seconds="$argument"
                  timed=1 ;;      # a seconds value only means one thing
    esac
done
# WAIT in the environment selects the timed mode too
if [ -n "${WAIT:-}" ]; then
    timed=1
    wait_seconds="$WAIT"
fi
case "$wait_seconds" in
    ''|*[!0-9]*) echo "$usage" >&2; exit 2 ;;
esac

# one cleanup for the whole script, registered once. zsh cannot resolve a bare
# command name inside an EXIT trap in every context, so resolve rm once, up front.
RM="$(command -v rm)"
listing="$(mktemp)"
cleanup() {
    [ -n "${listing:-}" ] && "$RM" -rf -- "$listing" 2>/dev/null
    return 0
}
trap cleanup EXIT

# 1. let a downloaded copy run.
case "$(uname -s)" in
    Darwin)
        # macOS quarantines everything it downloads; one command clears all of
        # it, which is faster than approving each binary in System Settings.
        xattr -dr com.apple.quarantine . 2>/dev/null || true
        ;;
    Linux)
        # X11 and Mesa come from the distribution - see README.txt.
        # A headless machine needs them too, plus a display: xvfb-run -a "$0"
        ;;
esac

# 2. the executables: every executable file in this directory. The payload layout
# is flat (no per-example subdirectories), so one level is enough, and nothing
# has to be registered anywhere when gdm gains an example.
find . -mindepth 1 -maxdepth 1 -type f -perm -u+x \
    ! -name 'smoke-test.sh' ! -name 'smoke-test.ps1' \
    | sort > "$listing"

executables=()
while IFS= read -r entry; do
    [ -n "$entry" ] || continue
    executables+=("$entry")
done < "$listing"

if [ "${#executables[@]}" -eq 0 ]; then
    echo "error: no executables found under $here (wrong directory?)" >&2
    exit 2
fi

if [ "$timed" -eq 0 ]; then
    printf '%s\n' \
        "smoke test: ${#executables[@]} executables, close each window to move on" \
        "root: $here"
else
    printf '%s\n' \
        "smoke test: ${#executables[@]} executables, ${wait_seconds}s each" \
        "root: $here"
fi
if [ "$list_only" -eq 1 ]; then
    printf '  %s\n' "${executables[@]}"
    cleanup
    exit 0
fi

# 3. run each one. Output is never redirected, in either mode: it lands in this
#    console as the executable produces it.
passed=0
failed=0

for exe in "${executables[@]}"; do
    label="${exe#./}"
    name="${exe##*/}"
    # a separator per executable, so the console can be read - or split at - the
    # boundary between runs
    printf '\n========== %s ==========\n' "$name"
    if [ "$timed" -eq 0 ]; then
        # no timeout and no kill: the script blocks until the window is closed
        # (Esc), and the executable writes straight to this terminal
        ( cd -- "$here" && exec "./${name}" )
        exit_status=$?      # not `status`: that is read-only in zsh
        if [ "$exit_status" -eq 0 ]; then
            passed=$((passed + 1))
            printf 'PASS  %s (closed)\n' "$label"
        else
            failed=$((failed + 1))
            printf 'FAIL  %s (exit %d)\n' "$label" "$exit_status"
        fi
        continue
    fi
    # timed: same console output, but the process is killed after a few seconds.
    # exec replaces the subshell, so $! is the executable itself and can be killed.
    ( cd -- "$here" && exec "./${name}" ) &
    pid=$!
    sleep "$wait_seconds"

    if kill -0 "$pid" 2>/dev/null; then
        # still alive -> kill it. disown first: bash otherwise reports every
        # terminated background job, which buries the actual result.
        disown "$pid" 2>/dev/null || true
        kill -TERM "$pid" 2>/dev/null
        sleep 1
        kill -KILL "$pid" 2>/dev/null
        wait "$pid" 2>/dev/null
        passed=$((passed + 1))
        printf 'PASS  %s\n' "$label"
    else
        # already gone: it exited on its own, and bash has reaped it by now.
        # A clean exit is a pass - the examples normally loop until you close
        # them, so exiting at all is unusual, but exiting *successfully* is not
        # a failure of the payload.
        wait "$pid"
        exit_status=$?      # not `status`: that is read-only in zsh
        if [ "$exit_status" -eq 0 ]; then
            passed=$((passed + 1))
            printf 'PASS  %s (exited cleanly)\n' "$label"
        else
            failed=$((failed + 1))
            printf 'FAIL  %s (exit %d)\n' "$label" "$exit_status"
        fi
    fi
done

printf '\n%d passed, %d failed\n' "$passed" "$failed"
cleanup

# hold the terminal open so the output above can be read (only when there is a
# terminal to read from: piped runs would otherwise wait for input that never comes)
if [ -t 0 ]; then
    printf '\nPress Enter to exit '
    read -r || true
fi

[ "$failed" -eq 0 ] || exit 1
