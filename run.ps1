<#
.SYNOPSIS
    One-Click Build, Test & Run Script for Multithreaded File Search Engine.
.DESCRIPTION
    Automatically detects CMake, Ninja, and Visual Studio / MSVC build environment,
    configures CMake, builds the project, runs all 57 tests, and launches the search engine.
#>

param(
    [switch]$TestOnly,
    [switch]$NoInteractive
)

$ErrorActionPreference = "Stop"
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Definition
Set-Location $ScriptDir

Write-Host "==========================================================" -ForegroundColor Cyan
Write-Host "   MULTITHREADED FILE SEARCH ENGINE - ONE-CLICK LAUNCHER  " -ForegroundColor Cyan
Write-Host "==========================================================" -ForegroundColor Cyan

# 1. Ensure CMake & Ninja are in PATH
$PythonScripts = "$env:USERPROFILE\anaconda3\Scripts"
$CMakeDataBin = "$env:USERPROFILE\anaconda3\Lib\site-packages\cmake\data\bin"

if (Test-Path $PythonScripts) {
    if ($env:PATH -notlike "*$PythonScripts*") {
        $env:PATH = "$PythonScripts;$env:PATH"
    }
}
if (Test-Path $CMakeDataBin) {
    if ($env:PATH -notlike "*$CMakeDataBin*") {
        $env:PATH = "$CMakeDataBin;$env:PATH"
    }
}

# 2. Locate Visual Studio vcvars / VsDevCmd
$VsDevCmd = $null
$CommonVsPaths = @(
    "C:\Program Files\Microsoft Visual Studio\18\Insiders\Common7\Tools\VsDevCmd.bat",
    "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat",
    "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat",
    "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\Common7\Tools\VsDevCmd.bat",
    "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat",
    "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\Common7\Tools\VsDevCmd.bat",
    "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\Common7\Tools\VsDevCmd.bat"
)

foreach ($p in $CommonVsPaths) {
    if (Test-Path $p) {
        $VsDevCmd = $p
        break
    }
}

if (-not $VsDevCmd) {
    # Try finding with vswhere
    $vswhere = "C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path $vswhere) {
        $installPath = & $vswhere -latest -prerelease -products * -property installationPath
        if ($installPath -and (Test-Path "$installPath\Common7\Tools\VsDevCmd.bat")) {
            $VsDevCmd = "$installPath\Common7\Tools\VsDevCmd.bat"
        }
    }
}

# 3. Check or Build Executables
$SearchExe = Join-Path $ScriptDir "build\bin\search_engine.exe"
$TestExe   = Join-Path $ScriptDir "build\bin\run_tests.exe"

$NeedBuild = (-not (Test-Path $SearchExe)) -or (-not (Test-Path $TestExe))

if ($NeedBuild) {
    Write-Host "`n[*] Executables not found. Building project..." -ForegroundColor Yellow
    if ($VsDevCmd) {
        Write-Host "[*] Using Visual Studio Environment: $VsDevCmd" -ForegroundColor Gray
        $buildCmd = "call `"$VsDevCmd`" -arch=x64 && cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release && cmake --build build"
        cmd.exe /c $buildCmd
    } else {
        # Fallback to direct cmake invocation if already in a configured environment
        cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
        cmake --build build
    }

    if (-not (Test-Path $SearchExe)) {
        Write-Host "`n[!] Build failed. Please verify compiler installation." -ForegroundColor Red
        exit 1
    }
    Write-Host "[+] Build completed successfully!" -ForegroundColor Green
} else {
    Write-Host "[+] Engine and tests already built and ready!" -ForegroundColor Green
}

# 4. Run Automated Tests
Write-Host "`n[*] Running automated tests..." -ForegroundColor Yellow
& $TestExe

if ($LASTEXITCODE -ne 0) {
    Write-Host "`n[!] Tests failed." -ForegroundColor Red
    exit $LASTEXITCODE
}
Write-Host "[+] All tests passed!" -ForegroundColor Green

if ($TestOnly) {
    exit 0
}

# 5. Launch Search Engine
Write-Host "`n[*] Starting Multithreaded File Search Engine..." -ForegroundColor Cyan
Write-Host "==========================================================" -ForegroundColor Cyan
if (-not $NoInteractive) {
    & $SearchExe
}
