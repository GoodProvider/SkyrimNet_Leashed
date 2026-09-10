# Stage a FOMOD 7z: ESP + SKSE + Scripts + PrismaUI, no PDBs.
param(
    [Parameter(Mandatory = $true)]
    [string]$Version,
    [string]$Name = "SkyrimNet Leashed",
    [string]$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
)

$ErrorActionPreference = "Stop"
Set-Location $RepoRoot

$pf7z = "C:\Program Files\7-Zip"
if (Test-Path $pf7z) {
    $env:Path = "$pf7z;$env:Path"
}

$requiredPex = @(
    "Scripts\SkyrimNet_Leashed_Actions.pex",
    "Scripts\SkyrimNet_Leashed_PlayerAlias.pex",
    "Scripts\SkyrimNet_Leashed_Native.pex"
)
$missing = @($requiredPex | Where-Object { -not (Test-Path (Join-Path $RepoRoot $_)) })
if ($missing.Count -gt 0) {
    throw ("Missing compiled Papyrus (run compile: pyro and commit the .pex): " + ($missing -join ", "))
}

$dllCandidates = @(
    (Join-Path $RepoRoot "SKSE\Plugins\SkyrimNet_Leashed.dll"),
    (Join-Path $RepoRoot "SKSE_Source\build\release\Release\SkyrimNet_Leashed.dll")
)
$dll = $dllCandidates | Where-Object { Test-Path $_ } | Select-Object -First 1
if (-not $dll) {
    throw "SkyrimNet_Leashed.dll not found. Build Release SKSE (CMake: Build SKSE Release) first."
}

$esp = Join-Path $RepoRoot "SkyrimNet_Leashed.esp"
if (-not (Test-Path $esp)) {
    throw "SkyrimNet_Leashed.esp not found. Run python_scripts/deserialize_esp.ps1 first."
}

$sevenZip = Get-Command 7z -ErrorAction SilentlyContinue
$sevenZipPath = if ($sevenZip) { $sevenZip.Source } else { "C:\Program Files\7-Zip\7z.exe" }
if (-not (Test-Path $sevenZipPath)) {
    throw "7z.exe not found. Install 7-Zip or add 7z to PATH."
}

$stage = Join-Path $RepoRoot "dist\package"
$versionsDir = Join-Path $RepoRoot "versions"
$releaseName = "SkyrimNet_Leashed $Version.7z"
$releaseFile = Join-Path $versionsDir $releaseName

if (Test-Path $stage) { Remove-Item -Recurse -Force $stage }
New-Item -ItemType Directory -Force -Path $stage | Out-Null
New-Item -ItemType Directory -Force -Path $versionsDir | Out-Null

Copy-Item -Path (Join-Path $RepoRoot "FOMOD") -Destination (Join-Path $stage "FOMOD") -Recurse -Force
Copy-Item -Path $esp -Destination (Join-Path $stage "SkyrimNet_Leashed.esp") -Force
Copy-Item -Path (Join-Path $RepoRoot "Scripts") -Destination (Join-Path $stage "Scripts") -Recurse -Force
Copy-Item -Path (Join-Path $RepoRoot "PrismaUI") -Destination (Join-Path $stage "PrismaUI") -Recurse -Force
Copy-Item -Path (Join-Path $RepoRoot "SKSE") -Destination (Join-Path $stage "SKSE") -Recurse -Force

$stageDllDir = Join-Path $stage "SKSE\Plugins"
New-Item -ItemType Directory -Force -Path $stageDllDir | Out-Null
Copy-Item -Path $dll -Destination (Join-Path $stageDllDir "SkyrimNet_Leashed.dll") -Force

Get-ChildItem -Path $stage -Filter *.pdb -Recurse -ErrorAction SilentlyContinue | Remove-Item -Force

if (Test-Path $releaseFile) { Remove-Item -Force $releaseFile }

Push-Location $stage
try {
    & $sevenZipPath -bb1 a $releaseFile -aoa FOMOD SkyrimNet_Leashed.esp SKSE Scripts PrismaUI
    if ($LASTEXITCODE -ne 0) { throw "7z failed with exit $LASTEXITCODE" }
} finally {
    Pop-Location
}

Write-Host "Packed $releaseFile"
Write-Host "DLL source: $dll"
Write-Host "Name: $Name"
