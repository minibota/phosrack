param(
    [string]$BuildDirectory = "build-win",
    [string]$OutputDirectory = "dist"
)
$ErrorActionPreference = "Stop"
$project = (Resolve-Path (Join-Path $PSScriptRoot "../..")).Path
$build = (Resolve-Path (Join-Path $project $BuildDirectory)).Path
$stage = Join-Path $build "windows-deploy"
$output = Join-Path $project $OutputDirectory
$deploy = Get-Command windeployqt.exe -ErrorAction Stop
$qtRoot = Split-Path (Split-Path $deploy.Source -Parent) -Parent
New-Item -ItemType Directory -Force -Path $output | Out-Null

# Clear a previous deployment reference before staging, preventing recursive copies.
& cmake -S $project -B $build "-DPHOSRACK_WINDOWS_DEPLOY_DIR="
if ($LASTEXITCODE -ne 0) { throw "CMake deployment configure failed" }
if (Test-Path $stage) { Remove-Item -Recurse -Force $stage }
& cmake --install $build --config Release --prefix $stage
if ($LASTEXITCODE -ne 0) { throw "Application installation failed" }
$exe = Join-Path $stage "phosrack.exe"
& $deploy.Source --release --qmldir (Join-Path $project "package/contents/ui") --compiler-runtime --translations en,es,fr $exe
if ($LASTEXITCODE -ne 0) { throw "windeployqt failed" }

# Include the CRT app-locally so the portable package does not assume prior setup.
if (-not $env:VCToolsRedistDir) { throw "Run in the MSVC developer environment (VCToolsRedistDir missing)" }
$crt = Get-ChildItem -Path (Join-Path $env:VCToolsRedistDir "x64") -Directory -Filter "Microsoft.VC*.CRT" |
    Sort-Object Name -Descending | Select-Object -First 1
if (-not $crt) { throw "MSVC x64 runtime was not found" }
Copy-Item -Path (Join-Path $crt.FullName "*.dll") -Destination $stage -Force
$offscreen = Join-Path $qtRoot "plugins/platforms/qoffscreen.dll"
if (Test-Path $offscreen) {
    New-Item -ItemType Directory -Force -Path (Join-Path $stage "platforms") | Out-Null
    Copy-Item $offscreen (Join-Path $stage "platforms") -Force
}

$licenses = Join-Path $stage "licenses"
New-Item -ItemType Directory -Force -Path $licenses | Out-Null
Copy-Item (Join-Path $project "LICENSE") (Join-Path $licenses "Phosrack-GPL-3.0.txt")
$qtVersion = & (Join-Path $qtRoot "bin/qmake.exe") -query QT_VERSION
@"
Phosrack is GPL-3.0-or-later.
This deployment uses the GPL open-source option of Qt $qtVersion.
Qt source and license information:
https://download.qt.io/archive/qt/
https://www.qt.io/licensing/open-source-lgpl-obligations
Microsoft Visual C++ runtime DLLs are redistributed from the installed MSVC SDK.
The optional Equalizer APO driver is not included.
"@ | Set-Content -Encoding UTF8 (Join-Path $licenses "Third-party-notices.txt")

# Test the deployed tree without the SDK's PATH or QML/plugin search overrides.
$saved = @{}
foreach ($name in @("PATH", "QT_PLUGIN_PATH", "QML_IMPORT_PATH", "QML2_IMPORT_PATH", "QT_QPA_PLATFORM", "QT_QUICK_BACKEND")) {
    $saved[$name] = [Environment]::GetEnvironmentVariable($name, "Process")
}
try {
    $env:PATH = "$env:SystemRoot/system32;$env:SystemRoot"
    Remove-Item Env:QT_PLUGIN_PATH, Env:QML_IMPORT_PATH, Env:QML2_IMPORT_PATH -ErrorAction SilentlyContinue
    $env:QT_QPA_PLATFORM = "offscreen"
    $env:QT_QUICK_BACKEND = "software"
    foreach ($arguments in @(
        @("--smoke-test", "--settings", "--language", "fr"),
        @("--smoke-test", "--visualizer", "--language", "es")
    )) {
        $process = Start-Process $exe -ArgumentList $arguments -WorkingDirectory $stage -Wait -PassThru
        if ($process.ExitCode -ne 0) { throw "Deployed application smoke test failed: $($process.ExitCode)" }
    }
} finally {
    foreach ($name in $saved.Keys) {
        [Environment]::SetEnvironmentVariable($name, $saved[$name], "Process")
    }
}

& cmake -S $project -B $build "-DPHOSRACK_WINDOWS_DEPLOY_DIR=$stage"
if ($LASTEXITCODE -ne 0) { throw "Final packaging configure failed" }
& cpack --config (Join-Path $build "CPackConfig.cmake") -C Release -G ZIP -B $output
if ($LASTEXITCODE -ne 0) { throw "Portable ZIP packaging failed" }
& cpack --config (Join-Path $build "CPackConfig.cmake") -C Release -G NSIS -B $output
if ($LASTEXITCODE -ne 0) { throw "NSIS installer packaging failed" }
Get-ChildItem $output -File | Where-Object { $_.Extension -in ".zip", ".exe" } | Get-FileHash -Algorithm SHA256
