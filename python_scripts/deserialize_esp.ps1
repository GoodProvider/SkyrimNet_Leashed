# Deserialize every plugin under Spriggit/ (source of truth) to the repo root.
# Each Spriggit/<folder>/spriggit-meta.json names its ModKey (output file name).
param(
    [string]$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
)

$ErrorActionPreference = "Stop"
Set-Location $RepoRoot

$spriggitRoot = Join-Path $RepoRoot "Spriggit"
$cliDir = Join-Path $RepoRoot "SpriggitCLI"
$cli = $null
if (Test-Path $cliDir) {
    $cli = Get-ChildItem -Path $cliDir -Filter "Spriggit.CLI.exe" -Recurse -ErrorAction SilentlyContinue | Select-Object -First 1
}

$spriggitDirs = @(Get-ChildItem -Path $spriggitRoot -Directory -ErrorAction SilentlyContinue | Where-Object { Test-Path (Join-Path $_.FullName "spriggit-meta.json") })
if ($spriggitDirs.Count -eq 0) {
    throw "No Spriggit sources found under $spriggitRoot"
}

foreach ($dir in $spriggitDirs) {
    $meta = Get-Content (Join-Path $dir.FullName "spriggit-meta.json") -Raw | ConvertFrom-Json
    $espOut = Join-Path $RepoRoot $meta.ModKey
    Write-Host "Deserializing $($dir.Name) -> $($meta.ModKey)"
    if ($cli) {
        & $cli.FullName convert-to-plugin -i $dir.FullName -o $espOut
        if ($LASTEXITCODE -ne 0) { throw "Spriggit.CLI.exe failed with exit $LASTEXITCODE for $($dir.Name)" }
    } else {
        dotnet tool run spriggit deserialize -i $dir.FullName -o $espOut
        if ($LASTEXITCODE -ne 0) { throw "dotnet tool spriggit failed with exit $LASTEXITCODE for $($dir.Name)" }
    }
}
