# Golden-file regression tests for Pudl example programs (Windows/PowerShell).
#
# Usage:
#   run_golden_tests.ps1 -Bin <path-to-pudl-binary> [-Record] [-Ir]
#
# See run_golden_tests.sh for full behavior notes — this is the same test
# logic, kept in a separate script rather than requiring bash on Windows CI.

param(
    [Parameter(Mandatory = $true)]
    [string]$Bin,

    [switch]$Record,
    [switch]$Ir
)

$ErrorActionPreference = "Stop"

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$RepoRoot = Split-Path -Parent $ScriptDir
$ExamplesDir = Join-Path $RepoRoot "examples"
$GoldenDir = Join-Path $ScriptDir "golden"
$GoldenIrDir = Join-Path $ScriptDir "golden-ir"
$KnownBrokenFile = Join-Path $ScriptDir "KNOWN_BROKEN.md"

$IrSubset = @("main", "ex1", "ex5")

if (-not (Test-Path $Bin)) {
    Write-Error "pudl binary not found: $Bin"
    exit 2
}

function Invoke-Pudl {
    param([string[]]$PudlArgs)
    # PowerShell's own `&`-plus-`2>&1` on a native command wraps every
    # stderr line in a NativeCommandError record -- and, worse, under
    # $ErrorActionPreference = "Stop" (set above) that becomes a
    # terminating exception the instant pudl writes anything to stderr,
    # which -p/--print-ir does (the IR dump goes to stderr when no output
    # file is given), aborting the capture mid-stream. Let cmd.exe do the
    # low-level stream merge instead of PowerShell, so only ordinary text
    # ever reaches PowerShell.
    $quoted = (@($Bin) + $PudlArgs) | ForEach-Object { '"' + $_ + '"' }
    $cmdLine = $quoted -join ' '
    $lines = cmd /c "$cmdLine 2>&1"
    return ($lines -join "`n")
}

function Test-KnownBroken([string]$Name) {
    if (-not (Test-Path $KnownBrokenFile)) { return $false }
    $pattern = "``" + $Name
    return (Select-String -Path $KnownBrokenFile -Pattern ([regex]::Escape($pattern)) -Quiet)
}

function Invoke-CheckOrRecord([string]$Name, [string]$ExpectedPath, [string]$Actual) {
    # Normalize CRLF -> LF so a Windows build's console output can't
    # spuriously mismatch a golden file recorded on Linux (or vice versa)
    # over line endings alone. Also strip ALL trailing newlines: bash's
    # $(...) command substitution (used by run_golden_tests.sh, which
    # recorded these baselines) does this unconditionally, and Invoke-Pudl's
    # line-join can leave one trailing newline of its own if the program's
    # last line of output was blank. Without stripping both sides the same
    # way, a golden file could spuriously mismatch by one invisible
    # trailing newline.
    $Actual = ($Actual -replace "`r`n", "`n").TrimEnd("`n")

    if ($Record) {
        Set-Content -Path $ExpectedPath -Value $Actual -NoNewline
        Write-Host "recorded: $Name"
        return $true
    }

    if (-not (Test-Path $ExpectedPath)) {
        Write-Host "FAIL: $Name (no golden file recorded -- run with -Record first)"
        return $false
    }

    $expectedContent = Get-Content -Path $ExpectedPath -Raw
    if ($null -eq $expectedContent) { $expectedContent = "" }
    $expectedContent = ($expectedContent -replace "`r`n", "`n").TrimEnd("`n")

    if ($Actual -eq $expectedContent) {
        Write-Host "PASS: $Name"
        return $true
    }

    if (Test-KnownBroken $Name) {
        Write-Host "KNOWN-BROKEN (mismatch, not failing): $Name"
        return $true
    }

    Write-Host "FAIL: $Name"
    Write-Host "--- expected ---"
    Write-Host $expectedContent
    Write-Host "--- actual ---"
    Write-Host $Actual
    return $false
}

$fail = $false

# Run from the repo root and pass example paths relative to it (e.g.
# "examples/main.pudl"), never absolute -- the CLI echoes back whatever path
# it was given ("Loading source file ..."), and an absolute path would bake
# this machine's checkout location into the golden files, breaking on every
# other machine (including CI).
Push-Location $RepoRoot
try {
    if ($Ir) {
        New-Item -ItemType Directory -Force -Path $GoldenIrDir | Out-Null
        foreach ($name in $IrSubset) {
            $rel = "examples/$name.pudl"
            if (-not (Test-Path $rel)) {
                Write-Host "FAIL: $name (source not found: $rel)"
                $fail = $true
                continue
            }
            $actual = Invoke-Pudl -PudlArgs @($rel, "-p")
            $ok = Invoke-CheckOrRecord -Name $name -ExpectedPath (Join-Path $GoldenIrDir "$name.ir.txt") -Actual $actual
            if (-not $ok) { $fail = $true }
        }
    } else {
        New-Item -ItemType Directory -Force -Path $GoldenDir | Out-Null
        Get-ChildItem -Path $ExamplesDir -Filter "*.pudl" | ForEach-Object {
            $name = $_.BaseName
            $rel = "examples/$name.pudl"
            $actual = Invoke-Pudl -PudlArgs @($rel)
            $ok = Invoke-CheckOrRecord -Name $name -ExpectedPath (Join-Path $GoldenDir "$name.expected.txt") -Actual $actual
            if (-not $ok) { $fail = $true }
        }
    }
} finally {
    Pop-Location
}

if ($fail) { exit 1 } else { exit 0 }
