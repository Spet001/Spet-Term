# ==============================================================================
# Spet@Term - Automated Desktop Bridge (Full Trust) MSIX Packaging Pipeline
# ==============================================================================

[CmdletBinding()]
param(
    [string]$OutputDir = "",
    [switch]$Sign = $true,
    [switch]$InstallCert = $false,
    [switch]$Rebuild = $false,
    [switch]$KeepLayout = $false
)

$ErrorActionPreference = "Stop"

$ProjectRoot = (Resolve-Path "$PSScriptRoot\..\..").Path
$PackagingDir = "$ProjectRoot\packaging\msix"

if (-not $OutputDir) {
    $OutputDir = "$PackagingDir\dist"
} elseif (-not [System.IO.Path]::IsPathRooted($OutputDir)) {
    $OutputDir = [System.IO.Path]::GetFullPath((Join-Path $ProjectRoot $OutputDir))
}

# 1. Locate Windows SDK Tools (makeappx.exe & signtool.exe)
$SdkKitsRoot = "C:\Program Files (x86)\Windows Kits\10\bin"
if (-not (Test-Path $SdkKitsRoot)) {
    throw "Windows 10/11 SDK not found in '$SdkKitsRoot'."
}

$MakeAppxPath = Get-ChildItem -Path $SdkKitsRoot -Filter "makeappx.exe" -Recurse -ErrorAction SilentlyContinue |
    Where-Object { $_.FullName -match "x64" } |
    Sort-Object LastWriteTime -Descending |
    Select-Object -First 1 -ExpandProperty FullName

$SignToolPath = Get-ChildItem -Path $SdkKitsRoot -Filter "signtool.exe" -Recurse -ErrorAction SilentlyContinue |
    Where-Object { $_.FullName -match "x64" } |
    Sort-Object LastWriteTime -Descending |
    Select-Object -First 1 -ExpandProperty FullName

if (-not $MakeAppxPath -or -not (Test-Path $MakeAppxPath)) {
    throw "makeappx.exe could not be located in Windows SDK."
}

Write-Host "[OK] Using MakeAppx : $MakeAppxPath" -ForegroundColor Green
if ($SignToolPath) {
    Write-Host "[OK] Using SignTool : $SignToolPath" -ForegroundColor Green
}

# 2. Check or Rebuild spetaterm.exe
$BinaryPath = "$ProjectRoot\spetaterm.exe"
if ($Rebuild -or -not (Test-Path $BinaryPath)) {
    Write-Host "[BUILD] Rebuilding spetaterm.exe via build.ps1..." -ForegroundColor Yellow
    Push-Location $ProjectRoot
    try {
        & powershell -ExecutionPolicy Bypass -File ".\build.ps1"
        if ($LASTEXITCODE -ne 0 -or -not (Test-Path $BinaryPath)) {
            throw "build.ps1 failed with exit code $LASTEXITCODE"
        }
    } finally {
        Pop-Location
    }
}

# 3. Prepare Staging Layout Directory
$LayoutDir = "$PackagingDir\layout"
if (Test-Path $LayoutDir) {
    Remove-Item -Recurse -Force $LayoutDir -ErrorAction SilentlyContinue
}
New-Item -ItemType Directory -Path "$LayoutDir\Assets" -Force | Out-Null
New-Item -ItemType Directory -Path "$OutputDir" -Force | Out-Null

Write-Host "[STAGING] Copying binaries, manifest, and visual assets..." -ForegroundColor Yellow
Copy-Item "$PackagingDir\AppxManifest.xml" "$LayoutDir\AppxManifest.xml"
Copy-Item "$PackagingDir\Assets\*" "$LayoutDir\Assets\" -Recurse
Copy-Item "$BinaryPath" "$LayoutDir\spetaterm.exe"

# Copy runtime assets if available
if (Test-Path "$ProjectRoot\assets") {
    Copy-Item -Recurse "$ProjectRoot\assets" "$LayoutDir\assets"
}

# 4. Package with makeappx.exe
$PackageName = "SpetATerm_0.0.5.0_x64.msix"
$PackagePath = "$OutputDir\$PackageName"

Write-Host "[PACK] Generating MSIX package with Desktop Bridge Full Trust..." -ForegroundColor Yellow
& $MakeAppxPath pack /d "$LayoutDir" /p "$PackagePath" /o /nv
if ($LASTEXITCODE -ne 0) {
    throw "makeappx.exe failed with exit code $LASTEXITCODE"
}

$PackageInfo = Get-Item "$PackagePath"
$FormattedSize = [math]::Round($PackageInfo.Length / 1MB, 2)
Write-Host "[SUCESSO] Pacote MSIX gerado: $PackagePath ($FormattedSize MB)" -ForegroundColor Green

# 5. Sign Package for Local Testing
$CertPfx = "$PackagingDir\SpetTerm_Dev.pfx"
$CertCer = "$PackagingDir\SpetTerm_Dev.cer"
$CertPassword = "SpetTermDev123!"

if ($Sign -and $SignToolPath) {
    if (-not (Test-Path $CertPfx)) {
        Write-Host "[CERT] Creating local development test certificate (CN=Spet001)..." -ForegroundColor Yellow
        $cert = New-SelfSignedCertificate -Type Custom -Subject "CN=Spet001" `
            -KeyUsage DigitalSignature -FriendlyName "Spet@Term Dev Package Certificate" `
            -CertStoreLocation "Cert:\CurrentUser\My" `
            -TextExtension @("2.5.29.37={text}1.3.6.1.5.5.7.3.3")
        $pwd = ConvertTo-SecureString -String $CertPassword -Force -AsPlainText
        Export-PfxCertificate -Cert $cert -FilePath $CertPfx -Password $pwd | Out-Null
        Export-Certificate -Cert $cert -FilePath $CertCer | Out-Null
    }

    Write-Host "[SIGN] Signing MSIX package with developer certificate..." -ForegroundColor Yellow
    & $SignToolPath sign /fd SHA256 /a /f "$CertPfx" /p "$CertPassword" "$PackagePath"
    if ($LASTEXITCODE -eq 0) {
        Write-Host "[OK] Pacote MSIX assinado digitalmente com sucesso!" -ForegroundColor Green
    } else {
        Write-Warning "Assinatura do pacote falhou com codigo $LASTEXITCODE."
    }
}

# 6. Optional: Install Certificate to TrustedPeople for 1-Click Local Installation
if ($InstallCert -and (Test-Path $CertCer)) {
    Write-Host "[TRUST] Installing dev certificate into TrustedPeople store (requires Admin)..." -ForegroundColor Yellow
    try {
        Import-Certificate -FilePath $CertCer -CertStoreLocation "Cert:\LocalMachine\TrustedPeople" | Out-Null
        Write-Host "[OK] Certificado instalado em TrustedPeople com sucesso!" -ForegroundColor Green
    } catch {
        Write-Warning "Falha ao instalar certificado em TrustedPeople. Execute o PowerShell como Administrador."
    }
}

# 7. Cleanup Layout Directory
if (-not $KeepLayout) {
    Remove-Item -Recurse -Force $LayoutDir -ErrorAction SilentlyContinue
}

Write-Host "==========================================================" -ForegroundColor Cyan
Write-Host " INSTRUCOES DE DISTRIBUICAO & INSTALACAO:                 " -ForegroundColor Cyan
Write-Host "==========================================================" -ForegroundColor Cyan
Write-Host "1. Instalacao Local / Sideload:" -ForegroundColor White
Write-Host "   Para instalar na sua maquina diretamente com 2 cliques:" -ForegroundColor Gray
Write-Host "   powershell -ExecutionPolicy Bypass -File .\packaging\msix\build_msix.ps1 -InstallCert" -ForegroundColor Gray
Write-Host "   Ou via terminal: Add-AppxPackage -Path '$PackagePath'" -ForegroundColor Gray
Write-Host ""
Write-Host "2. Publicacao na Microsoft Store:" -ForegroundColor White
Write-Host "   - No Microsoft Partner Center (https://partner.microsoft.com):" -ForegroundColor Gray
Write-Host "   - Crie a submissao do Spet@Term." -ForegroundColor Gray
Write-Host "   - Copie o Identity Name e Publisher atribuidos pelo Partner Center" -ForegroundColor Gray
Write-Host "     para o 'packaging\msix\AppxManifest.xml'." -ForegroundColor Gray
Write-Host "   - Execute este script novamente e faca o upload de '$PackageName'." -ForegroundColor Gray
Write-Host "   - A Microsoft Store assina o pacote com o certificado oficial automaticamente." -ForegroundColor Gray
Write-Host "==========================================================" -ForegroundColor Cyan
