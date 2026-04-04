# Script de déploiement du frontend React vers l'ESP32

param(
    [switch]$SkipBuild,
    [switch]$SkipUpload
)

$ErrorActionPreference = "Stop"

# Couleurs pour l'output
$colors = @{
    Success = 'Green'
    Error = 'Red'
    Info = 'Cyan'
    Warning = 'Yellow'
}

function Write-Info { Write-Host "[INFO] $args" -ForegroundColor $colors.Info }
function Write-Success { Write-Host "[OK] $args" -ForegroundColor $colors.Success }
function Write-ErrorMsg { Write-Host "[ERR] $args" -ForegroundColor $colors.Error }
function Write-WarningMsg { Write-Host "[!] $args" -ForegroundColor $colors.Warning }

Write-Info "DMXESP Web Deployment Script"
Write-Info "=============================="

$scriptPath = Split-Path -Parent $MyInvocation.MyCommand.Path
$frontendPath = Join-Path $scriptPath "frontend"
$firmwarePath = Join-Path $scriptPath "firmware"
$webDataPath = Join-Path $firmwarePath "data\web"

# Vérifier que les dossiers existent
if (!(Test-Path $frontendPath)) {
    Write-ErrorMsg "Frontend folder not found: $frontendPath"
    exit 1
}

if (!(Test-Path $firmwarePath)) {
    Write-ErrorMsg "Firmware folder not found: $firmwarePath"
    exit 1
}

# Créer le dossier de destination
if (!(Test-Path $webDataPath)) {
    Write-Info "Creating web data directory..."
    New-Item -ItemType Directory -Path $webDataPath -Force | Out-Null
}

# Étape 1: Construire le frontend
if (!$SkipBuild) {
    Write-Info ""
    Write-Info "Building React frontend..."
    
    Push-Location $frontendPath
    try {
        # Vérifier si node_modules existe
        if (!(Test-Path "node_modules")) {
            Write-Info "Installing dependencies..."
            npm install
            if ($LASTEXITCODE -ne 0) {
                Write-ErrorMsg "npm install failed"
                exit 1
            }
            Write-Success "Dependencies installed"
        }
        
        # Construire le projet
        npm run build
        if ($LASTEXITCODE -ne 0) {
            Write-ErrorMsg "Build failed"
            exit 1
        }
        Write-Success "Build completed"
    }
    finally {
        Pop-Location
    }
} else {
    Write-WarningMsg "Skipping build phase"
}

# Étape 2: Copier les fichiers
Write-Info ""
Write-Info "Copying files to firmware directory..."

$distPath = Join-Path $frontendPath "dist"
if (!(Test-Path $distPath)) {
    Write-ErrorMsg "Build artifacts not found. Run build first or remove -SkipBuild flag"
    exit 1
}

# Nettoyer le dossier de destination
if (Test-Path $webDataPath) {
    Remove-Item $webDataPath -Recurse -Force
}
New-Item -ItemType Directory -Path $webDataPath -Force | Out-Null

# Copier les fichiers
Copy-Item "$distPath\*" $webDataPath -Recurse -Force
Write-Success "Files copied: $webDataPath"

# Compter les fichiers
$fileCount = (Get-ChildItem $webDataPath -Recurse -File).Count
$totalSize = (Get-ChildItem $webDataPath -Recurse -File | Measure-Object -Property Length -Sum).Sum / 1MB
$sizeMB = [math]::Round($totalSize, 2)
Write-Info "Total files: $fileCount ($sizeMB MB)"

# Étape 3: Upload vers l'ESP32
if (!$SkipUpload) {
    Write-Info ""
    Write-Info "Uploading filesystem to ESP32..."
    
    Push-Location $firmwarePath
    try {
        python -m platformio run --target uploadfs
        if ($LASTEXITCODE -ne 0) {
            Write-ErrorMsg "Upload failed"
            exit 1
        }
        Write-Success "Filesystem uploaded successfully!"
    }
    finally {
        Pop-Location
    }
} else {
    Write-WarningMsg "Skipping upload phase"
}

Write-Info ""
Write-Success "Deployment completed!"
Write-Info "Access your ESP32 at: http://192.168.4.1 (or your IP address)"
