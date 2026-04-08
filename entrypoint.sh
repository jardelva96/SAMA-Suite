#!/bin/bash
# ============================================================================
# entrypoint.sh — SAMA-Suite Simulation Runner
# Executa simulação e análise automaticamente
# ============================================================================
set -e

# Configurar ambiente GEANT4
source /opt/geant4-install/bin/geant4.sh 2>/dev/null || true

OUTPUT_DIR="/opt/sama-suite/output"
mkdir -p "${OUTPUT_DIR}" "${OUTPUT_DIR}/plots"

echo "============================================================"
echo "  SAMA-Suite Detector Simulation"
echo "  Sistema de detectores para monitoramento de fluxo de"
echo "  partículas na Anomalia Magnética do Atlântico Sul"
echo "============================================================"
echo ""

run_simulation() {
    local MACRO=$1
    local DESC=$2
    echo "──────────────────────────────────────────────────────────"
    echo "  Simulação: ${DESC}"
    echo "  Macro: ${MACRO}"
    echo "──────────────────────────────────────────────────────────"
    cd "${OUTPUT_DIR}"
    /opt/sama-suite/sama_suite /opt/sama-suite/macros/${MACRO}
    echo "  ✓ Concluída"
    echo ""
}

run_analysis() {
    echo "──────────────────────────────────────────────────────────"
    echo "  Executando análise de dados..."
    echo "──────────────────────────────────────────────────────────"
    cd "${OUTPUT_DIR}"

    if [ -f "sama_suite_output.root" ]; then
        python3 /opt/sama-suite/analysis/plot_energy_deposit.py sama_suite_output.root || true
        python3 /opt/sama-suite/analysis/plot_particle_flux.py sama_suite_output.root || true
        echo "  ✓ Análise concluída — gráficos em output/plots/"
    else
        echo "  ⚠ Arquivo ROOT não encontrado — pulando análise"
    fi
    echo ""
}

case "${1}" in
    protons)
        run_simulation "run_protons.mac" "Prótons aprisionados (AP-8)"
        run_analysis
        ;;
    electrons)
        run_simulation "run_electrons.mac" "Elétrons aprisionados (AE-8)"
        run_analysis
        ;;
    sama)
        run_simulation "run_trapped_spectrum.mac" "Espectro misto SAMA (50k eventos)"
        run_analysis
        ;;
    shielding)
        run_simulation "run_shielding_study.mac" "Estudo de blindagem (1/2/4/8 mm Al)"
        ;;
    all)
        echo ">>> Executando pipeline completo..."
        echo ""
        run_simulation "run_trapped_spectrum.mac" "Espectro misto SAMA (50k eventos)"
        run_analysis
        echo ""
        echo "============================================================"
        echo "  ✓ Pipeline completo finalizado!"
        echo "  Resultados em: output/"
        echo "    - sama_suite_output.root  (dados brutos)"
        echo "    - plots/                  (gráficos PNG)"
        echo "============================================================"
        ;;
    bash)
        exec /bin/bash
        ;;
    *)
        echo "Uso: docker run sama-suite [all|protons|electrons|sama|shielding|bash]"
        echo ""
        echo "  all        Pipeline completo: simulação SAMA + análise (padrão)"
        echo "  protons    Apenas prótons aprisionados (10k eventos)"
        echo "  electrons  Apenas elétrons aprisionados (10k eventos)"
        echo "  sama       Espectro misto da SAMA (50k eventos)"
        echo "  shielding  Estudo paramétrico de blindagem"
        echo "  bash       Shell interativo"
        exit 1
        ;;
esac
