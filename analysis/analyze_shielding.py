#!/usr/bin/env python3
"""
analyze_shielding.py
Análise do estudo paramétrico de blindagem do SAMA-Suite.

Compara o efeito de diferentes espessuras de blindagem de Al na:
  - Energia depositada nos detectores
  - Atenuação de fluxo de prótons e elétrons
  - Dose integrada por evento
  - Razão sinal/ruído

Entrada: múltiplos arquivos ROOT do run_shielding_study.mac
Uso: python analyze_shielding.py shield_1mm.root shield_2mm.root shield_4mm.root shield_8mm.root
     Ou: python analyze_shielding.py --dir <diretório com arquivos>

Requer: uproot + matplotlib + numpy
"""

import sys
import os
import glob
import numpy as np

try:
    import uproot
except ImportError:
    print("ERRO: uproot não instalado. pip install uproot awkward")
    sys.exit(1)

import matplotlib.pyplot as plt

plt.rcParams.update({
    'font.size': 12,
    'axes.labelsize': 14,
    'axes.titlesize': 14,
    'figure.dpi': 150,
    'savefig.dpi': 200,
    'savefig.bbox_inches': 'tight'
})


def load_multiple_runs(filenames):
    """Carrega dados de múltiplos arquivos ROOT."""
    datasets = []
    for fname in filenames:
        f = uproot.open(fname)
        event_data = f["EventData"].arrays(library="np")
        datasets.append({
            'filename': os.path.basename(fname),
            'events': event_data,
            'n_events': len(event_data["EventID"])
        })
    return datasets


def extract_shield_thickness(filename):
    """Extrai espessura da blindagem do nome do arquivo."""
    # Tenta pattern como 'shield_2mm.root' ou '2mm.root'
    import re
    match = re.search(r'(\d+)mm', filename)
    if match:
        return int(match.group(1))
    return 0


def plot_shielding_comparison(datasets, thicknesses, output_dir):
    """Comparação de blindagem: estatísticas de deposição."""
    fig = plt.figure(figsize=(16, 12))
    gs = GridSpec(2, 2, figure=fig, hspace=0.3, wspace=0.3)
    fig.suptitle("SAMA-Suite: Estudo de Blindagem de Al", fontsize=18, y=0.98)

    colors = plt.cm.plasma(np.linspace(0.2, 0.9, len(datasets)))

    # ---- Gráfico 1: Edep total médio vs espessura ----
    ax1 = fig.add_subplot(gs[0, 0])
    mean_edep = []
    std_edep = []
    for ds in datasets:
        edep = ds['events']['EdepTotal']
        mean_edep.append(np.mean(edep))
        std_edep.append(np.std(edep) / np.sqrt(ds['n_events']))

    ax1.errorbar(thicknesses, mean_edep, yerr=std_edep, fmt='s-',
                  color='darkblue', markersize=10, capsize=5, linewidth=2)
    ax1.set_xlabel("Espessura de Al [mm]")
    ax1.set_ylabel("⟨Edep total⟩ [MeV]")
    ax1.set_title("Energia Média Depositada vs Blindagem")
    ax1.grid(True, alpha=0.3)

    # ---- Gráfico 2: Fração de eventos com deposição vs espessura ----
    ax2 = fig.add_subplot(gs[0, 1])
    detection_frac = []
    for ds in datasets:
        edep = ds['events']['EdepTotal']
        frac = np.sum(edep > 0) / ds['n_events'] * 100
        detection_frac.append(frac)

    ax2.plot(thicknesses, detection_frac, 'o-', color='crimson',
              markersize=10, linewidth=2)
    ax2.set_xlabel("Espessura de Al [mm]")
    ax2.set_ylabel("Fração de Detecção [%]")
    ax2.set_title("Fração de Eventos com Deposição")
    ax2.grid(True, alpha=0.3)

    # ---- Gráfico 3: Espectros de Edep sobrepostos ----
    ax3 = fig.add_subplot(gs[1, 0])
    for i, (ds, t) in enumerate(zip(datasets, thicknesses)):
        edep = ds['events']['EdepTotal']
        edep = edep[edep > 0]
        if len(edep) > 0:
            ax3.hist(edep, bins=80, color=colors[i], alpha=0.5,
                      label=f"{t} mm Al", histtype='stepfilled')
    ax3.set_xlabel("Edep Total [MeV]")
    ax3.set_ylabel("Contagem")
    ax3.set_title("Espectros de Edep por Espessura")
    ax3.set_yscale('log')
    ax3.legend()
    ax3.grid(True, alpha=0.3)

    # ---- Gráfico 4: Perfil de deposição por camada ----
    ax4 = fig.add_subplot(gs[1, 1])
    x = np.arange(6)
    width = 0.8 / len(datasets)
    for i, (ds, t) in enumerate(zip(datasets, thicknesses)):
        means = []
        for layer in range(6):
            col = f"EdepLayer{layer}"
            if col in ds['events']:
                means.append(np.mean(ds['events'][col]))
            else:
                means.append(0)
        offset = (i - len(datasets) / 2 + 0.5) * width
        ax4.bar(x + offset, means, width, color=colors[i], alpha=0.8,
                label=f"{t} mm Al", edgecolor='black', linewidth=0.3)

    ax4.set_xlabel("Camada de Si")
    ax4.set_ylabel("⟨Edep⟩ [MeV]")
    ax4.set_title("Perfil de Deposição por Camada")
    ax4.set_xticks(x)
    ax4.legend(fontsize=9)
    ax4.grid(True, alpha=0.3, axis='y')

    plt.savefig(os.path.join(output_dir, "shielding_study.png"))
    plt.close()
    print("  → shielding_study.png salvo")


def print_summary_table(datasets, thicknesses):
    """Tabela resumo da análise de blindagem."""
    print("\n" + "=" * 70)
    print("SAMA-Suite — RESUMO DO ESTUDO DE BLINDAGEM")
    print("=" * 70)
    print(f"{'Al [mm]':>8} | {'N eventos':>10} | {'Detecção':>10} | "
          f"{'⟨Edep⟩':>10} | {'σ(Edep)':>10}")
    print("-" * 70)

    for ds, t in zip(datasets, thicknesses):
        edep = ds['events']['EdepTotal']
        n = ds['n_events']
        det_frac = np.sum(edep > 0) / n * 100
        print(f"{t:>8} | {n:>10} | {det_frac:>9.1f}% | "
              f"{np.mean(edep):>8.3f} MeV | {np.std(edep):>8.3f} MeV")

    print("=" * 70)

    # Fator de atenuação relativo ao caso sem blindagem (1mm)
    if len(datasets) > 1:
        ref_rate = np.sum(datasets[0]['events']['EdepTotal'] > 0) / datasets[0]['n_events']
        print("\nFator de atenuação relativo a 1 mm Al:")
        for ds, t in zip(datasets, thicknesses):
            rate = np.sum(ds['events']['EdepTotal'] > 0) / ds['n_events']
            if ref_rate > 0:
                atten = rate / ref_rate
                print(f"  {t} mm Al: {atten:.3f} ({(1 - atten) * 100:.1f}% atenuação)")


def main():
    from matplotlib.gridspec import GridSpec

    if len(sys.argv) < 2:
        print("Uso: python analyze_shielding.py file1.root file2.root ...")
        print("  ou: python analyze_shielding.py --dir <diretório>")
        sys.exit(1)

    # Parse argumentos
    if sys.argv[1] == "--dir":
        directory = sys.argv[2] if len(sys.argv) > 2 else "."
        filenames = sorted(glob.glob(os.path.join(directory, "*.root")))
    else:
        filenames = sys.argv[1:]

    if not filenames:
        print("ERRO: nenhum arquivo ROOT encontrado")
        sys.exit(1)

    output_dir = "plots"
    os.makedirs(output_dir, exist_ok=True)

    print(f"Carregando {len(filenames)} arquivo(s)...")
    datasets = load_multiple_runs(filenames)

    # Extrair espessuras dos nomes
    thicknesses = [extract_shield_thickness(ds['filename']) for ds in datasets]

    # Se não conseguiu extrair, usar valores padrão
    if all(t == 0 for t in thicknesses):
        thicknesses = [1, 2, 4, 8][:len(datasets)]
        print(f"  Usando espessuras padrão: {thicknesses}")

    print_summary_table(datasets, thicknesses)
    plot_shielding_comparison(datasets, thicknesses, output_dir)

    print(f"\nGráficos salvos em '{output_dir}/'")


if __name__ == "__main__":
    main()
