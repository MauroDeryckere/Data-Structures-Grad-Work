Write-Host "=== Clearing benchmark results ==="

$buildDir = Join-Path $PSScriptRoot "build/msvc"
$cache    = Join-Path $buildDir "CMakeCache.txt"

if (!(Test-Path $cache)) {
    Write-Warning "CMakeCache.txt not found. Build first."
    exit 1
}

$resultsDir = Select-String $cache -Pattern "^RESULTS_DIR:PATH=" |
    ForEach-Object { $_.Line.Split("=")[1] }

if (!$resultsDir -or !(Test-Path $resultsDir)) {
    Write-Warning "Results directory not found."
    exit 1
}

Get-ChildItem -Path $resultsDir -Recurse -Force |
    Remove-Item -Recurse -Force -ErrorAction SilentlyContinue

Write-Host "Cleared results in $resultsDir"
