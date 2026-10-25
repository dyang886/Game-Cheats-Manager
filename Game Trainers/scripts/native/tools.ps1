# Toolchain for native (RUNTIME none) trainer discovery - see docs/native_trainer_discovery.md.
# Everything lives in tools/native/ (gitignored): Cheat Engine 7.7 set up to load the
# CheatEngine.Mcp plugin from its autorun, the .NET 10 runtimes that plugin needs, and the MCP
# gateway the AI client starts over stdio.
#
#   .\scripts\native\tools.ps1 -Setup [-CheatEngineSource <Cheat Engine 7.7 folder>]
#   .\scripts\native\tools.ps1 -Start

param(
    [switch]$Setup,
    [switch]$Start,
    [string]$McpVersion = "2.0.0-beta.4",
    [string]$CheatEngineSource
)

$ErrorActionPreference = "Stop"

$root = Split-Path -Parent (Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path))
$toolsDir = Join-Path $root "tools\native"
$ceDir = Join-Path $toolsDir "CheatEngine"
$ceExe = Join-Path $ceDir "cheatengine-x86_64.exe"
$dotnetDir = Join-Path $toolsDir "dotnet"
$mcpDir = Join-Path $toolsDir "CheatEngine.Mcp"
$mcpDataDir = Join-Path $mcpDir "data"

function Install-CheatEngine {
    if (-not (Test-Path -LiteralPath $ceExe)) {
        if (-not $CheatEngineSource -or -not (Test-Path -LiteralPath (Join-Path $CheatEngineSource "cheatengine-x86_64.exe"))) {
            throw "Cheat Engine 7.7 is not in $ceDir - copy a Cheat Engine 7.7 folder there, or pass -CheatEngineSource <folder>."
        }
        robocopy $CheatEngineSource $ceDir /E /NFL /NDL /NJH /NJS /NP | Out-Null
        if ($LASTEXITCODE -ge 8) { throw "robocopy failed copying Cheat Engine ($LASTEXITCODE)" }
    }

    $version = (Get-Item -LiteralPath $ceExe).VersionInfo.ProductVersion
    if ($version -notlike "7.7*") {
        throw "CheatEngine.Mcp $McpVersion targets Cheat Engine 7.7 x64; $ceDir is $version."
    }

    # The plugin's managed host needs .NET 10 with all three frameworks.
    $runtimeConfig = [ordered]@{
        runtimeOptions = [ordered]@{
            tfm        = "net10.0"
            frameworks = @(
                [ordered]@{ name = "Microsoft.NETCore.App"; version = "10.0.0"; rollForward = "latestMinor" },
                [ordered]@{ name = "Microsoft.WindowsDesktop.App"; version = "10.0.0"; rollForward = "latestMinor" },
                [ordered]@{ name = "Microsoft.AspNetCore.App"; version = "10.0.0"; rollForward = "latestMinor" }
            )
        }
    }
    $runtimeConfig | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $ceDir "ce.runtimeconfig.json") -Encoding utf8

    $autorun = @'
-- Written by scripts/native/tools.ps1: loads CheatEngine.Mcp from beside this Cheat Engine.
local dll = getCheatEngineDir():gsub('[\\/]$', '') .. [[\..\CheatEngine.Mcp\CheatEngine.Mcp.dll]]
if loadPlugin(dll) == nil then
  print('CheatEngine.Mcp: loadPlugin failed for ' .. dll)
end
'@
    Set-Content -LiteralPath (Join-Path $ceDir "autorun\gcm_mcp.lua") -Value $autorun -Encoding ascii
}

function Install-DotNet {
    $frameworks = "Microsoft.NETCore.App", "Microsoft.AspNetCore.App", "Microsoft.WindowsDesktop.App"
    $missing = $frameworks | Where-Object { -not (Test-Path (Join-Path $dotnetDir "shared\$_\10.*")) }
    if (-not $missing) { return }

    $installer = Join-Path $dotnetDir "_install\dotnet-install.ps1"
    New-Item -ItemType Directory -Force (Split-Path $installer) | Out-Null
    Invoke-WebRequest https://dot.net/v1/dotnet-install.ps1 -OutFile $installer
    if ((Get-AuthenticodeSignature $installer).Status -ne "Valid") { throw "dotnet-install.ps1 signature is not valid" }

    foreach ($runtime in "dotnet", "aspnetcore", "windowsdesktop") {
        & $installer -Channel 10.0 -Runtime $runtime -Architecture x64 -InstallDir $dotnetDir -NoPath | Out-Null
    }
}

function Install-CheatEngineMcp {
    $gateway = Join-Path $mcpDir "CheatEngine.Mcp.Gateway.exe"
    $stamp = Join-Path $mcpDir "VERSION"
    if ((Test-Path $gateway) -and (Test-Path $stamp) -and ((Get-Content $stamp -Raw).Trim() -eq $McpVersion)) { return }

    $zipName = "CheatEngine.Mcp-$McpVersion-win-x64.zip"
    $base = "https://github.com/CheatEngineNet/CheatEngine.Mcp/releases/download/v$McpVersion"
    $downloadDir = Join-Path $mcpDir "_download"
    New-Item -ItemType Directory -Force $downloadDir | Out-Null
    $zip = Join-Path $downloadDir $zipName
    Invoke-WebRequest "$base/$zipName" -OutFile $zip
    Invoke-WebRequest "$base/SHA256SUMS.txt" -OutFile (Join-Path $downloadDir "SHA256SUMS.txt")

    $expected = (Get-Content (Join-Path $downloadDir "SHA256SUMS.txt") | Where-Object { $_ -like "*$zipName" }) -split '\s+' | Select-Object -First 1
    $actual = (Get-FileHash $zip -Algorithm SHA256).Hash
    if (-not $expected -or $actual -ne $expected) { throw "SHA256 mismatch for $zipName" }

    Expand-Archive -LiteralPath $zip -DestinationPath $mcpDir -Force
    Set-Content -LiteralPath $stamp -Value $McpVersion
}

function Install-McpSettings {
    $filesDir = Join-Path $mcpDir "files"
    $tablesDir = Join-Path $mcpDir "tables"
    New-Item -ItemType Directory -Force $mcpDataDir, $filesDir, $tablesDir | Out-Null

    # Only the two roots are set, so other edits survive a re-run and a moved checkout is fixed up.
    $settingsFile = Join-Path $mcpDataDir "appsettings.json"
    $settings = if (Test-Path $settingsFile) { Get-Content $settingsFile -Raw | ConvertFrom-Json -AsHashtable } else { [ordered]@{} }
    if (-not $settings.Mcp) { $settings.Mcp = [ordered]@{} }
    if (-not $settings.Mcp.Files) { $settings.Mcp.Files = [ordered]@{} }
    if (-not $settings.CheatEngineClient) { $settings.CheatEngineClient = [ordered]@{} }
    $settings.Mcp.Files.AllowedRoots = @($filesDir)
    $settings.CheatEngineClient.AllowedTableRoots = @($tablesDir)
    $settings | ConvertTo-Json -Depth 10 | Set-Content -LiteralPath $settingsFile -Encoding utf8
}

function Start-CheatEngine {
    if (-not (Test-Path -LiteralPath $ceExe)) { throw "Run .\scripts\native\tools.ps1 -Setup first." }

    # Environment reaches Cheat Engine because it runs asInvoker; an elevated launch would drop it.
    $env:DOTNET_ROOT = $dotnetDir
    $env:MCP_DATA_DIRECTORY = $mcpDataDir
    Start-Process -FilePath $ceExe -WorkingDirectory $ceDir
}

if (-not ($Setup -or $Start)) {
    Write-Host "Usage: .\scripts\native\tools.ps1 -Setup [-CheatEngineSource <folder>]   set up tools/native"
    Write-Host "       .\scripts\native\tools.ps1 -Start                                  launch Cheat Engine with the MCP plugin"
    exit 0
}

if ($Setup) {
    Install-CheatEngine
    Install-DotNet
    Install-CheatEngineMcp
    Install-McpSettings
    Write-Host "Native tools ready in $toolsDir"
}

if ($Start) {
    Start-CheatEngine
}
