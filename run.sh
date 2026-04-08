#!/bin/bash
# ============================================================================
# SAMA-Suite — Rodar simulação completa com 1 comando
#
# Uso:
#   ./run.sh              # Pipeline completo
#   ./run.sh protons      # Apenas prótons
#   ./run.sh electrons    # Apenas elétrons
#   ./run.sh shielding    # Estudo de blindagem
#   ./run.sh bash         # Shell interativo
#   ./run.sh clean        # Limpar imagem e outputs
# ============================================================================
set -e

MODE="${1:-all}"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
OUTPUT_DIR="${SCRIPT_DIR}/output"

echo ""
echo "============================================================"
echo "  SAMA-Suite Detector Simulation"
echo "  Anomalia Magnética do Atlântico Sul"
echo "============================================================"
echo ""

# Verificar Docker
if ! command -v docker &> /dev/null; then
    echo "ERRO: Docker não encontrado. Instale: https://docs.docker.com/get-docker/"
    exit 1
fi

if ! docker info &> /dev/null; then
    echo "ERRO: Docker não está rodando. Inicie o serviço Docker."
    exit 1
fi

mkdir -p "${OUTPUT_DIR}"

if [ "${MODE}" = "clean" ]; then
    echo "Limpando..."
    docker rmi sama-suite 2>/dev/null || true
    rm -rf "${OUTPUT_DIR}"
    echo "Limpo."
    exit 0
fi

# Build
echo "[1/2] Compilando GEANT4 + SAMA-Suite (primeira vez ~30 min)..."
echo ""
cd "${SCRIPT_DIR}"
docker compose build sim

echo ""
echo "[2/2] Executando simulação (modo: ${MODE})..."
echo ""

# Run
if [ "${MODE}" = "bash" ]; then
    docker compose run --rm -it sim bash
else
    docker compose run --rm sim "${MODE}"
fi

echo ""
echo "============================================================"
echo "  ✓ Simulação concluída!"
echo "  Resultados em: ${OUTPUT_DIR}/"
echo "============================================================"

if [ -d "${OUTPUT_DIR}" ]; then
    echo ""
    echo "Arquivos gerados:"
    find "${OUTPUT_DIR}" -type f -exec ls -lh {} \; | awk '{print "  " $NF " (" $5 ")"}'
fi
