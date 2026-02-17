param(
    [string]$ForkRepoRoot = "D:\git_projects\unreal-mcp"
)

$ErrorActionPreference = "Stop"

$ProjectRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
$SourcePluginRoot = Join-Path $ForkRepoRoot "MCPGameProject\Plugins\UnrealMCP"
$DestinationPluginRoot = Join-Path $ProjectRoot "clients\ue\StupidChessUE\Plugins\UnrealMCP"

if (-not (Test-Path $SourcePluginRoot)) {
    throw "Source plugin path not found: $SourcePluginRoot"
}

if (-not (Test-Path (Split-Path $DestinationPluginRoot -Parent))) {
    New-Item -ItemType Directory -Path (Split-Path $DestinationPluginRoot -Parent) | Out-Null
}

Write-Host "Sync UnrealMCP plugin"
Write-Host "  Source: $SourcePluginRoot"
Write-Host "  Dest  : $DestinationPluginRoot"

robocopy `
    $SourcePluginRoot `
    $DestinationPluginRoot `
    /MIR `
    /R:2 `
    /W:1 `
    /NFL `
    /NDL `
    /NJH `
    /NJS `
    /NP `
    /XD Binaries Intermediate Saved .vs | Out-Null

$RobocopyExitCode = $LASTEXITCODE
if ($RobocopyExitCode -gt 7) {
    throw "robocopy failed with exit code $RobocopyExitCode"
}

Write-Host "UnrealMCP sync completed."
