# Regression test for the command-injection fix in Linker.h/Codegen.h.
# See test_no_shell_injection.sh for the full explanation -- same test,
# kept as a separate script rather than requiring bash on Windows CI.
#
# Usage: test_no_shell_injection.ps1 -Bin <path-to-pudl-binary>

param(
    [Parameter(Mandatory = $true)]
    [string]$Bin
)

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$RepoRoot = Split-Path -Parent $ScriptDir
$Marker = Join-Path $ScriptDir "injection_marker_$PID"

Remove-Item -Path $Marker -ErrorAction SilentlyContinue

Push-Location $RepoRoot
try {
    # cmd.exe's metacharacter for command separation is `&`, not `;` --
    # exercise both since either would indicate a shell got involved.
    & $Bin examples/main.pudl -o "pwned_out & type nul > `"$Marker`"" -l "clang++-13 & type nul > `"$Marker`"" *>$null
} finally {
    Pop-Location
}

$fail = $false
if (Test-Path $Marker) {
    Write-Host "FAIL: a shell metacharacter in a CLI value executed an injected command"
    $fail = $true
} else {
    Write-Host "PASS: no shell injection via -o/-l values"
}

Remove-Item -Path $Marker -ErrorAction SilentlyContinue
Remove-Item -Path (Join-Path $RepoRoot "TempLinker.cpp") -ErrorAction SilentlyContinue
Remove-Item -Path (Join-Path $RepoRoot "temp.o") -ErrorAction SilentlyContinue

if ($fail) { exit 1 } else { exit 0 }
