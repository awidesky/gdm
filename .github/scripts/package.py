#!/usr/bin/env python3
"""Verify the gdm build and package it into a runnable artifact.

The CI (see .github/workflows/ci.yml) builds every CMake target but never runs
them for real: the examples open a window and loop until Esc, which a runner
cannot judge. What the build *can* prove is that configuring succeeded, that
every target linked, and that every executable exists. This script turns that
proof into an artifact: every executable next to the assets it loads at runtime,
so the result can be run on a machine that has no OpenGL development files
installed.

The assets are staged into a single "assets" directory, because that is the
name glutil's exampleAssetDir()/testAssetDir() resolve when the compile-time
directory does not exist (the shipped binary case) - they search the working
directory, then the executable directory, then the project root.

Two outputs:
  * the unpacked payload (--payload-dir) - what actions/upload-artifact should
    get, so a downloaded artifact expands straight into it instead of into yet
    another archive (upload-artifact always zips what it is handed)
  * a zip (--out-dir, optional) - what you hand to someone else

Usage (usually called from the workflow, but works locally, too):

    python3 .github/scripts/package.py --build-dir b-dbg --payload-dir payload

Exits non-zero if any expected executable is missing, so the workflow step
fails as well.
"""

from __future__ import annotations

import argparse
import os
import platform
import re
import shutil
import subprocess
import sys
import tempfile
import zipfile
from pathlib import Path

SCRIPT_DIR = Path(__file__).resolve().parent
REPO_ROOT = SCRIPT_DIR.parent.parent

ADD_EXECUTABLE_RE = re.compile(r"add_executable\s*\(\s*([A-Za-z0-9_.+-]+)([^)]*)\)", re.S)
# glutil declares its examples/tests through wrapper functions, not
# add_executable(), so parse those too (external/glutil/CMakeLists.txt).
GLUTIL_EXAMPLE_RE = re.compile(r"add_glutil_example\s*\(\s*([A-Za-z0-9_]+)\s+([^\s)]+)")
GLUTIL_TEST_RE = re.compile(
    r"add_glutil_test\s*\(\s*([A-Za-z0-9_]+)\s+([A-Za-z0-9_]+)\s+([^\s)]+)")
OPTION_KEYWORDS = {"win32", "macosx_bundle", "exclude_from_all"}

GLUTIL_DIR = Path("external/glutil")
EXAMPLES_DIR = Path("examples")

# Runtime assets, staged under one "assets" directory in the payload: the name
# glutil::exampleAssetDir()/testAssetDir() fall back to. The example and test
# shader directories hold different files, so they merge without collisions.
ASSET_DIR_NAME = "assets"
ASSET_SOURCES = [
    (GLUTIL_DIR / "example" / "shader", "shader"),
    (GLUTIL_DIR / "example" / "texture", "texture"),
    (GLUTIL_DIR / "example" / "model", "model"),
    (GLUTIL_DIR / "test" / "shader", "shader"),
]

# Not needed to run an executable: build inputs and OS/editor noise.
SKIP_DIRS = {"__pycache__", ".git"}
SKIP_SUFFIXES = {".cpp", ".hpp", ".c", ".h", ".cxx", ".cc", ".ipp", ".inl",
                 ".txt", ".pyc"}
# Build outputs of any platform: never runtime assets, and a source directory
# left over from a Windows build must not leak into another platform's payload.
SKIP_BUILD_SUFFIXES = {".exe", ".pdb", ".dll", ".ilk", ".exp", ".lib", ".a",
                       ".o", ".so", ".dylib", ".app"}
SKIP_FILENAMES = {".ds_store", "thumbs.db", "build.log"}

PLATFORM_NAMES = {"Windows": "windows", "Darwin": "macos", "Linux": "linux"}

# Shared libraries the payload must not bundle: everything below these prefixes
# belongs to the OS and every target machine already has it. gdm builds glfw,
# glew-cmake, glm and cpptrace from source, and static, so this list is normally
# empty - it is kept as a safety net for a run that ever links something shared.
SYSTEM_LIB_PREFIXES = {
    "Darwin": ("/usr/lib/", "/System/Library/", "@executable_path/",
               "@loader_path/", "@rpath/"),
    "Linux": ("/lib/", "/usr/lib/", "/lib64/", "/usr/lib64/"),
}
NON_DYNAMIC_MARKERS = ("statically linked", "not a dynamic executable",
                       "linux-vdso", "ldd:")

REQUIREMENTS = {
    "windows": (
        "A GPU/driver exposing a modern OpenGL. Windows only ships opengl32.dll,\n"
        "  which is an OpenGL 1.1 software rasterizer; these executables need at\n"
        "  least GL 3.3 from the graphics driver. Nothing else to install.\n"
        "\n"
        "A downloaded zip carries the \"Mark of the Web\", which blocks unsigned\n"
        "executables: run the smoke test once (it clears it for the whole folder)\n"
        "or right-click each file, Properties, Unblock."
    ),
    "macos": (
        "Nothing to install: OpenGL.framework and the other frameworks these\n"
        "  executables use ship with macOS itself (OpenGL has been deprecated\n"
        "  since 10.14, but it is still there and sufficient, these use at most\n"
        "  GL 4.1).\n"
        "\n"
        "These executables are not notarized - that needs a paid Apple Developer\n"
        "  ID - so Gatekeeper stops the first launch. Approve each file once in\n"
        "  System Settings > Privacy & Security > Open Anyway, or clear the\n"
        "  download marker for the whole folder at once:\n"
        "      xattr -dr com.apple.quarantine ."
    ),
    "linux": (
        "X11 and Mesa are not part of these executables, so they must come from\n"
        "  your distribution:\n"
        "\n"
        "  Debian/Ubuntu:\n"
        "      sudo apt-get install -y libgl1 libglu1-mesa libx11-6 libxext6 \\\n"
        "        libxrandr2 libxinerama1 libxcursor1 libxi6 mesa-utils \\\n"
        "        mesa-dri-drivers libwayland-client0 libwayland-cursor0 \\\n"
        "        libwayland-egl1 libxkbcommon0\n"
        "\n"
        "  Fedora:\n"
        "      sudo dnf install -y mesa-libGL mesa-libGLU libX11 libXext \\\n"
        "        libXrandr libXinerama libXcursor libXi mesa-dri-drivers \\\n"
        "        mesa-utils libwayland-client libwayland-cursor libwayland-egl \\\n"
        "        libxkbcommon\n"
        "\n"
        "  Arch:\n"
        "      sudo pacman -S mesa libglu libx11 libxext libxrandr libxinerama \\\n"
        "        libxcursor libxi mesa-dri-drivers mesa-utils wayland libxkbcommon\n"
        "\n"
        "Mesa (llvmpipe) provides the software fallback, so these also run on a\n"
        "  machine with no GPU. Headless, run them under an X server:\n"
        "      xvfb-run -a ./smoke-test.sh --timed 5"
    ),
}

README_TEMPLATE = """gdm examples - {plat} build
Commit {sha}{config}

This directory holds the gdm example and the glutil examples and tests built by
this configuration, together with the assets they load at runtime. Run them from
inside this directory, e.g.:

    cd <this directory>
    ./gdm_example_1_HelloWindow{exe_note}

gdm_example_1_HelloWindow is the one to attach to a bug report: it prints the
machine's OpenGL environment - GL version, vendor, renderer, capability limits,
supported extensions - and then draws a triangle until you close the window (Esc).
The glutil examples demonstrate shader/texture/model loading and the debug
features; the glutil tests check the path and shader-encoding handling.

Every executable opens a window and loops until you close it. Two exceptions:
glutil_pathResolveTest and glutil_encodingCheckBenchMark print their report and
exit on their own, without a display.

Assets live in the "{assets}/" directory (shaders, textures, models); the
executables find them next to themselves, which is why they are run from here.
Debug information is{strip_note}.

To run them all, one at a time:

    macOS / Linux:  ./smoke-test.sh
    Windows:        .\\smoke-test.ps1

By default each one waits until you close its window (Esc), with its output
inline here. The timed mode runs each for a few seconds and just reports whether
it started - which is what proves it found its assets and loaded its libraries:

    macOS / Linux:  ./smoke-test.sh --timed 5
    Windows:        .\\smoke-test.ps1 -Timed 5

Either way the script waits for Enter at the end, so the output does not vanish
when this payload was opened by a double-click.

Requirements on the machine that runs these:
{requirements}
"""


def parse_targets(cmake_file: Path, cmake_dir: Path):
    """[(target name, source dir), ...] declared by add_executable() in a file."""
    text = cmake_file.read_text(errors="replace")
    targets = []
    for match in ADD_EXECUTABLE_RE.finditer(text):
        name = match.group(1)
        args = match.group(2).split()
        source = next((a for a in args if a.lower() not in OPTION_KEYWORDS), None)
        if source is None:
            continue
        targets.append((name, (cmake_dir / source).parent))
    return targets


def parse_glutil_targets(cmake_file: Path):
    """[(target name, source dir), ...] declared by glutil's wrapper functions."""
    text = cmake_file.read_text(errors="replace")
    targets = []
    for match in GLUTIL_EXAMPLE_RE.finditer(text):
        targets.append((match.group(1), cmake_file.parent / "example"))
    for match in GLUTIL_TEST_RE.finditer(text):
        targets.append((match.group(1), cmake_file.parent / "test"))
    return targets


def collect_targets(repo_root: Path, cache: dict):
    """[(target name, source dir), ...] of everything this configuration builds.

    glutil's examples and tests only exist for glfw + glad + glm
    (external/glutil/CMakeLists.txt gates them), and gdm's own examples only when
    GDM_BUILD_EXAMPLES is on, so a build configured any other way packages only
    what it actually produced.
    """
    targets = list(parse_targets(repo_root / EXAMPLES_DIR / "CMakeLists.txt",
                                 repo_root / EXAMPLES_DIR))

    glutil = repo_root / GLUTIL_DIR / "CMakeLists.txt"
    if not glutil.is_file():
        raise SystemExit(f"error: {GLUTIL_DIR} is missing; "
                         "glutil is shipped inside this repository")
    providers_ok = (cache.get("GDM_WINDOW_BACKEND", "glfw") == "glfw"
                    and cache.get("GDM_GL_LOADER", "glad") == "glad"
                    and cache.get("GDM_USE_GLM", "ON").upper() == "ON")
    if providers_ok:
        targets += parse_glutil_targets(glutil)
    return targets


def read_cache(build_dir: Path):
    values = {}
    cache_file = build_dir / "CMakeCache.txt"
    if cache_file.is_file():
        for line in cache_file.read_text(errors="replace").splitlines():
            match = re.match(r"^([A-Za-z0-9_]+):[A-Z_]*=(.*)$", line)
            if match:
                values[match.group(1)] = match.group(2)
    return values


def exe_suffix():
    return ".exe" if os.name == "nt" else ""


def find_executable(name, source_dir, build_dir, config, repo_root=None):
    """Where the build put the exe: source dir (POST_BUILD copy), build dir, or
    multi-config subdir. Targets declared in a subdirectory of the source tree
    keep that layout under build/ too - glutil's land in build/external/glutil/ -
    so those paths are probed before the bounded search of the build tree, which
    covers generators we did not anticipate."""
    exe = name + exe_suffix()
    candidates = [source_dir / exe, build_dir / exe]
    if config:
        candidates.insert(1, build_dir / config / exe)
    if repo_root is not None:
        try:
            source_rel = source_dir.resolve().relative_to(repo_root)
        except ValueError:
            source_rel = None
        if source_rel:
            sub = build_dir / source_rel
            candidates += [sub / exe]
            if config:
                candidates.append(sub / config / exe)
    for candidate in candidates:
        if candidate.is_file():
            return candidate
    for root, dirs, files in os.walk(build_dir):
        if exe in files:
            return Path(root) / exe
        dirs[:] = [d for d in dirs
                   if d not in ("CMakeFiles", "_gdm_deps", "_gdm_extract",
                                "_gdm_downloads")]
    return None


EXECUTABLE_MAGICS = (b"\xcf\xfa\xed\xfe", b"\xce\xfa\xed\xfe",  # Mach-O
                     b"\xca\xfe\xba\xbe", b"\xbe\xba\xfe\xca",  # universal
                     b"\xfe\xed\xfa\xcf", b"\xfe\xed\xfa\xce")  # Mach-O, other endian


def is_executable_file(path: Path) -> bool:
    """Mach-O / ELF / PE, so permissions can be normalized by content."""
    try:
        with open(path, "rb") as handle:
            head = handle.read(4)
    except OSError:
        return False
    return (head.startswith(b"MZ") or head.startswith(b"\x7fELF")
            or head in EXECUTABLE_MAGICS)


WINDOWS_FORBIDDEN_CHARS = '<>:"|?*'
WINDOWS_RESERVED_STEMS = ({"CON", "PRN", "AUX", "NUL"}
                          | {f"COM{i}" for i in range(1, 10)}
                          | {f"LPT{i}" for i in range(1, 10)})
PLATFORM_MAGIC = {
    "windows": (b"MZ",),                                # PE
    "macos": tuple(EXECUTABLE_MAGICS),                  # Mach-O / universal
    "linux": (b"\x7fELF",),
}


def validate_payload(stage_root: Path, plat: str):
    """Refuse to ship something the target OS cannot use.

    A directory Explorer or `dir` reports as corrupted, or an executable that
    fails with 0x0000007B (ERROR_INVALID_NAME), is a payload problem long before
    it is a code problem, so catch the two ways it can happen: an entry name the
    file system rejects, and a staged executable that is not one.
    """
    problems = []
    magic_ok = PLATFORM_MAGIC.get(plat, (b"MZ", b"\x7fELF") + EXECUTABLE_MAGICS)
    for path in sorted(stage_root.rglob("*")):
        name = path.name
        rel = path.relative_to(stage_root)
        if plat == "windows":
            forbidden = [c for c in name if c in WINDOWS_FORBIDDEN_CHARS
                         or ord(c) < 32]
            if forbidden:
                problems.append(f"{rel}: name contains {forbidden!r}")
            if name.rstrip() != name or name.rstrip(".") != name:
                problems.append(f"{rel}: trailing space or dot")
            stem = name.split(".")[0].upper()
            if stem in WINDOWS_RESERVED_STEMS:
                problems.append(f"{rel}: '{stem}' is a reserved device name")
            if len(str(rel)) > 200:
                problems.append(f"{rel}: relative path is {len(str(rel))} chars")
        if not path.is_file():
            continue
        with open(path, "rb") as handle:
            head = handle.read(4)
        if is_executable_file(path) and not any(head.startswith(m)
                                                for m in magic_ok):
            problems.append(f"{rel}: not a {plat} executable "
                            f"(magic {head.hex()})")
        if name.lower().endswith(".exe") and plat == "windows" \
                and not head.startswith(b"MZ"):
            problems.append(f"{rel}: .exe that is not a PE image")
    return problems


MACHO_ARCHS = {0x00000007: "x86", 0x01000007: "x86_64",
               0x0000000C: "arm", 0x0100000C: "arm64"}


def describe_binary(path: Path) -> str:
    """'Mach-O arm64 (3.6 MB)', 'PE (2.1 MB)', ... - what the payload holds, so a
    reviewer on another machine can tell without running anything."""
    with open(path, "rb") as handle:
        head = handle.read(8)
    size = human_size(path.stat().st_size)
    if head.startswith(b"MZ"):
        return f"PE ({size})"
    if head.startswith(b"\x7fELF"):
        return f"ELF ({size})"
    if head[:4] in EXECUTABLE_MAGICS:
        cputype = int.from_bytes(head[4:8], "little")
        return f"Mach-O {MACHO_ARCHS.get(cputype, 'arch 0x%x' % cputype)} ({size})"
    return f"unknown format ({size})"


def strip_debug_info(path: Path):
    """Drop debug/symbol information from a staged executable.

    A Debug build embeds the symbols of gdm, glutil and every statically linked
    library (glfw, glew, glm, cpptrace/libdwarf) into each executable, which
    repeats tens of megabytes over and over. GCC/Clang keep them in the binary;
    MSVC moves them to a sidecar .pdb, which is why a Linux payload dwarfs a
    Windows one. Only the staged copies are touched: the build tree keeps its
    symbols for anyone who needs a traceback.
    """
    if os.name == "nt":
        return  # MSVC keeps debug info in the .pdb; there is nothing to strip
    command = (["strip", "-S", "-x"] if platform.system() == "Darwin"
               else ["strip", "--strip-unneeded", str(path)])
    try:
        subprocess.run(command + [str(path)], check=True, capture_output=True)
    except (OSError, subprocess.CalledProcessError) as error:
        print(f"warning: could not strip {path}: {error}", file=sys.stderr)
        return
    if platform.system() == "Darwin":
        # stripping rewrites the binary, so re-sign it ad-hoc
        subprocess.run(["codesign", "--force", "--sign", "-", str(path)],
                       check=True, capture_output=True)


def relax_permissions(path: Path, executable: bool = False):
    """A downloaded payload must be usable without escalating privileges.

    copy2 preserves the source mode, and sources bundled from elsewhere can be
    read-only. Removing the quarantine marker macOS puts on a download -
    `xattr -d com.apple.quarantine` - needs write access, so a read-only file
    makes the one-command remedy fail.
    """
    mode = 0o755 if executable else 0o644
    try:
        if path.stat().st_mode & 0o777 != mode:
            path.chmod(mode)
    except OSError:
        pass  # best effort: some filesystems forbid chmod


def stage_assets(stage_root: Path, repo_root: Path):
    """Stage the runtime assets under one "assets" directory.

    That is the name glutil's exampleAssetDir()/testAssetDir() resolve when the
    compile-time directory is missing - the case for every shipped binary - by
    looking in the working directory, then next to the executable, then the
    project root.
    """
    staged = 0
    for source, sub in ASSET_SOURCES:
        source = repo_root / source
        if not source.is_dir():
            continue
        target = stage_root / ASSET_DIR_NAME / sub
        target.mkdir(parents=True, exist_ok=True)
        for item in sorted(source.iterdir()):
            if not item.is_file():
                continue
            if item.name.lower() in SKIP_FILENAMES:
                continue
            if item.suffix.lower() in SKIP_BUILD_SUFFIXES:
                continue
            shutil.copy2(item, target / item.name)
            relax_permissions(target / item.name)
            staged += 1
    return staged


def dynamic_dependencies(binary):
    """Absolute paths of the shared libraries `binary` loads ([] on Windows)."""
    if os.name == "nt":
        return []
    command = (["otool", "-L", str(binary)]
               if platform.system() == "Darwin" else ["ldd", str(binary)])
    try:
        result = subprocess.run(command, capture_output=True, text=True, timeout=120)
    except (OSError, subprocess.SubprocessError):
        return []
    if result.returncode != 0:
        return []
    dependencies = []
    for line in result.stdout.splitlines():
        line = line.strip()
        if any(marker in line for marker in NON_DYNAMIC_MARKERS):
            continue
        path = line.split(" => ", 1)[-1].split(" (")[0].strip()
        if os.path.isabs(path) and os.path.exists(path):
            dependencies.append(path)
    return dependencies


def dump_dependencies(binary):
    """Raw otool/ldd listing of `binary`, for the manifest."""
    if os.name == "nt":
        return []
    command = (["otool", "-L", str(binary)]
               if platform.system() == "Darwin" else ["ldd", str(binary)])
    try:
        result = subprocess.run(command, capture_output=True, text=True, timeout=120)
    except (OSError, subprocess.SubprocessError):
        return [f"<{command[0]} not runnable>"]
    if result.returncode != 0:
        return [f"<{command[0]} failed: {result.stderr.strip()}>"]
    lines = [line.strip() for line in result.stdout.splitlines() if line.strip()]
    if lines and lines[0].startswith(str(binary)):  # otool's "path:" header
        lines = lines[1:]
    return lines


def is_system_lib(path):
    prefixes = SYSTEM_LIB_PREFIXES.get(platform.system(), ("/usr/",))
    normalized = os.path.normpath(path)
    return any(normalized.startswith(p) for p in prefixes)


LIB_DIR_NAME = "lib"
EXE_RPATH = {"Darwin": ["@executable_path/lib", "@executable_path/../lib"],
             "Linux": ["$ORIGIN/lib", "$ORIGIN/../lib"]}
LIB_RPATH = {"Darwin": ["@loader_path/"], "Linux": ["$ORIGIN"]}


def patch(binary, arguments):
    """Run a Mach-O/ELF patching tool, then re-sign on Darwin: any
    install_name_tool change invalidates the (ad-hoc) signature, and macOS then
    refuses to exec the file at all - SIGKILL, no diagnostic."""
    subprocess.run(arguments, check=True, capture_output=True)
    if platform.system() == "Darwin":
        subprocess.run(["codesign", "--force", "--sign", "-", str(binary)],
                       check=True, capture_output=True)


def rpath_entries(binary):
    if platform.system() != "Darwin":
        return []
    listing = subprocess.run(["otool", "-l", str(binary)],
                             capture_output=True, text=True).stdout
    return [m.group(1) for m in re.finditer(r"^\s*path (.+) \(offset", listing, re.M)]


def ensure_rpath(binary, rpaths):
    """Let `binary` find the payload's lib/ directory, and nothing else.

    The build tree can rpath in the machine's own library directories, and dyld
    honours rpaths in order, so an inherited entry would resolve our @rpath names
    to the build machine's copies instead of the shipped ones. Every other entry
    is therefore deleted; on Linux patchelf's --set-rpath replaces the list.
    """
    rpaths = list(rpaths)
    if platform.system() == "Darwin":
        foreign = [r for r in rpath_entries(binary) if r not in rpaths]
        for rpath in foreign:
            try:
                patch(binary, ["install_name_tool", "-delete_rpath", rpath,
                               str(binary)])
            except subprocess.CalledProcessError:
                pass  # duplicate entry: already gone
        for rpath in rpaths:
            if rpath not in rpath_entries(binary):
                patch(binary, ["install_name_tool", "-add_rpath", rpath,
                               str(binary)])
    else:
        if not shutil.which("patchelf"):
            raise SystemExit(
                f"error: '{binary}' needs non-system libraries from {LIB_DIR_NAME}/; "
                "bundling them requires patchelf (apt-get install patchelf) to set "
                "an $ORIGIN rpath")
        patch(binary, ["patchelf", "--set-rpath", ":".join(rpaths), str(binary)])


def repoint(binary, dep, name):
    """Make `binary` load the bundled copy `name` through the rpath."""
    if platform.system() == "Darwin":
        patch(binary, ["install_name_tool", "-change", dep,
                       f"@rpath/{name}", str(binary)])
    # Linux: DT_NEEDED already holds the soname, so the rpath alone is enough.


def bundle_libs(binary, lib_dir):
    """Copy every non-system library `binary` needs into the payload's single
    lib/ directory (recursively), and repoint the staged copies at it. Nothing
    outside the OS is shipped more than once; only the staged files are patched,
    never the build outputs. Returns [bundled library names]."""
    binary = Path(binary)
    bundled, queue, seen = {}, [binary], set()
    while queue:
        current = queue.pop()
        for dep in dynamic_dependencies(current):
            if dep in seen or is_system_lib(dep):
                continue
            seen.add(dep)
            name = Path(dep).name
            target = lib_dir / name
            if not target.exists():
                lib_dir.mkdir(parents=True, exist_ok=True)
                shutil.copy2(dep, target)
                relax_permissions(target)
                queue.append(target)
                if platform.system() == "Darwin":
                    patch(target, ["install_name_tool", "-id",
                                   f"@rpath/{name}", str(target)])
                ensure_rpath(target, LIB_RPATH[platform.system()])
            bundled[dep] = name
            repoint(current, dep, name)
        if current == binary and bundled:
            ensure_rpath(binary, EXE_RPATH[platform.system()])
    # install_name_tool rewrites its input through a temporary file, which drops
    # the executable bit; re-normalize after every patch.
    relax_permissions(binary, executable=True)
    for staged_lib in sorted(lib_dir.glob("*")):
        if staged_lib.is_file():
            relax_permissions(staged_lib)
    return sorted(set(bundled.values()))


def write_manifest(stage_root: Path, records, plat: str, sha: str, config,
                   assets: int):
    """records: (target, build exe, staged exe, size, bundled)."""
    lines = [
        f"gdm {plat} build - commit {sha}"
        + (f" - configuration {config}" if config else ""),
        f"executables: {len(records)}",
        f"assets: {assets} files under {ASSET_DIR_NAME}/",
        "",
    ]
    lib_names = sorted({lib for *_, libs in records for lib in libs})
    if lib_names:
        lines.append(f"libraries: {LIB_DIR_NAME}/ (shared by every executable)")
        lines.append("")

    # Column widths come from the content, never hardcoded; the unbounded column
    # goes last so a long name cannot push the size out of alignment.
    name_width = max([len("target")] + [len(r[0]) for r in records])
    size_width = max([len("size")] + [len(r[3]) for r in records])
    indent = " " * (name_width + size_width + 3)
    header = f"{'target':<{name_width}} {'size':>{size_width}}"
    lines.append(header)
    lines.append("-" * len(header))
    for name, _exe, staged, size, bundled in records:
        lines.append(f"{name:<{name_width}} {size:>{size_width}}")
        lines.append(f"{indent}{describe_binary(staged)}")
        if bundled:
            lines.append(f"{indent}loads from {LIB_DIR_NAME}/: "
                         + ", ".join(sorted(set(bundled))))
    lines.append("")
    lines.append("Dependencies of the staged executables:")
    dumped = False
    for _name, _exe, staged, _size, _bundled in records:
        if os.name == "nt" or not Path(staged).is_file():
            continue
        dumped = True
        lines.append(f"  {Path(staged).name}:")
        lines += ["    " + dep for dep in dump_dependencies(staged)]
    if not dumped:
        lines.append("  <not collected on this platform>")
    (stage_root / "MANIFEST.txt").write_text("\n".join(lines) + "\n")


def stage_smoke_tests(stage_root: Path, plat: str):
    """Ship the bulk smoke test script inside the payload, so whoever gets it can
    run every executable at once instead of one by one. Only this platform's
    script is copied - the other is dead weight there."""
    for script in sorted(SCRIPT_DIR.glob("smoke-test.*")):
        if plat == "windows" and script.suffix != ".ps1":
            continue
        if plat != "windows" and script.suffix != ".sh":
            continue
        shutil.copy2(script, stage_root / script.name)
        relax_permissions(stage_root / script.name,
                          executable=script.suffix == ".sh")


def write_readme(stage_root: Path, plat: str, sha: str, config, stripped: bool):
    (stage_root / "README.txt").write_text(README_TEMPLATE.format(
        plat=plat, sha=sha, config=f" - configuration {config}" if config else "",
        assets=ASSET_DIR_NAME,
        exe_note=".exe (Windows)" if plat == "windows" else "",
        strip_note=(" included in the executables, so a crash here can be "
                    "traced locally (strip them if you want the smaller "
                    "payload)" if not stripped else
                    " not included: the executables are stripped, so rebuild "
                    "locally if you need line-accurate tracebacks"),
        requirements=REQUIREMENTS[plat],
    ))


def human_size(num_bytes):
    for unit in ("B", "KB", "MB", "GB"):
        if num_bytes < 1024 or unit == "GB":
            return f"{num_bytes:.0f} {unit}" if unit == "B" else f"{num_bytes:.1f} {unit}"
        num_bytes /= 1024


def git_sha(repo_root):
    try:
        out = subprocess.run(["git", "-C", str(repo_root), "rev-parse",
                              "--short=7", "HEAD"],
                             capture_output=True, text=True, timeout=30)
        return out.stdout.strip() or "local"
    except (OSError, subprocess.SubprocessError):
        return "local"


def collect_dlls(build_dir, config):
    """Every DLL the build produced. Windows loads a DLL from the executable's own
    directory, so nothing has to be repointed - just copied next to the exe."""
    if os.name != "nt":
        return []
    dlls, seen = [], set()
    prune = ("CMakeFiles", "_gdm_deps", "_gdm_extract", "_gdm_downloads")
    for root, dirs, files in os.walk(build_dir):
        if any(part in prune for part in Path(root).parts):
            dirs[:] = []
            continue
        dirs[:] = [d for d in dirs if d not in prune]
        for filename in sorted(files):
            if filename.lower().endswith(".dll") and filename not in seen:
                seen.add(filename)
                dlls.append(Path(root) / filename)
    return dlls


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--repo-root", type=Path, default=REPO_ROOT,
                        help="repository checkout (default: %(default)s)")
    parser.add_argument("--build-dir", type=Path, default=None,
                        help="CMake build dir (default: <repo-root>/build)")
    parser.add_argument("--config", default="",
                        help="multi-config generator configuration, e.g. Debug")
    parser.add_argument("--payload-dir", type=Path, required=True,
                        help="leave the unpacked payload here (replaced if it "
                             "exists); this is what upload-artifact receives")
    parser.add_argument("--out-dir", type=Path, default=None,
                        help="also write a zip here (default: no zip)")
    parser.add_argument("--no-strip", dest="strip", action="store_false",
                        default=True,
                        help="keep debug information in the staged executables "
                             "(default: strip it, which cuts the payload "
                             "substantially; the build tree keeps its symbols)")
    args = parser.parse_args(argv)

    repo_root = args.repo_root.resolve()
    build_dir = args.build_dir or repo_root / "build"
    config = args.config
    plat = PLATFORM_NAMES.get(platform.system(), platform.system().lower())
    sha = git_sha(repo_root)

    cache = read_cache(build_dir)
    targets = collect_targets(repo_root, cache)
    if not targets:
        print("error: no executables to package for this configuration",
              file=sys.stderr)
        return 1

    # --- verify: the "configure and build succeeded" proof -------------------
    print(f"gdm {plat} build{(' - ' + config) if config else ''} "
          f"(commit {sha}, {build_dir})")
    found, missing = [], []
    for name, source_dir in targets:
        exe = find_executable(name, source_dir, build_dir, config, repo_root)
        (found if exe else missing).append((name, source_dir, exe))

    print(f"\n{len(found)}/{len(targets)} executables found:")
    for name, source_dir, exe in found:
        print(f"  OK   {name:<36} {exe}")
    for name, _source_dir, _exe in missing:
        print(f"  FAIL {name:<36} not found")

    if missing:
        print(f"\nerror: {len(missing)} target(s) produced no executable; "
              "the build did not complete", file=sys.stderr)
        return 1

    # --- package --------------------------------------------------------------
    stage_root = args.payload_dir
    if stage_root.exists() and not stage_root.is_dir():
        print(f"error: --payload-dir {stage_root} is not a directory",
              file=sys.stderr)
        return 1
    if stage_root.resolve() == repo_root or stage_root == Path(stage_root.anchor):
        print(f"error: refusing to use --payload-dir {stage_root}",
              file=sys.stderr)
        return 1
    if stage_root.exists():
        shutil.rmtree(stage_root)
    stage_root.mkdir(parents=True)

    dlls = collect_dlls(build_dir, config)
    lib_dir = stage_root / LIB_DIR_NAME
    assets = stage_assets(stage_root, repo_root)

    records = []
    for name, _source_dir, exe in found:
        dest = stage_root / exe.name
        shutil.copy2(exe, dest)
        relax_permissions(dest, executable=True)
        for suffix in (".pdb", ".ilk", ".exp"):
            candidate = exe.with_suffix(suffix)
            if candidate.is_file():
                shutil.copy2(candidate, dest / candidate.name)
            elif config:
                candidate = build_dir / config / (name + suffix)
                if candidate.is_file():
                    shutil.copy2(candidate, dest / candidate.name)
        for dll in dlls:
            target = dest / dll.name
            if not target.exists():
                shutil.copy2(dll, target)
                relax_permissions(target)
        # gdm prefers an already-installed package when one exists, so a machine
        # with Homebrew links shared libraries a clean build does not. Bundle
        # whatever is non-system either way - once, in lib/.
        bundled = bundle_libs(dest, lib_dir)
        records.append((name, exe, dest, human_size(exe.stat().st_size), bundled))

    write_manifest(stage_root, records, plat, sha, config, assets)
    write_readme(stage_root, plat, sha, config, args.strip)
    stage_smoke_tests(stage_root, plat)

    # Final pass by content, not by name: every executable in the payload ends
    # up runnable, and everything else stays writable.
    for path in sorted(stage_root.rglob("*")):
        if not path.is_file():
            continue
        if args.strip and is_executable_file(path):
            strip_debug_info(path)
        relax_permissions(path,
                          executable=is_executable_file(path)
                          or os.access(path, os.X_OK))

    bundled_total = sum(len(r[4]) for r in records)
    if bundled_total:
        names = sorted({lib for *_, libs in records for lib in libs})
        print(f"\n{len(names)} non-system librar(ies) bundled once in "
              f"{LIB_DIR_NAME}/ ({bundled_total} references): " + ", ".join(names))
        if plat == "macos":
            print("         a downloaded copy asks Gatekeeper approval for each of\n"
                  "         these files once; `xattr -dr com.apple.quarantine .`\n"
                  "         in the payload root clears them all at once.",
                  file=sys.stderr)

    if args.out_dir:
        args.out_dir.mkdir(parents=True, exist_ok=True)
        zip_path = args.out_dir / f"gdm-{plat}-{sha}.zip"
        with zipfile.ZipFile(zip_path, "w", zipfile.ZIP_DEFLATED) as zf:
            for path in sorted(stage_root.rglob("*")):
                if path.is_file():
                    zf.write(path, path.relative_to(stage_root))
        # a truncated or CRC-broken entry is what Explorer reports as a
        # "corrupted" directory, so prove the archive before handing it over
        with zipfile.ZipFile(zip_path) as zf:
            corrupt = zf.testzip()
        if corrupt:
            print(f"error: archive entry is corrupt: {corrupt}", file=sys.stderr)
            return 1

    problems = validate_payload(stage_root, plat)
    if problems:
        print("error: the payload is not usable on this platform:",
              file=sys.stderr)
        for problem in problems:
            print(f"  {problem}", file=sys.stderr)
        return 1

    print(f"\npackaged {len(found)} executables, {assets} asset files, "
          f"{bundled_total} non-system librar(ies) -> {stage_root}")
    if args.out_dir:
        print(f"zip: {zip_path} ({human_size(zip_path.stat().st_size)})")
    return 0


if __name__ == "__main__":
    sys.exit(main())
