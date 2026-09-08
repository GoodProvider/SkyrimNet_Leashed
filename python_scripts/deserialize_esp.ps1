# Deserialize SkyrimNet_Leash.esp from Spriggit/ (source of truth).
param(
    [string]$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
)

$ErrorActionPreference = "Stop"
Set-Location $RepoRoot

$spriggitDir = Join-Path $RepoRoot "Spriggit\SkyrimNet_Leash"
$espOut = Join-Path $RepoRoot "SkyrimNet_Leash.esp"
$cliDir = Join-Path $RepoRoot "SpriggitCLI"
$cli = $null
if (Test-Path $cliDir) {
    $cli = Get-ChildItem -Path $cliDir -Filter "Spriggit.CLI.exe" -Recurse -ErrorAction SilentlyContinue | Select-Object -First 1
}

if (-not (Test-Path $spriggitDir)) {
    throw "Spriggit source not found: $spriggitDir"
}

if ($cli) {
    Write-Host "Using $($cli.FullName)"
    & $cli.FullName convert-to-plugin -i $spriggitDir -o $espOut
    if ($LASTEXITCODE -ne 0) { throw "Spriggit.CLI.exe failed with exit $LASTEXITCODE" }
    return
}

Write-Host "Using dotnet tool spriggit"
dotnet tool run spriggit deserialize -i $spriggitDir -o $espOut
if ($LASTEXITCODE -ne 0) { throw "dotnet tool spriggit failed with exit $LASTEXITCODE" }
