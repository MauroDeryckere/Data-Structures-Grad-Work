<#
.SYNOPSIS
    Builds and runs the benchmark suite with every toolchain installed by install.ps1.

.PARAMETER Only
    Which toolchain legs to run. Any combination of: msvc, gcc, clang, clang-libcxx.
    Defaults to all four.

.PARAMETER Categories
    Optional benchmark category filter (e.g. "Emplace", "Iterate"), forwarded as-is
    to Project.exe. Defaults to running every registered category.

.EXAMPLE
    tools/run_tests.ps1 -Only gcc,clang-libcxx -Categories Iterate,Lookup
#>
param(
    [ValidateSet("msvc", "gcc", "clang", "clang-libcxx")]
    [string[]] $Only = @("msvc", "gcc", "clang", "clang-libcxx"),

    [string[]] $Categories = @()
)

Write-Host "=== Building and running with all available compilers ==="
Write-Host "Toolchains: $($Only -join ', ')"

# Setup paths
$src       = (Resolve-Path "$PSScriptRoot/..").ToString()
$buildRoot = (Join-Path $PSScriptRoot "build").ToString()
$compRoot  = (Join-Path $PSScriptRoot "compilers").ToString()

$gccListFile   = Join-Path $PSScriptRoot "gcc_versions.txt"
$clangListFile = Join-Path $PSScriptRoot "clang_versions.txt"

# ------------------------------
# Helper functions
# ------------------------------
function Clean-Dir($path)
{
    if (Test-Path $path)
    {
        try
        {
            Remove-Item -Recurse -Force $path -ErrorAction Stop
        } catch
        {
            Write-Warning "Could not fully clean $path (maybe exe still running)."
        }
    }
}

function Run-Exe($exePath)
{
    if (Test-Path $exePath)
    {
        Write-Host "Running $exePath $($Categories -join ' ')"
        & $exePath @Categories
    }
    else
    {
        Write-Warning "No exe found at $exePath"
    }
}

function Read-VersionList($listFile)
{
    return Get-Content $listFile | Where-Object { $_.Trim() -ne "" -and !$_.StartsWith("#") } | ForEach-Object {
        $parts = $_ -split "\|"
        @{ Ver = $parts[0].Trim(); Url = $parts[1].Trim() }
    }
}

function Build-WithNinja($buildDir, $ccPath, $cxxPath, $extraCxxFlags)
{
    Clean-Dir $buildDir
    cmake -G "Ninja" -B $buildDir -S $src `
        -DCMAKE_BUILD_TYPE=Release `
        -DCMAKE_C_COMPILER="$ccPath" `
        -DCMAKE_CXX_COMPILER="$cxxPath" `
        -DCMAKE_EXE_LINKER_FLAGS="-static" `
        -DCMAKE_CXX_FLAGS="-O3 -DNDEBUG -march=native -static $extraCxxFlags"

    cmake --build $buildDir --config Release
}
# ------------------------------


# ------------------------------
# MSVC - built via Ninja + a vcvarsall-loaded environment rather than a
# version-pinned "Visual Studio <year>" generator, so this works unchanged
# whether VS2022 (17.x) or VS2026 (18.x, or later) is installed.
# ------------------------------
if ($Only -contains "msvc")
{
    Write-Host "`n[MSVC]..."
    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"

    if (Test-Path $vswhere)
    {
        $vsInstall = & $vswhere -latest -products * `
            -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
            -property installationPath

        if ($vsInstall)
        {
            $vcvars = Join-Path $vsInstall "VC\Auxiliary\Build\vcvars64.bat"
            if (Test-Path $vcvars)
            {
                Write-Host "Using MSVC from $vcvars"

                cmd /c "`"$vcvars`" x64 && set" 2>$null | ForEach-Object {
                    if ($_ -match "^(.*?)=(.*)$") {
                        Set-Item -Force -Path "Env:$($matches[1])" -Value $matches[2]
                    }
                }

                $msvcBuild = Join-Path $buildRoot "msvc"
                $msvcExe   = Join-Path $msvcBuild "bin/Project.exe"
                Clean-Dir $msvcBuild

                cmake -G "Ninja" -B $msvcBuild -S $src `
                    -DCMAKE_BUILD_TYPE=Release `
                    -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded

                cmake --build $msvcBuild --config Release

                Run-Exe $msvcExe
            }
            else
            {
                Write-Warning "vcvars64.bat not found at $vcvars."
            }
        }
        else
        {
            Write-Warning "No Visual Studio with C++ tools found."
        }
    }
    else
    {
        Write-Warning "vswhere.exe not found - skipping MSVC."
    }
}
# ------------------------------


# ------------------------------
# GCC (WinLibs, version-pinned) - libstdc++
# ------------------------------
if ($Only -contains "gcc")
{
    Write-Host "`n[GCC]..."
    $gccVersions = Read-VersionList $gccListFile

    if ($gccVersions.Count -eq 0)
    {
        Write-Warning "No GCC versions listed in $gccListFile."
    }

    foreach ($gcc in $gccVersions)
    {
        $outDir   = Join-Path $compRoot "gcc-$($gcc.Ver)"
        $gccBin   = Join-Path $outDir "mingw64\bin"
        $gccBuild = Join-Path $buildRoot "gcc-$($gcc.Ver)"
        $gccExe   = Join-Path $gccBuild "bin\Project.exe"

        if (!(Test-Path $gccBin))
        {
            Write-Warning "GCC $($gcc.Ver) not found at $gccBin. Run install.ps1 first. Skipping..."
            continue
        }

        Write-Host "=== Building with GCC $($gcc.Ver) ==="
        Build-WithNinja $gccBuild "$gccBin\gcc.exe" "$gccBin\g++.exe" ""

        if (Test-Path $gccExe)
        {
            Run-Exe $gccExe
        }
        else
        {
            Write-Warning "Build for GCC $($gcc.Ver) failed (no exe)."
        }
    }
}
# ------------------------------


# ------------------------------
# Clang (LLVM-Windows release, version-pinned) - MSVC STL
# Requires the MSVC linker (vcvarsall) to be reachable; also requires
# Visual Studio to be installed even though MSVC itself isn't the compiler here.
# ------------------------------
if ($Only -contains "clang")
{
    Write-Host "`n[Clang / MSVC STL]..."

    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path $vswhere)
    {
        $vsInstall = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
        if ($vsInstall)
        {
            $vcvars = Join-Path $vsInstall "VC\Auxiliary\Build\vcvars64.bat"
            if (Test-Path $vcvars)
            {
                cmd /c "`"$vcvars`" x64 && set" 2>$null | ForEach-Object {
                    if ($_ -match "^(.*?)=(.*)$") {
                        Set-Item -Force -Path "Env:$($matches[1])" -Value $matches[2]
                    }
                }
            }
        }
    }

    $clangVersions = Read-VersionList $clangListFile

    if ($clangVersions.Count -eq 0)
    {
        Write-Warning "No Clang versions listed in $clangListFile."
    }

    foreach ($clang in $clangVersions)
    {
        $clangRoot  = Join-Path $compRoot "clang-$($clang.Ver)"
        $clangBuild = Join-Path $buildRoot "clang-$($clang.Ver)"
        $clangExe   = Join-Path $clangBuild "bin/Project.exe"

        $clangBin = Get-ChildItem -Path $clangRoot -Recurse -Directory -Filter "bin" -ErrorAction SilentlyContinue | Select-Object -First 1
        if (!$clangBin)
        {
            Write-Warning "Clang $($clang.Ver) not found under $clangRoot. Run install.ps1 first. Skipping..."
            continue
        }

        Write-Host "=== Building with Clang $($clang.Ver) (MSVC STL) ==="
        Clean-Dir $clangBuild
        cmake -G "Ninja" -B $clangBuild -S $src `
            -DCMAKE_BUILD_TYPE=Release `
            -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded `
            -DCMAKE_C_COMPILER="$(Join-Path $clangBin.FullName 'clang.exe')" `
            -DCMAKE_CXX_COMPILER="$(Join-Path $clangBin.FullName 'clang++.exe')"

        cmake --build $clangBuild --config Release

        if (Test-Path $clangExe)
        {
            Run-Exe $clangExe
        }
        else
        {
            Write-Warning "Build for Clang $($clang.Ver) failed (no exe)."
        }
    }
}
# ------------------------------


# ------------------------------
# Clang (MSYS2 CLANG64) - libc++
# The only leg of the matrix that exercises real libc++ on Windows; the
# LLVM-Windows release above targets the MSVC ABI and links MSVC STL instead.
# ------------------------------
if ($Only -contains "clang-libcxx")
{
    Write-Host "`n[Clang / libc++ (MSYS2 CLANG64)]..."

    $clang64Bin = "C:\msys64\clang64\bin"
    $ccPath  = Join-Path $clang64Bin "clang.exe"
    $cxxPath = Join-Path $clang64Bin "clang++.exe"

    if (!(Test-Path $cxxPath))
    {
        Write-Warning "MSYS2 CLANG64 toolchain not found at $clang64Bin. Run install.ps1 first. Skipping..."
    }
    else
    {
        $oldPath = $env:Path
        $env:Path = "$clang64Bin;$env:Path"

        $libcxxBuild = Join-Path $buildRoot "clang-libcxx"
        $libcxxExe   = Join-Path $libcxxBuild "bin\Project.exe"

        Write-Host "=== Building with Clang (MSYS2 CLANG64, libc++) ==="
        Build-WithNinja $libcxxBuild $ccPath $cxxPath "-stdlib=libc++ -D_LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_NONE"

        if (Test-Path $libcxxExe)
        {
            Run-Exe $libcxxExe
        }
        else
        {
            Write-Warning "Build for Clang/libc++ failed (no exe)."
        }

        $env:Path = $oldPath
    }
}
# ------------------------------

Write-Host "`n=== All builds complete ==="
