[CmdletBinding()]
param(
    [ValidateSet('1804', '2004', '2204', '2404')]
    [string]$Target = '1804',
    [string]$Distribution,
    [ValidateRange(1, 128)]
    [int]$Jobs = 4,
    [switch]$CheckOnly
)

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
if (-not (Get-Command wsl.exe -ErrorAction SilentlyContinue)) {
    throw 'WSL is required. Install an Ubuntu WSL distribution and enable Docker access in it.'
}

$wslArgs = @()
if ($Distribution) { $wslArgs += @('--distribution', $Distribution) }
$wslArgs += @('--cd', $projectRoot, '--exec', 'env', "DISPCTRL_BUILD_JOBS=$Jobs",
              'bash', 'docker/docker_build.sh', $Target)
if ($CheckOnly) { $wslArgs += '--check' }
& wsl.exe @wslArgs
if ($LASTEXITCODE -ne 0) { throw "Ubuntu release workflow failed (exit $LASTEXITCODE)." }
if (-not $CheckOnly) {
    Write-Host "Release output: $projectRoot\deploy\ubuntu$Target\DispCtrl-linux-x64.tar.gz"
}
