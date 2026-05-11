param(
    [string]$p = "",
    [string]$t = "",
    [string]$d = "",
    [switch]$r,
    [switch]$h
)

$TopDir = $PSScriptRoot
Write-Host "TOP_DIR=$TopDir"

function Print-Usage {
    $scriptName = Split-Path -Leaf $PSCommandPath
    Write-Host "Usage: .\$scriptName [-p <platform>] [-t <target>] [-d <debug>] [-r]"
    Write-Host "Supported platform : win, linux"
    Write-Host "Supported target: winMedia_ut"
    Write-Host "eg: .\$scriptName -p win -t winMedia_ut"
}

if ($h) {
    Print-Usage
    exit 0
}

Set-Location $TopDir

$cmakeParams = @()
if ($d -eq "y") {
    $cmakeParams += "-DCMAKE_BUILD_TYPE=Debug"
} else {
    $cmakeParams += "-DCMAKE_BUILD_TYPE=Release"
}

if ($r) {
    if (Test-Path "build") {
        Remove-Item -LiteralPath "build" -Recurse -Force
    }

    cmake -B build --preset=default @cmakeParams
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }
}

Write-Host "compile target = $t, platform = $p"

$jobs = [Environment]::ProcessorCount
$buildParams = @("--build", "--preset=default")
if ($t -ne "") {
    $buildParams += @("--target", $t)
}
$buildParams += @("--", "-j", "$jobs")

cmake @buildParams
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}
