Write-Host "=== Installing compilers and build tools ==="

$root = "$PSScriptRoot/compilers"

$gccListFile = Join-Path $PSScriptRoot "gcc_versions.txt"
$clangListFile = Join-Path $PSScriptRoot "clang_versions.txt"

New-Item -ItemType Directory -Force -Path $root | Out-Null

# ------------------------------
# Helper functions
# ------------------------------
function Download-File($url, $dest)
{
    Write-Host "Downloading: $url"
    [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
    & curl.exe -L --retry 3 --retry-delay 5 --ssl-no-revoke -o $dest $url

    if (!(Test-Path $dest))
    {
        throw "Failed to download $url"
    }
}

# Extraction relies solely on the tar.exe (bsdtar/libarchive) shipped with Windows
# 10 1803+ / Windows 11, which natively handles .zip and .tar.xz. This removes the
# previous hard dependency on a manually pre-installed 7-Zip.
function Extract-ArchiveSafe($file, $dest)
{
    New-Item -ItemType Directory -Force -Path $dest | Out-Null

    if (!(Get-Command tar.exe -ErrorAction SilentlyContinue))
    {
        throw "tar.exe not found. It ships with Windows 10 1803+ / Windows 11 by default; please update Windows or install bsdtar."
    }

    & tar.exe -xf $file -C $dest
    if ($LASTEXITCODE -ne 0)
    {
        throw "Failed to extract $file"
    }
}
# ------------------------------

# ------------------------------
# MSVC
# ------------------------------
Write-Host "`n[1/4] Checking for Visual Studio (MSVC)..."

$vswhere = "C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe"

if (Test-Path $vswhere)
{
    # No upper version bound: any VS with the C++ desktop toolset works (VS2022 17.x,
    # VS2026 18.x, ...). run_tests.ps1 builds MSVC via Ninja + vcvarsall, so it does not
    # depend on a specific VS "year" generator either.
    $vsInfo = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath 2>$null

    if ($vsInfo)
    {
        $vcvarsPath = Join-Path $vsInfo "VC\Auxiliary\Build\vcvars64.bat"
        if (Test-Path $vcvarsPath)
        {
            $vsVersion = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationVersion 2>$null
            Write-Host "Visual Studio detected at: $vsInfo (version $vsVersion)"
            Write-Host "vcvars64.bat path: $vcvarsPath"
        }
        else
        {
            Write-Warning "Visual Studio found, but vcvars64.bat missing."
        }
    }
    else
    {
        Write-Warning "No Visual Studio with the C++ (VC.Tools.x86.x64) workload found. You can install it via:"
        Write-Host " winget install Microsoft.VisualStudio.2022.Community --silent --accept-package-agreements --accept-source-agreements"
        Write-Host " (then add the 'Desktop development with C++' workload, e.g. via tools/configs/msvc-2022-14.38.33135.vsconfig)"
    }
}
else
{
    Write-Warning "vswhere.exe not found! Visual Studio Installer may not be installed."
}
# ------------------------------


# ------------------------------
# GCC (WinLibs, version-pinned only)
# ------------------------------
Write-Host "`n[2/4] Installing GCC (WinLibs, pinned versions from gcc_versions.txt)..."

$gccVersions = Get-Content $gccListFile | Where-Object { $_.Trim() -ne "" -and !$_.StartsWith("#") } | ForEach-Object {
    $parts = $_ -split "\|"
    @{
        Ver = $parts[0].Trim()
        Url = $parts[1].Trim()
    }
}

foreach ($gcc in $gccVersions)
{
    $outDir = Join-Path $root "gcc-$($gcc.Ver)"
    if (!(Test-Path $outDir))
    {
        $zipFile = "$outDir.zip"
        Download-File $gcc.Url $zipFile
        Extract-ArchiveSafe $zipFile $outDir
        Remove-Item $zipFile
        Write-Host "GCC $($gcc.Ver) installed at $outDir"
    }
    else
    {
        Write-Host "GCC $($gcc.Ver) already installed at $outDir"
    }
}
# ------------------------------


# ------------------------------
# Clang (LLVM-Windows release, tests Clang + MSVC STL - see readme for why)
# ------------------------------
Write-Host "`n[3/4] Installing Clang/LLVM (Windows release, pinned versions from clang_versions.txt)..."

$clangVersions = Get-Content $clangListFile | Where-Object { $_.Trim() -ne "" -and !$_.StartsWith("#") } | ForEach-Object {
    $parts = $_ -split "\|"
    @{
        Ver = $parts[0].Trim()
        Url = $parts[1].Trim()
    }
}

foreach ($clang in $clangVersions)
{
    $outDir = Join-Path $root "clang-$($clang.Ver)"
    if (Test-Path $outDir)
    {
        Write-Host "Clang/LLVM $($clang.Ver) already installed at $outDir"
        continue
    }

    New-Item -ItemType Directory -Path $outDir | Out-Null

    $tarUrl = "$($clang.Url)/clang+llvm-$($clang.Ver)-x86_64-pc-windows-msvc.tar.xz"
    $downloadFile = Join-Path $outDir "clang.tar.xz"

    try
    {
        Write-Host "Downloading Clang $($clang.Ver) from $tarUrl..."
        Download-File $tarUrl $downloadFile

        Write-Host "Extracting Clang $($clang.Ver)..."
        Extract-ArchiveSafe $downloadFile $outDir
        Remove-Item $downloadFile -Force
        Write-Host "Clang $($clang.Ver) installed at $outDir"
    }
    catch
    {
        Write-Warning "Failed to install Clang $($clang.Ver): $_"
        Remove-Item -Recurse -Force $outDir -ErrorAction SilentlyContinue
    }
}
# ------------------------------


# ------------------------------
# MSYS2 (hosts the CLANG64 environment for genuine Clang + libc++ on Windows -
# the LLVM-Windows release above links against MSVC STL, not libc++, so this is
# the only leg of the matrix that exercises libc++ at all)
# ------------------------------
Write-Host "`n[4/4] Installing MSYS2 and the CLANG64 (Clang + libc++) toolchain..."

$msysPath = "C:\msys64\usr\bin\bash.exe"

winget install MSYS2.MSYS2 --silent --accept-package-agreements --accept-source-agreements

if (Test-Path $msysPath)
{
    & $msysPath -lc "pacman -Syu --noconfirm"
    & $msysPath -lc "pacman -S --noconfirm --needed mingw-w64-clang-x86_64-toolchain mingw-w64-clang-x86_64-libc++"
    Write-Host "MSYS2 CLANG64 (Clang + libc++) installed."
}
else
{
    Write-Warning "MSYS2 install did not produce $msysPath - Clang+libc++ leg will be unavailable."
}
# ------------------------------


# ------------------------------
# Build Tools (CMake + Ninja, installed once via winget)
# ------------------------------
Write-Host "`n[Extra] Installing CMake and Ninja..."
winget install Kitware.CMake --silent --accept-package-agreements --accept-source-agreements
winget install Ninja-build.Ninja --silent --accept-package-agreements --accept-source-agreements
# ------------------------------


# ------------------------------
# Summary
# ------------------------------
Write-Host "`n=== Setup Complete ==="
Write-Host "MSVC:            available via vcvarsall.bat (MSVC STL)"
Write-Host "GCC:             WinLibs, pinned versions under $root (libstdc++)"
Write-Host "Clang:           LLVM-Windows release, pinned versions under $root (MSVC STL)"
Write-Host "Clang (libc++):  MSYS2 CLANG64 environment at C:\msys64\clang64 (libc++)"
Write-Host "CMake & Ninja installed system-wide via winget"
Write-Host "`nRun tools/run_tests.ps1 next to build and benchmark all available toolchains."
# ------------------------------
