<#
.SYNOPSIS
    Clears benchmark results. Defaults to the repo-local ./results directory,
    matching CMakeLists.txt's default (RESULTS_IN_SOURCE=ON).

.PARAMETER ResultsDir
    Override if you built with -DRESULTS_IN_SOURCE=OFF (out-of-tree results dir).
#>
param(
    [string] $ResultsDir = ""
)

Write-Host "=== Clearing benchmark results ==="

if ($ResultsDir -eq "")
{
    $src = (Resolve-Path "$PSScriptRoot/..").ToString()
    $ResultsDir = Join-Path $src "results"
}

if (!(Test-Path $ResultsDir))
{
    Write-Host "Nothing to clear - $ResultsDir does not exist."
    exit 0
}

Get-ChildItem -Path $ResultsDir -Recurse -Force |
    Remove-Item -Recurse -Force -ErrorAction SilentlyContinue

Write-Host "Cleared results in $ResultsDir"
