# Build script for Game Trainers.
# Discovers trainers from trainers/*/CMakeLists.txt. Each trainer CMake file must
# define GAME_NAME, TRAINER_NAME, and TRAINER_ARCH.

param(
    [string]$Trainer = "",

    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Debug",

    [switch]$All,
    [switch]$Run,
    [switch]$List
)

$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$buildDir = Join-Path $scriptDir "build"

function Get-CMakeSetValue {
    param(
        [string]$Content,
        [string]$Name
    )

    $pattern = "(?m)^\s*set\s*\(\s*$([regex]::Escape($Name))\s+`"?([^`"`r`n\)]+)`"?\s*\)"
    $match = [regex]::Match($Content, $pattern)
    if (-not $match.Success) {
        return $null
    }

    return $match.Groups[1].Value.Trim()
}

function Get-Trainers {
    $trainersDir = Join-Path $scriptDir "trainers"
    if (-not (Test-Path -LiteralPath $trainersDir)) {
        throw "Trainers folder not found: $trainersDir"
    }

    Get-ChildItem -LiteralPath $trainersDir -Directory |
        ForEach-Object {
            $cmakeFile = Join-Path $_.FullName "CMakeLists.txt"
            if (-not (Test-Path -LiteralPath $cmakeFile)) {
                return
            }

            $content = Get-Content -LiteralPath $cmakeFile -Raw
            $target = Get-CMakeSetValue $content "GAME_NAME"
            $name = Get-CMakeSetValue $content "TRAINER_NAME"
            $arch = Get-CMakeSetValue $content "TRAINER_ARCH"

            # Trainers written against add_trainer() derive their names from the folder and declare
            # ARCH in the manifest. Older ones still set all three themselves.
            if (-not $arch -and $content -match 'add_trainer\s*\(') {
                $name = $_.Name
                $target = ($name -replace '[^A-Za-z0-9_]', '_')
                $arch = if ($content -match 'ARCH\s+"?([A-Za-z0-9_]+)"?') { $Matches[1] } else { $null }
            }

            if (-not $target -or -not $name -or -not $arch) {
                Write-Warning "Skipping $($_.Name): CMakeLists.txt must define GAME_NAME, TRAINER_NAME, and TRAINER_ARCH."
                return
            }

            [pscustomobject]@{
                Folder = $_.Name
                Name = $name
                Target = $target
                Architecture = $arch
            }
        } |
        Sort-Object Name
}

function Get-Platform {
    param([string]$Architecture)

    switch ($Architecture.ToLowerInvariant()) {
        "x86" { return "Win32" }
        "x64" { return "x64" }
        default { throw "Unsupported architecture '$Architecture'. Use x86 or x64." }
    }
}

function Write-MenuLine {
    param(
        [int]$Index,
        [object]$TrainerInfo
    )

    $archColor = if ($TrainerInfo.Architecture -eq "x64") { "Cyan" } else { "Yellow" }

    Write-Host ("  {0,2}. " -f $Index) -NoNewline -ForegroundColor DarkGray
    Write-Host $TrainerInfo.Name -NoNewline -ForegroundColor White
    Write-Host " [" -NoNewline -ForegroundColor DarkGray
    Write-Host $TrainerInfo.Architecture -NoNewline -ForegroundColor $archColor
    Write-Host "] " -NoNewline -ForegroundColor DarkGray
    Write-Host $TrainerInfo.Target -ForegroundColor DarkGray
}

function Show-TrainerMenu {
    param([array]$Trainers)

    Write-Host ""
    Write-Host "Game Trainers" -ForegroundColor Cyan
    Write-Host "=============" -ForegroundColor DarkCyan
    Write-Host "Enter a number, name, folder, or CMake target. Press Ctrl+C to cancel." -ForegroundColor DarkGray
    Write-Host ""

    foreach ($arch in @("x64", "x86")) {
        $group = @($Trainers | Where-Object { $_.Architecture -eq $arch })
        if ($group.Count -eq 0) {
            continue
        }

        $archColor = if ($arch -eq "x64") { "Cyan" } else { "Yellow" }
        Write-Host "$arch trainers" -ForegroundColor $archColor

        foreach ($trainerInfo in $group) {
            $index = [array]::IndexOf($Trainers, $trainerInfo) + 1
            Write-MenuLine $index $trainerInfo
        }

        Write-Host ""
    }
}

function Find-Trainer {
    param(
        [array]$Trainers,
        [string]$Query
    )

    $trimmed = $Query.Trim()
    $index = 0
    if ([int]::TryParse($trimmed, [ref]$index) -and $index -ge 1 -and $index -le $Trainers.Count) {
        return @($Trainers[$index - 1])
    }

    $exact = @($Trainers | Where-Object {
        $_.Name -ieq $trimmed -or $_.Folder -ieq $trimmed -or $_.Target -ieq $trimmed
    })
    if ($exact.Count -gt 0) {
        return $exact
    }

    return @($Trainers | Where-Object {
        $_.Name -like "*$trimmed*" -or $_.Folder -like "*$trimmed*" -or $_.Target -like "*$trimmed*"
    })
}

function Resolve-Trainer {
    param(
        [array]$Trainers,
        [string]$Query
    )

    if (-not [string]::IsNullOrWhiteSpace($Query)) {
        $matches = @(Find-Trainer $Trainers $Query)
        if ($matches.Count -eq 1) {
            return $matches[0]
        }

        if ($matches.Count -gt 1) {
            $names = ($matches | ForEach-Object { $_.Name }) -join "`n  - "
            throw "Trainer query '$Query' matched multiple trainers:`n  - $names"
        }

        throw "Trainer not found: $Query"
    }

    while ($true) {
        Show-TrainerMenu $Trainers
        $selection = Read-Host "Select trainer"
        if ([string]::IsNullOrWhiteSpace($selection)) {
            continue
        }

        $matches = @(Find-Trainer $Trainers $selection)
        if ($matches.Count -eq 1) {
            return $matches[0]
        }

        if ($matches.Count -eq 0) {
            Write-Host "No trainer matched '$selection'." -ForegroundColor Red
            continue
        }

        Write-Host "That matched multiple trainers:" -ForegroundColor Yellow
        $matches | ForEach-Object { Write-Host "  - $($_.Name)" -ForegroundColor DarkYellow }
        Write-Host "Try a number or a more specific name." -ForegroundColor Yellow
    }
}

function Reset-CMakeCacheIfNeeded {
    param([string]$Platform)

    if (-not (Test-Path -LiteralPath $buildDir)) {
        New-Item -ItemType Directory -Path $buildDir -Force | Out-Null
        return
    }

    $cachePath = Join-Path $buildDir "CMakeCache.txt"
    if (-not (Test-Path -LiteralPath $cachePath)) {
        return
    }

    $cache = Get-Content -LiteralPath $cachePath -Raw
    $match = [regex]::Match($cache, "(?m)^CMAKE_GENERATOR_PLATFORM:INTERNAL=(.+)$")
    $cachedPlatform = if ($match.Success) { $match.Groups[1].Value.Trim() } else { "" }

    if ($cachedPlatform -ieq $Platform) {
        return
    }

    Write-Host "Clearing CMake cache for platform switch ($cachedPlatform -> $Platform)..." -ForegroundColor Yellow
    Remove-Item -LiteralPath $cachePath -Force

    $cmakeFiles = Join-Path $buildDir "CMakeFiles"
    if (Test-Path -LiteralPath $cmakeFiles) {
        Remove-Item -LiteralPath $cmakeFiles -Recurse -Force
    }
}

function Configure-Build {
    param([string]$Platform)

    Reset-CMakeCacheIfNeeded $Platform

    cmake -G "Visual Studio 17 2022" -A $Platform -S "$scriptDir" -B "$buildDir"
    if ($LASTEXITCODE -ne 0) {
        throw "CMake configure failed for $Platform."
    }
}

function Build-Trainer {
    param([object]$TrainerInfo)

    $platform = Get-Platform $TrainerInfo.Architecture

    Write-Host "=== Build Trainer ===" -ForegroundColor Cyan
    Write-Host "Trainer:       $($TrainerInfo.Name)"
    Write-Host "Target:        $($TrainerInfo.Target)"
    Write-Host "Architecture:  $($TrainerInfo.Architecture)"
    Write-Host "Configuration: $Configuration"
    Write-Host ""

    Configure-Build $platform

    cmake --build "$buildDir" --target $TrainerInfo.Target --config $Configuration --parallel
    if ($LASTEXITCODE -ne 0) {
        throw "Build failed for $($TrainerInfo.Name)."
    }

    if ($Run) {
        $exePath = Join-Path $buildDir "bin\$($TrainerInfo.Name)\$($TrainerInfo.Name).exe"
        if (-not (Test-Path -LiteralPath $exePath)) {
            throw "Built executable not found: $exePath"
        }

        Write-Host "Running $exePath" -ForegroundColor Green
        & $exePath
    }
}

function Build-ForArchitecture {
    param(
        [string]$Architecture,
        [array]$TrainersForArch
    )

    if ($TrainersForArch.Count -eq 0) {
        return
    }

    $platform = Get-Platform $Architecture

    Write-Host "=== Building $Architecture trainers ===" -ForegroundColor Green
    $TrainersForArch | ForEach-Object { Write-Host "  - $($_.Name)" }
    Write-Host ""

    Configure-Build $platform

    cmake --build "$buildDir" --config $Configuration --parallel
    if ($LASTEXITCODE -ne 0) {
        throw "Build failed for $Architecture."
    }

    Write-Host "Build successful for $Architecture trainers." -ForegroundColor Green
    Write-Host ""
}

$trainers = @(Get-Trainers | Sort-Object @{ Expression = { if ($_.Architecture -eq "x64") { 0 } else { 1 } } }, Name)
if ($trainers.Count -eq 0) {
    throw "No trainers found under $scriptDir\trainers."
}

if ($List) {
    Show-TrainerMenu $trainers
    return
}

if ($All) {
    Write-Host "=== Game Trainers Build Script ===" -ForegroundColor Cyan
    Write-Host "Configuration: $Configuration"
    Write-Host "Discovered trainers: $($trainers.Count)"
    Write-Host ""

    Build-ForArchitecture "x86" @($trainers | Where-Object { $_.Architecture -eq "x86" })
    Build-ForArchitecture "x64" @($trainers | Where-Object { $_.Architecture -eq "x64" })

    Write-Host "=== Build Complete ===" -ForegroundColor Green
    Write-Host "All trainers built successfully."
    return
}

$selected = Resolve-Trainer $trainers $Trainer
Build-Trainer $selected
