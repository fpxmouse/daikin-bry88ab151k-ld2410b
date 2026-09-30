param(
    [string]$Python = "python",
    [string]$EspHomeVersion = "2024.12.4"
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
$tools = Join-Path $root ".tools\esphome-$EspHomeVersion"
$dist = Join-Path $root "dist"
$firmwareBin = Join-Path $root "firmware\bin"
$stage = Join-Path ([System.IO.Path]::GetTempPath()) ("daikin-air-sensor-" + [guid]::NewGuid().ToString("N"))

New-Item -ItemType Directory -Force -Path $tools, $dist, $firmwareBin, $stage | Out-Null

if (-not (Test-Path (Join-Path $tools "esphome\__init__.py"))) {
    & $Python -m pip install --target $tools "esphome==$EspHomeVersion"
}

$env:PYTHONPATH = $tools
$stageFirmware = Join-Path $stage "firmware"
New-Item -ItemType Directory -Force -Path (Join-Path $stageFirmware "include") | Out-Null
Copy-Item -Path (Join-Path $root "firmware\*.yaml") -Destination $stageFirmware
Copy-Item -Path (Join-Path $root "firmware\include\*.h") -Destination (Join-Path $stageFirmware "include")
$configs = [ordered]@{
    "pm2005" = Join-Path $stageFirmware "pm2005.yaml"
    "pm2105" = Join-Path $stageFirmware "pm2105.yaml"
}

try {
    foreach ($model in $configs.Keys) {
        & $Python -m esphome config $configs[$model]
        & $Python -m esphome compile $configs[$model]
        $source = Join-Path $stageFirmware ".esphome\build\daikin-air-sensor\.pioenvs\daikin-air-sensor\firmware.bin"
        if (-not (Test-Path -LiteralPath $source)) {
            throw "OTA image not found after compiling $model`: $source"
        }
        $fileName = "bry88ab151k-$model-ld2410b-ota.bin"
        Copy-Item -LiteralPath $source -Destination (Join-Path $dist $fileName) -Force
        Copy-Item -LiteralPath $source -Destination (Join-Path $firmwareBin $fileName) -Force
    }
} finally {
    $tempPrefix = Join-Path ([System.IO.Path]::GetTempPath()) "daikin-air-sensor-"
    if ($stage.StartsWith($tempPrefix, [System.StringComparison]::OrdinalIgnoreCase)) {
        Remove-Item -LiteralPath $stage -Recurse -Force
    }
}

Get-FileHash (Join-Path $dist "*.bin") -Algorithm SHA256
