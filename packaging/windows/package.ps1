param(
    [string]$BuildDirectory = "build-win",
    [string]$OutputDirectory = "dist",
    [switch]$VerifyInstaller
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
& $deploy.Source --release --webenginecore --webchannel --qmldir (Join-Path $project "package/contents/ui") --compiler-runtime --translations en,es,fr $exe
if ($LASTEXITCODE -ne 0) { throw "windeployqt failed" }

# A working QML shell alone does not prove the Chromium deployment is complete.
$webEngineFiles = @(
    "Qt6WebEngineCore.dll", "Qt6WebChannel.dll", "QtWebEngineProcess.exe",
    "resources/qtwebengine_resources.pak", "resources/qtwebengine_resources_100p.pak",
    "resources/qtwebengine_resources_200p.pak", "resources/icudtl.dat",
    "resources/v8_context_snapshot.bin", "translations/qtwebengine_locales/en-US.pak"
)
foreach ($relative in $webEngineFiles) {
    if (-not (Test-Path (Join-Path $stage $relative))) {
        throw "Missing Qt WebEngine deployment file: $relative"
    }
}
if (Get-ChildItem $stage -Recurse -File -Filter "widevinecdm.dll") {
    throw "Widevine must not be redistributed in Phosrack packages"
}

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
Qt WebEngine includes Chromium and third-party components covered by their
respective licenses: https://doc.qt.io/qt-6/qtwebengine-licensing.html
Widevine is not included. Integrated Spotify playback needs a compatible
separately installed Widevine CDM; see Phosrack's setup documentation.
"@ | Set-Content -Encoding UTF8 (Join-Path $licenses "Third-party-notices.txt")

# Test a deployed tree without the SDK's PATH or QML/plugin search overrides.
function Test-DeployedApplication([string]$Executable, [string]$Directory) {
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
            $process = Start-Process $Executable -ArgumentList $arguments -WorkingDirectory $Directory -PassThru
            if (-not $process.WaitForExit(20000)) {
                $process.Kill()
                throw "Deployed application smoke test timed out: $Executable"
            }
            if ($process.ExitCode -ne 0) { throw "Deployed application smoke test failed: $($process.ExitCode)" }
        }
    } finally {
        foreach ($name in $saved.Keys) {
            [Environment]::SetEnvironmentVariable($name, $saved[$name], "Process")
        }
    }
}
Test-DeployedApplication $exe $stage

& cmake -S $project -B $build "-DPHOSRACK_WINDOWS_DEPLOY_DIR=$stage"
if ($LASTEXITCODE -ne 0) { throw "Final packaging configure failed" }
& cpack --config (Join-Path $build "CPackConfig.cmake") -C Release -G ZIP -B $output
if ($LASTEXITCODE -ne 0) { throw "Portable ZIP packaging failed" }

# Validate the final package layout, not only the pre-CPack deployment tree.
$packageNameMatch = [regex]::Match((Get-Content (Join-Path $build "CPackConfig.cmake") -Raw), '(?m)^set\(CPACK_PACKAGE_FILE_NAME "([^"]+)"\)\s*$')
if (-not $packageNameMatch.Success) { throw "Cannot determine the generated package name" }
$packageName = $packageNameMatch.Groups[1].Value
$zipPath = Join-Path $output "$packageName.zip"
$archive = [System.IO.Compression.ZipFile]::OpenRead($zipPath)
try {
    $entries = @($archive.Entries | ForEach-Object { $_.FullName.Replace('\', '/') })
    foreach ($relative in (@("phosrack.exe", "platforms/qwindows.dll") + $webEngineFiles)) {
        if ($entries -notcontains "$packageName/$relative") {
            throw "Portable ZIP is missing $relative at its application root; check CPACK_PACKAGING_INSTALL_PREFIX"
        }
    }
    if ($entries -contains "$packageName/usr/phosrack.exe") {
        throw "Portable ZIP incorrectly nests the Windows application under usr/"
    }
} finally {
    $archive.Dispose()
}
Write-Host "Portable ZIP layout verified: $packageName/phosrack.exe"

& cpack --config (Join-Path $build "CPackConfig.cmake") -C Release -G NSIS -B $output
if ($LASTEXITCODE -ne 0) { throw "NSIS installer packaging failed" }

# Explicit opt-in: installation changes Windows shortcuts/registry, so this check
# runs on the disposable CI runner rather than during ordinary local packaging.
if ($VerifyInstaller) {
    $installProbe = Join-Path ([System.IO.Path]::GetTempPath()) ("phosrack-install-test-" + [guid]::NewGuid().ToString("N"))
    $installer = Join-Path $output "$packageName.exe"
    try {
        $process = Start-Process $installer -ArgumentList @("/S", "/D=$installProbe") -PassThru
        if (-not $process.WaitForExit(120000)) {
            $process.Kill()
            throw "Silent NSIS installation timed out"
        }
        if ($process.ExitCode -ne 0) { throw "Silent NSIS installation failed: $($process.ExitCode)" }
        $installedExe = Join-Path $installProbe "phosrack.exe"
        foreach ($relative in (@("phosrack.exe") + $webEngineFiles)) {
            if (-not (Test-Path (Join-Path $installProbe $relative))) {
                throw "Installed package is missing $relative at the selected installation root"
            }
        }
        Test-DeployedApplication $installedExe $installProbe

        # A runnable EXE is insufficient if the generated Start-menu link is wrong.
        $shell = New-Object -ComObject WScript.Shell
        $programFolders = @([Environment]::GetFolderPath("CommonPrograms"), [Environment]::GetFolderPath("Programs"))
        $shortcuts = @($programFolders | Where-Object { $_ } | ForEach-Object {
            Get-ChildItem $_ -Recurse -File -Filter "Phosrack.lnk" -ErrorAction SilentlyContinue
        })
        $expectedTarget = [System.IO.Path]::GetFullPath($installedExe)
        $validShortcut = $false
        foreach ($shortcut in $shortcuts) {
            $target = $shell.CreateShortcut($shortcut.FullName).TargetPath
            if ($target -and [System.IO.Path]::GetFullPath($target) -eq $expectedTarget) {
                $validShortcut = $true
            }
        }
        if (-not $validShortcut) { throw "The installed Start-menu shortcut does not target $installedExe" }
        Write-Host "NSIS installation, launch and Start-menu shortcut verified"
    } finally {
        $uninstaller = Join-Path $installProbe "Uninstall.exe"
        if (Test-Path $uninstaller) {
            $cleanup = Start-Process $uninstaller -ArgumentList @("/S", "_?=$installProbe") -PassThru
            if (-not $cleanup.WaitForExit(30000)) { $cleanup.Kill() }
        }
        if (Test-Path $installProbe) { Remove-Item -Recurse -Force $installProbe }
    }
}
Get-ChildItem $output -File | Where-Object { $_.Extension -in ".zip", ".exe" } | Get-FileHash -Algorithm SHA256
