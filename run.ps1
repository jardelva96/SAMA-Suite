<# 
.SYNOPSIS
    SAMA-Suite — Rodar simulação completa com 1 comando
.DESCRIPTION
    Compila GEANT4 + projeto dentro de Docker e executa simulação + análise.
    Resultados ficam na pasta output/ (ROOT files + gráficos PNG).
.EXAMPLE
    .\run.ps1              # Pipeline completo (espectro SAMA + análise)
    .\run.ps1 protons      # Apenas prótons aprisionados
    .\run.ps1 electrons    # Apenas elétrons
    .\run.ps1 shielding    # Estudo de blindagem
    .\run.ps1 bash         # Shell interativo dentro do container
#>

param(
    [Parameter(Position=0)]
    [ValidateSet("all", "protons", "electrons", "sama", "shielding", "bash", "clean")]
    [string]$Mode = "all"
)

$ErrorActionPreference = "Stop"
$ProjectDir = $PSScriptRoot

Write-Host ""
Write-Host "============================================================" -ForegroundColor Cyan
Write-Host "  SAMA-Suite Detector Simulation" -ForegroundColor Cyan
Write-Host "  Anomalia Magnetica do Atlantico Sul" -ForegroundColor Cyan
Write-Host "============================================================" -ForegroundColor Cyan
Write-Host ""

# Verificar Docker
if (-not (Get-Command docker -ErrorAction SilentlyContinue)) {
    Write-Host "ERRO: Docker nao encontrado. Instale Docker Desktop:" -ForegroundColor Red
    Write-Host "  https://www.docker.com/products/docker-desktop/" -ForegroundColor Yellow
    exit 1
}

# Verificar se Docker está rodando
docker info *>$null
if ($LASTEXITCODE -ne 0) {
    Write-Host "ERRO: Docker nao esta rodando. Inicie o Docker Desktop." -ForegroundColor Red
    exit 1
}

# Criar pasta de output
$OutputDir = Join-Path $ProjectDir "output"
if (-not (Test-Path $OutputDir)) {
    New-Item -ItemType Directory -Path $OutputDir | Out-Null
}

if ($Mode -eq "clean") {
    Write-Host "Limpando imagem Docker e outputs..." -ForegroundColor Yellow
    docker rmi sama-suite 2>$null
    if (Test-Path $OutputDir) { Remove-Item -Recurse -Force $OutputDir }
    Write-Host "Limpo." -ForegroundColor Green
    exit 0
}

# Build da imagem (só recompila se houve mudança)
Write-Host "[1/2] Compilando GEANT4 + SAMA-Suite (primeira vez demora ~30 min)..." -ForegroundColor Yellow
Write-Host ""

Set-Location $ProjectDir
docker compose build sim
if ($LASTEXITCODE -ne 0) {
    Write-Host "ERRO: Falha no build. Verifique o Dockerfile." -ForegroundColor Red
    exit 1
}

Write-Host ""
Write-Host "[2/2] Executando simulacao (modo: $Mode)..." -ForegroundColor Yellow
Write-Host ""

# Executar
if ($Mode -eq "bash") {
    docker compose run --rm -it sim bash
} else {
    docker compose run --rm sim $Mode
}

if ($LASTEXITCODE -eq 0) {
    Write-Host ""
    Write-Host "============================================================" -ForegroundColor Green
    Write-Host "  Simulacao concluida!" -ForegroundColor Green
    Write-Host "  Resultados em: $OutputDir" -ForegroundColor Green
    Write-Host "============================================================" -ForegroundColor Green
    
    # Listar outputs
    if (Test-Path $OutputDir) {
        Write-Host ""
        Write-Host "Arquivos gerados:" -ForegroundColor Cyan
        Get-ChildItem -Path $OutputDir -Recurse -File | ForEach-Object {
            $relativePath = $_.FullName.Substring($OutputDir.Length + 1)
            $size = if ($_.Length -gt 1MB) { "{0:N1} MB" -f ($_.Length / 1MB) }
                    elseif ($_.Length -gt 1KB) { "{0:N1} KB" -f ($_.Length / 1KB) }
                    else { "$($_.Length) B" }
            Write-Host "  $relativePath ($size)" -ForegroundColor White
        }
    }
} else {
    Write-Host "ERRO: Simulacao falhou." -ForegroundColor Red
    exit 1
}
