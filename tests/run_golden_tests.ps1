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

function Test-KnownBroken([string]$Name) {
    if (-not (Test-Path $KnownBrokenFile)) { return $false }
    $pattern = "``" + $Name
    return (Select-String -Path $KnownBrokenFile -Pattern ([regex]::Escape($pattern)) -Quiet)
}

function Invoke-CheckOrRecord([string]$Name, [string]$ExpectedPath, [string]$Actual) {
    # Normalize CRLF -> LF so a Windows build's console output can't
    # spuriously mismatch a golden file recorded on Linux (or vice versa)
    # over line endings alone.
    $Actual = $Actual -replace "`r`n", "`n"

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
    $expectedContent = $expectedContent -replace "`r`n", "`n"

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
            $actual = (& $Bin $rel -p 2>&1 | Out-String)
            $ok = Invoke-CheckOrRecord -Name $name -ExpectedPath (Join-Path $GoldenIrDir "$name.ir.txt") -Actual $actual
            if (-not $ok) { $fail = $true }
        }
    } else {
        New-Item -ItemType Directory -Force -Path $GoldenDir | Out-Null
        Get-ChildItem -Path $ExamplesDir -Filter "*.pudl" | ForEach-Object {
            $name = $_.BaseName
            $rel = "examples/$name.pudl"
            $actual = (& $Bin $rel 2>&1 | Out-String)
            $ok = Invoke-CheckOrRecord -Name $name -ExpectedPath (Join-Path $GoldenDir "$name.expected.txt") -Actual $actual
            if (-not $ok) { $fail = $true }
        }
    }
} finally {
    Pop-Location
}

if ($fail) { exit 1 } else { exit 0 }
