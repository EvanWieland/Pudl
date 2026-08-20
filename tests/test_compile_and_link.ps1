# Regression test: the -o (compile-to-object, link, produce an executable)
# path. See test_compile_and_link.sh for the full explanation.
#
# Usage: test_compile_and_link.ps1 -Bin <path-to-pudl-binary>

param(
    [Parameter(Mandatory = $true)]
    [string]$Bin
)

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$RepoRoot = Split-Path -Parent $ScriptDir

Push-Location $RepoRoot
try {
    $outExe = Join-Path $RepoRoot "pudl_test_compile_and_link_exe.exe"
    Remove-Item -Path $outExe, "temp.o", "TempLinker.cpp" -ErrorAction SilentlyContinue

    # See run_golden_tests.ps1's Invoke-Pudl for why this goes through
    # cmd.exe rather than PowerShell's own `&`-plus-redirection -- also
    # means we actually get to see what went wrong below, instead of
    # silently discarding it.
    $quoted = @($Bin, "examples/main.pudl", "-o", "pudl_test_compile_and_link_exe") | ForEach-Object { '"' + $_ + '"' }
    $cmdLine = $quoted -join ' '
    $buildOutput = (cmd /c "$cmdLine 2>&1") -join "`n"

    if (-not (Test-Path $outExe)) {
        Write-Host "FAIL: -o did not produce a runnable executable"
        Write-Host "--- pudl output ---"
        Write-Host $buildOutput
        Remove-Item -Path $outExe, "temp.o", "TempLinker.cpp" -ErrorAction SilentlyContinue
        exit 1
    }

    $actual = ((& $outExe | Out-String) -replace "`r`n", "`n").TrimEnd("`n")
    # The linker's generated wrapper (see Linker.h) is `int main() {
    # std::cout << mast() << std::endl; }` -- mast()'s own return value (0)
    # is always printed as a trailing line on top of whatever mast() itself
    # printed.
    $expected = "1`n10`n0"

    Remove-Item -Path $outExe, "temp.o", "TempLinker.cpp" -ErrorAction SilentlyContinue

    if ($actual -ne $expected) {
        Write-Host "FAIL: compiled+linked executable produced wrong output"
        Write-Host "--- expected ---"
        Write-Host $expected
        Write-Host "--- actual ---"
        Write-Host $actual
        exit 1
    }

    Write-Host "PASS: -o compile+link+run produced the correct output"
    exit 0
} finally {
    Pop-Location
}
