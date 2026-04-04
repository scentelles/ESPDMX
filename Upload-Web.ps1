# Script pour uploader les fichiers web vers l'ESP32
# Utilise Python pour appeler PlatformIO

param(
    [switch]$Help
)

if ($Help) {
    Write-Host @"
Usage: Upload-Web.ps1

Description:
  Uploads the web interface files to ESP32 filesystem using PlatformIO

Requirements:
  - Python 3.x installed
  - platformio package installed (pip install platformio)
  - firmware/data/web directory populated with built files

Examples:
  .\Upload-Web.ps1
  .\Upload-Web.ps1 -Help

"@
    exit 0
}

$pythonPath = "C:\Users\Sylvain\AppData\Local\Programs\Python\Python314\python.exe"
$firmwarePath = Join-Path (Split-Path -Parent $MyInvocation.MyCommand.Path) "firmware"

if (!(Test-Path $pythonPath)) {
    Write-Host "[ERR] Python not found at: $pythonPath" -ForegroundColor Red
    exit 1
}

if (!(Test-Path $firmwarePath)) {
    Write-Host "[ERR] Firmware folder not found at: $firmwarePath" -ForegroundColor Red
    exit 1
}

Write-Host "[INFO] Uploading filesystem to ESP32..." -ForegroundColor Cyan
Write-Host "[INFO] Using Python: $pythonPath" -ForegroundColor Cyan
Write-Host "[INFO] Firmware path: $firmwarePath" -ForegroundColor Cyan

Push-Location $firmwarePath
try {
    & $pythonPath -m platformio run --target uploadfs
    if ($LASTEXITCODE -eq 0) {
        Write-Host "[OK] Filesystem uploaded successfully!" -ForegroundColor Green
        Write-Host "[INFO] Access your ESP32 at: http://192.168.4.1 (or configured IP)" -ForegroundColor Green
    } else {
        Write-Host "[ERR] Upload failed with exit code: $LASTEXITCODE" -ForegroundColor Red
        exit 1
    }
}
finally {
    Pop-Location
}
