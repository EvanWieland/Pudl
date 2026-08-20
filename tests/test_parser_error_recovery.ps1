# Regression test: malformed programs that trigger a parser error path must
# be reported cleanly and exit normally, not crash the compiler.
# See test_parser_error_recovery.sh for the full explanation.
#
# Usage: test_parser_error_recovery.ps1 -Bin <path-to-pudl-binary>

param(
    [Parameter(Mandatory = $true)]
    [string]$Bin
)

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$RepoRoot = Split-Path -Parent $ScriptDir

$fail = $false

function Test-NoCrash([string]$Name, [string]$Src) {
    Push-Location $RepoRoot
    try {
        & $Bin $Src *> $null
        $code = $LASTEXITCODE
    } finally {
        Pop-Location
    }

    # A crash (access violation, etc.) on Windows reports a large
    # NTSTATUS-style exit code, which PowerShell surfaces as negative when
    # read back as a signed value; a clean (even error-reporting) exit
    # never does.
    if ($code -lt 0) {
        Write-Host "FAIL: $Name crashed (exit code $code)"
        return $false
    }
    Write-Host "PASS: $Name did not crash (exit code $code)"
    return $true
}

if (-not (Test-NoCrash "undefined function call" "tests/regression/undefined_function.pudl")) { $fail = $true }
if (-not (Test-NoCrash "undeclared variable assignment" "tests/regression/undeclared_assignment.pudl")) { $fail = $true }

if ($fail) { exit 1 } else { exit 0 }
