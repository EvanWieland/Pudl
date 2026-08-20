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

function Test-NoCrash([string]$Name, [string]$Src, [string]$ExpectedText = "", [string]$ForbiddenText = "") {
    # See run_golden_tests.ps1's Invoke-Pudl for why this goes through
    # cmd.exe rather than PowerShell's own `&`-plus-redirection. Run inside
    # a background job with a timeout so a hang (see
    # malformed_top_level_token.pudl below) fails this test loudly instead
    # of blocking the whole CI job forever.
    $quoted = @($Bin, $Src) | ForEach-Object { '"' + $_ + '"' }
    $cmdLine = $quoted -join ' '

    $job = Start-Job -ScriptBlock {
        param($WorkDir, $Line)
        Set-Location $WorkDir
        $lines = cmd /c "$Line 2>&1"
        [PSCustomObject]@{ Output = ($lines -join "`n"); Code = $LASTEXITCODE }
    } -ArgumentList $RepoRoot, $cmdLine

    if (-not (Wait-Job $job -Timeout 10)) {
        Stop-Job $job | Out-Null
        Remove-Job $job -Force | Out-Null
        Write-Host "FAIL: $Name hung (timed out after 10s)"
        return $false
    }

    $result = Receive-Job $job
    Remove-Job $job | Out-Null
    $output = $result.Output
    $code = $result.Code

    # A crash (access violation, etc.) on Windows reports a large
    # NTSTATUS-style exit code, which PowerShell surfaces as negative when
    # read back as a signed value; a clean (even error-reporting) exit
    # never does.
    if ($code -lt 0) {
        Write-Host "FAIL: $Name crashed (exit code $code)"
        return $false
    }

    if ($ExpectedText -and -not $output.Contains($ExpectedText)) {
        Write-Host "FAIL: $Name did not report the expected error ('$ExpectedText')"
        return $false
    }

    if ($ForbiddenText -and $output.Contains($ForbiddenText)) {
        Write-Host "FAIL: $Name unexpectedly did '$ForbiddenText'"
        return $false
    }

    Write-Host "PASS: $Name did not crash (exit code $code)"
    return $true
}

if (-not (Test-NoCrash "undefined function call" "tests/regression/undefined_function.pudl")) { $fail = $true }
if (-not (Test-NoCrash "undeclared variable assignment" "tests/regression/undeclared_assignment.pudl")) { $fail = $true }
if (-not (Test-NoCrash "unary ! type mismatch" "tests/regression/unary_type_mismatch.pudl" "expected boolean but given number")) { $fail = $true }
if (-not (Test-NoCrash "codegen error must not run" "tests/regression/codegen_error_no_run.pudl" `
        "Codegen failed; not compiling, linking, or running." "Executing -----------------------")) { $fail = $true }
if (-not (Test-NoCrash "malformed top-level token must not hang" "tests/regression/malformed_top_level_token.pudl" `
        "unexpected token")) { $fail = $true }
if (-not (Test-NoCrash "binary operator with missing rhs" "tests/regression/binary_operator_missing_rhs.pudl" `
        "expression expected after")) { $fail = $true }
if (-not (Test-NoCrash "integer literal overflow" "tests/regression/integer_literal_overflow.pudl" `
        "is out of range")) { $fail = $true }
if (-not (Test-NoCrash "variable redeclaration" "tests/regression/variable_redeclaration.pudl" `
        "is already declared")) { $fail = $true }
if (-not (Test-NoCrash "if with non-bool condition" "tests/regression/if_non_bool_condition.pudl" `
        "expected boolean expression")) { $fail = $true }

if ($fail) { exit 1 } else { exit 0 }
