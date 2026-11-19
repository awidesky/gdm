<#
.SYNOPSIS
    Bulk smoke test for a gdm artifact payload on Windows.

.DESCRIPTION
    Runs every executable in this directory - gdm's example, the glutil examples
    and the glutil tests - and reports whether they ran. They are started from
    this directory, because that is where assets/ lives: the executables resolve
    their shaders, textures and models through the directory next to themselves
    (glutil's exampleAssetDir()/testAssetDir()), so running them from anywhere
    else is the one thing that can make them fail.

    Two of them are not windows: glutil_pathResolveTest and
    glutil_encodingCheckBenchMark print a report and exit on their own.

    Output is never redirected in either mode, so each executable's output lands
    in this console as it produces it. At the end the script waits for Enter, so
    that output does not vanish when the payload is opened by a double-click.

    Interactive (the default): no timeout and no kill - the script waits until you
    close each window (Esc), then moves on to the next one. This is the mode for
    actually using the examples, and for reading what gdm_example_1_HelloWindow
    prints about this machine's OpenGL.

    Timed: let each executable run for the given number of seconds and then kill
    it - a quick pass over the whole set, for when nobody is watching. Surviving
    there proves each starts, finds its assets and loads its libraries, but not
    that it renders.

    A downloaded artifact carries the "Mark of the Web", which blocks the
    executables; the script removes it. Optionally it also adds a Windows
    Defender exclusion, because unsigned executables are scanned on every launch
    (that needs an elevated shell, and is skipped with a warning if it fails).

    "running scripts is disabled on this system" (PSSecurityException): the
    execution policy is checked before this file is read, so nothing in here can
    prevent it. Run smoke-test.cmd next to this file (it does the bypass for you),
    or allow it for one run only - no permanent change - with either:
        powershell -NoProfile -ExecutionPolicy Bypass -File .\smoke-test.ps1
    or, in an already open PowerShell, for that window only:
        Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass -Force

.PARAMETER LogFile
    Positional. Write everything - this script's messages and the executables'
    own output - to this file instead of the console (no "Press Enter" pause).

.PARAMETER List
    Only print the executables that would run.

.PARAMETER Timed
    Opt out of the interactive default: seconds each executable may run before it
    is killed. 0 (the default) means the interactive mode.

.PARAMETER DefenderBypass
    Add an exclusion for this directory to Windows Defender.

.EXAMPLE
    .\smoke-test.ps1
.EXAMPLE
    .\smoke-test.ps1 -DefenderBypass
.EXAMPLE
    .\smoke-test.ps1 -Timed 3
.EXAMPLE
    .\smoke-test.ps1 hello.txt
#>
[CmdletBinding()]
param(
    [string]$LogFile,         # save everything to this file instead of the console
    [switch]$List,
    [int]$Timed = 0,          # 0 = interactive (wait for the window to close)
    [switch]$DefenderBypass
)

# -LogFile: run this same script again with the OS-level redirect `> file 2>&1`.
# The redirect is on the process handles, so the executables' own output lands in
# the file too (Start-Transcript would miss it), unchanged and in order.
if ($LogFile -and -not $env:SMOKE_LOGGING) {
    $LogFile = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($LogFile)
    $fwd = @()
    if ($List) { $fwd += '-List' }
    if ($PSBoundParameters.ContainsKey('Timed')) { $fwd += '-Timed', $Timed }
    if ($DefenderBypass) { $fwd += '-DefenderBypass' }
    $env:SMOKE_LOGGING = '1'
    Write-Host "writing all output to $LogFile"
    cmd /c "powershell -NoProfile -File `"$PSCommandPath`" $fwd > `"$LogFile`" 2>&1"
    exit $LASTEXITCODE
}

$ErrorActionPreference = 'Stop'
# only a value that was actually passed is checked: absent means interactive,
# whose sentinel is the same 0
if ($PSBoundParameters.ContainsKey('Timed') -and $Timed -lt 1) {
    throw '-Timed needs a number of seconds, for example: -Timed 5.'
}
$interactive = ($Timed -eq 0)
$here = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location -LiteralPath $here

function Start-ConsoleProcess {
    # UseShellExecute = $false hands the child this console's own handles, so its
    # output goes straight to the console. Running it through PowerShell (the
    # call operator, or Start-Process) routes the output through PowerShell,
    # which re-emits every line: a native program writing CRLF then gets a blank
    # line after each of its lines.
    param([System.IO.FileInfo]$Exe)
    $startInfo = New-Object System.Diagnostics.ProcessStartInfo
    $startInfo.FileName = $Exe.FullName
    $startInfo.WorkingDirectory = $Exe.DirectoryName
    $startInfo.UseShellExecute = $false
    [System.Diagnostics.Process]::Start($startInfo)
}

# 1. let a downloaded copy run.
#    Remove the Mark of the Web: without it Windows refuses, or prompts for, each
#    file that came out of an archive.
Get-ChildItem -LiteralPath $here -Recurse -File -Force |
    Unblock-File -ErrorAction SilentlyContinue

if ($DefenderBypass) {
    try {
        Add-MpPreference -ExclusionPath $here -ErrorAction Stop
        Write-Host "Defender exclusion added for $here"
    }
    catch {
        Write-Warning "Could not add a Defender exclusion (run in an elevated PowerShell): $($_.Exception.Message)"
    }
}

# 2. the executables: every exe in this directory. The payload layout is flat, so
#    one level is enough, and nothing has to be registered anywhere when gdm
#    gains an example.
$found = Get-ChildItem -LiteralPath $here -Filter *.exe -File |
    Sort-Object Name

if (-not $found) {
    Write-Host "No executables found under $here (wrong directory?)"
    exit 2
}

if ($interactive) {
    Write-Host "smoke test: $(@($found).Count) executables, close each window to move on"
} else {
    Write-Host "smoke test: $(@($found).Count) executables, ${Timed}s each"
}
Write-Host "root: $here"

$passed = 0
$failed = 0
foreach ($exe in $found) {
    if ($List) {
        Write-Host "  $($exe.FullName)"
        continue
    }

    # a separator per executable, so the console can be read - or split at - the
    # boundary between runs
    Write-Host ""
    Write-Host "========== $($exe.Name) =========="
    # every executable is started the same way: attached to this console, so its
    # output appears here in both modes
    try {
        $process = Start-ConsoleProcess $exe
    }
    catch {
        # one executable that cannot even start must not stop the whole run
        $failed++
        Write-Host "FAIL  $($exe.Name) (could not start: $($_.Exception.Message))"
        continue
    }

    if ($interactive) {
        # no timeout and no kill: the script waits until the window is closed
        # (Esc), with the executable writing straight to this console
        $process.WaitForExit()
        if ($process.ExitCode -eq 0) {
            $passed++
            Write-Host "PASS  $($exe.Name) (closed)"
        }
        else {
            $failed++
            Write-Host "FAIL  $($exe.Name) (exit $($process.ExitCode))"
        }
        continue
    }

    Start-Sleep -Seconds $Timed

    if (-not $process.HasExited) {
        # kill the tree: be sure nothing survives a half-open window
        try {
            taskkill /PID $process.Id /T /F 2>&1 | Out-Null
        }
        catch {
            Stop-Process -Id $process.Id -Force -ErrorAction SilentlyContinue
        }
        $passed++
        Write-Host "PASS  $($exe.Name)"
    }
    else {
        # A clean exit is a pass - the examples normally loop until you close
        # them, so exiting at all is unusual, but exiting *successfully* is not
        # a failure of the payload.
        if ($process.ExitCode -eq 0) {
            $passed++
            Write-Host "PASS  $($exe.Name) (exited cleanly)"
        }
        else {
            $failed++
            Write-Host "FAIL  $($exe.Name) (exit $($process.ExitCode))"
        }
    }
}

if ($List) { exit 0 }

Write-Host ''
Write-Host "$passed passed, $failed failed"

# hold the console open so the output above can be read
Write-Host ''
if (-not $env:SMOKE_LOGGING) { try { Read-Host 'Press Enter to exit' | Out-Null } catch { } }

if ($failed -ne 0) { exit 1 }
