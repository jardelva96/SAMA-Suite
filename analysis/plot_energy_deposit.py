#!/usr/bin/env python3
"""
plot_energy_deposit.py
Análise de energia depositada nos detectores de Si do SAMA-Suite.

Gera:
  - Espectro de energia depositada total e por camada
  - Perfil de Bragg (Edep vs camada)
  - Distribuição de multiplicidade de hits
  - Comparação prótons vs elétrons

Requer: ROOT (PyROOT) ou uproot + matplotlib
"""

import sys
import os
import numpy as np

try:
    import uproot
    USE_UPROOT = True
except ImportError:
    USE_UPROOT = False

import matplotlib.pyplot as plt
import matplotlib.colors as mcolors
from matplotlib.gridspec import GridSpec

plt.rcParams.update({
    'font.size': 12,
    'axes.labelsize': 14,
    'axes.titlesize': 14,
    'figure.figsize': (14, 10),
    'figure.dpi': 150,
    'savefig.dpi': 200,
    'savefig.bbox_inches': 'tight'
})


def load_root_file(filename):
    """Carrega dados do arquivo ROOT usando uproot."""
    if not USE_UPROOT:
        print("ERRO: uproot não instalado. Instale com: pip install uproot awkward")
        sys.exit(1)

    f = uproot.open(filename)

    # Ntuple de eventos
    event_tree = f["EventData"]
    event_data = event_tree.arrays(library="np")

    # Ntuple de hits
    hit_tree = f["HitData"]
    hit_data = hit_tree.arrays(library="np")

    return event_data, hit_data


def plot_energy_spectra(event_data, output_dir):
    """Espectros de energia depositada."""
    fig, axes = plt.subplots(2, 2, figsize=(14, 10))
    fig.suptitle("SAMA-Suite: Espectros de Energia Depositada", fontsize=16)

    # Espectro de energia primária
    ax = axes[0, 0]
    ax.hist(event_data["Eprimary"], bins=100, color='steelblue',
            alpha=0.8, edgecolor='navy', linewidth=0.5)
    ax.set_xlabel("Energia Primária [MeV]")
    ax.set_ylabel("Contagem")
    ax.set_title("Espectro de Partículas Primárias")
    ax.set_yscale('log')
    ax.grid(True, alpha=0.3)

    # Energia depositada total
    ax = axes[0, 1]
    edep_total = event_data["EdepTotal"]
    ax.hist(edep_total[edep_total > 0], bins=100, color='crimson',
            alpha=0.8, edgecolor='darkred', linewidth=0.5)
    ax.set_xlabel("Edep Total [MeV]")
    ax.set_ylabel("Contagem")
    ax.set_title("Energia Depositada Total (todas as camadas)")
    ax.set_yscale('log')
    ax.grid(True, alpha=0.3)

    # Energia por camada (overlay)
    ax = axes[1, 0]
    colors = plt.cm.viridis(np.linspace(0.2, 0.9, 6))
    for i in range(6):
        col_name = f"EdepLayer{i}"
        if col_name in event_data:
            data = event_data[col_name]
            data = data[data > 0]
            if len(data) > 0:
                ax.hist(data, bins=80, color=colors[i], alpha=0.6,
                        label=f"Camada {i}", histtype='stepfilled')
    ax.set_xlabel("Edep [MeV]")
    ax.set_ylabel("Contagem")
    ax.set_title("Edep por Camada de Si")
    ax.set_yscale('log')
    ax.legend(fontsize=9)
    ax.grid(True, alpha=0.3)

    # Perfil de Bragg: Edep médio vs camada
    ax = axes[1, 1]
    mean_edep = []
    std_edep = []
    for i in range(6):
        col_name = f"EdepLayer{i}"
        if col_name in event_data:
            data = event_data[col_name]
            mean_edep.append(np.mean(data))
            std_edep.append(np.std(data) / np.sqrt(max(len(data), 1)))
        else:
            mean_edep.append(0)
            std_edep.append(0)

    layers = np.arange(6)
    ax.errorbar(layers, mean_edep, yerr=std_edep, fmt='o-',
                color='darkblue', markersize=8, capsize=4, linewidth=2)
    ax.set_xlabel("Camada de Si")
    ax.set_ylabel("⟨Edep⟩ [MeV]")
    ax.set_title("Perfil de Deposição (tipo Bragg)")
    ax.set_xticks(layers)
    ax.grid(True, alpha=0.3)

    plt.tight_layout()
    plt.savefig(os.path.join(output_dir, "energy_spectra.png"))
    plt.close()
    print("  → energy_spectra.png salvo")


def plot_bragg_curve(hit_data, output_dir):
    """Curva de Bragg detalhada com separação por partícula."""
    fig, axes = plt.subplots(1, 2, figsize=(14, 5))
    fig.suptitle("SAMA-Suite: Perfil de Bragg por Tipo de Partícula", fontsize=16)

    particles = {
        'proton': {'color': 'red', 'label': 'Prótons'},
        'e-': {'color': 'blue', 'label': 'Elétrons'},
    }

    for pidx, (pname, pconfig) in enumerate(particles.items()):
        ax = axes[pidx]
        mask = hit_data["Particle"] == pname
        if np.any(mask):
            layers = hit_data["LayerID"][mask]
            edeps = hit_data["Edep"][mask]

            mean_by_layer = []
            for l in range(6):
                lmask = layers == l
                if np.any(lmask):
                    mean_by_layer.append(np.mean(edeps[lmask]))
                else:
                    mean_by_layer.append(0)

            ax.bar(range(6), mean_by_layer, color=pconfig['color'], alpha=0.7,
                   edgecolor='black', linewidth=0.5)
            ax.set_xlabel("Camada de Si")
            ax.set_ylabel("⟨Edep⟩ [MeV]")
            ax.set_title(f"{pconfig['label']}")
            ax.set_xticks(range(6))
            ax.grid(True, alpha=0.3, axis='y')

    plt.tight_layout()
    plt.savefig(os.path.join(output_dir, "bragg_curves.png"))
    plt.close()
    print("  → bragg_curves.png salvo")


def plot_dedx_vs_energy(hit_data, output_dir):
    """Plot dE/dx vs E para identificação de partículas (Bethe-Bloch)."""
    fig, ax = plt.subplots(figsize=(10, 8))

    si_thickness_mm = 0.3  # 300 µm

    particles = {
        'proton': {'color': 'red', 'marker': '.', 'label': 'Prótons', 'alpha': 0.3},
        'e-': {'color': 'blue', 'marker': '.', 'label': 'Elétrons', 'alpha': 0.3},
    }

    for pname, pconfig in particles.items():
        mask = (hit_data["Particle"] == pname) & (hit_data["Edep"] > 0)
        if np.any(mask):
            ke = hit_data["KinEnergy"][mask]
            edep = hit_data["Edep"][mask]
            dedx = edep / si_thickness_mm

            ax.scatter(ke, dedx, c=pconfig['color'], s=1,
                       alpha=pconfig['alpha'], label=pconfig['label'])

    ax.set_xlabel("Energia Cinética [MeV]")
    ax.set_ylabel("dE/dx [MeV/mm]")
    ax.set_title("SAMA-Suite: dE/dx vs E — Identificação de Partículas")
    ax.set_xscale('log')
    ax.set_yscale('log')
    ax.legend(markerscale=10, fontsize=12)
    ax.grid(True, alpha=0.3, which='both')

    plt.tight_layout()
    plt.savefig(os.path.join(output_dir, "dedx_vs_energy.png"))
    plt.close()
    print("  → dedx_vs_energy.png salvo")


def plot_hit_positions(hit_data, output_dir):
    """Distribuição espacial de hits na primeira camada."""
    fig, ax = plt.subplots(figsize=(8, 8))

    mask = hit_data["LayerID"] == 0
    if np.any(mask):
        x = hit_data["PosX"][mask]
        y = hit_data["PosY"][mask]

        h = ax.hist2d(x, y, bins=80, cmap='inferno',
                      norm=mcolors.LogNorm())
        plt.colorbar(h[3], ax=ax, label="Contagem")

        # Desenhar contorno do detector (R = 15 mm)
        theta = np.linspace(0, 2 * np.pi, 100)
        ax.plot(15 * np.cos(theta), 15 * np.sin(theta),
                'w--', linewidth=2, label='Borda do detector')

    ax.set_xlabel("X [mm]")
    ax.set_ylabel("Y [mm]")
    ax.set_title("SAMA-Suite: Distribuição XY de Hits (Camada 0)")
    ax.set_aspect('equal')
    ax.legend()

    plt.tight_layout()
    plt.savefig(os.path.join(output_dir, "hit_positions.png"))
    plt.close()
    print("  → hit_positions.png salvo")


def main():
    if len(sys.argv) < 2:
        print("Uso: python plot_energy_deposit.py <arquivo.root>")
        print("     Os gráficos serão salvos no diretório 'plots/'")
        sys.exit(1)

    filename = sys.argv[1]

    if not os.path.exists(filename):
        print(f"ERRO: arquivo '{filename}' não encontrado")
        sys.exit(1)

    output_dir = "plots"
    os.makedirs(output_dir, exist_ok=True)

    print(f"Carregando {filename}...")
    event_data, hit_data = load_root_file(filename)

    n_events = len(event_data["EventID"])
    n_hits = len(hit_data["EventID"])
    print(f"  {n_events} eventos, {n_hits} hits")

    print("\nGerando gráficos...")
    plot_energy_spectra(event_data, output_dir)
    plot_bragg_curve(hit_data, output_dir)
    plot_dedx_vs_energy(hit_data, output_dir)
    plot_hit_positions(hit_data, output_dir)

    print(f"\nTodos os gráficos salvos em '{output_dir}/'")


if __name__ == "__main__":
    main()
