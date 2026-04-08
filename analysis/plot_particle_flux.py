#!/usr/bin/env python3
"""
plot_particle_flux.py
Análise de fluxo de partículas e eficiência de detecção do SAMA-Suite.

Gera:
  - Eficiência de detecção vs energia
  - Fator geométrico efetivo
  - Taxa de contagem por camada
  - Separação de partículas via técnica ΔE-E

Requer: uproot + matplotlib + numpy
"""

import sys
import os
import numpy as np

try:
    import uproot
except ImportError:
    print("ERRO: uproot não instalado. pip install uproot awkward")
    sys.exit(1)

import matplotlib.pyplot as plt
from matplotlib.gridspec import GridSpec

plt.rcParams.update({
    'font.size': 12,
    'axes.labelsize': 14,
    'axes.titlesize': 14,
    'figure.dpi': 150,
    'savefig.dpi': 200,
    'savefig.bbox_inches': 'tight'
})


def load_data(filename):
    """Carrega ntuples do arquivo ROOT."""
    f = uproot.open(filename)
    event_data = f["EventData"].arrays(library="np")
    hit_data = f["HitData"].arrays(library="np")
    return event_data, hit_data


def plot_detection_efficiency(event_data, output_dir):
    """Eficiência de detecção vs energia primária."""
    fig, axes = plt.subplots(1, 2, figsize=(14, 5))
    fig.suptitle("SAMA-Suite: Eficiência de Detecção", fontsize=16)

    for pidx, pname in enumerate(["proton", "e-"]):
        ax = axes[pidx]
        mask = event_data["ParticleName"] == pname
        if not np.any(mask):
            continue

        energies = event_data["Eprimary"][mask]
        detected = event_data["EdepTotal"][mask] > 0  # pelo menos alguma deposição

        # Bins de energia
        bins = np.logspace(np.log10(max(energies.min(), 0.1)),
                           np.log10(energies.max()), 30)
        bin_centers = (bins[:-1] + bins[1:]) / 2

        # Calcular eficiência por bin
        eff = []
        eff_err = []
        for i in range(len(bins) - 1):
            in_bin = (energies >= bins[i]) & (energies < bins[i + 1])
            n_total = np.sum(in_bin)
            if n_total > 0:
                n_det = np.sum(detected & in_bin)
                e = n_det / n_total
                eff.append(e)
                # Erro binomial
                eff_err.append(np.sqrt(e * (1 - e) / n_total))
            else:
                eff.append(0)
                eff_err.append(0)

        eff = np.array(eff)
        eff_err = np.array(eff_err)

        ax.errorbar(bin_centers, eff * 100, yerr=eff_err * 100,
                     fmt='o-', capsize=3, markersize=5,
                     color='red' if pname == 'proton' else 'blue',
                     linewidth=1.5)
        ax.set_xlabel("Energia [MeV]")
        ax.set_ylabel("Eficiência [%]")
        title = "Prótons" if pname == "proton" else "Elétrons"
        ax.set_title(f"Eficiência — {title}")
        ax.set_xscale('log')
        ax.set_ylim(-5, 105)
        ax.grid(True, alpha=0.3, which='both')
        ax.axhline(y=100, color='gray', linestyle='--', alpha=0.5)

    plt.tight_layout()
    plt.savefig(os.path.join(output_dir, "detection_efficiency.png"))
    plt.close()
    print("  → detection_efficiency.png salvo")


def plot_delta_e_e(event_data, output_dir):
    """
    Técnica ΔE-E para identificação de partículas:
    Deposição na primeira camada (fina) vs camada mais espessa (última).
    """
    fig, ax = plt.subplots(figsize=(10, 8))

    colors = {'proton': 'red', 'e-': 'blue'}
    labels = {'proton': 'Prótons', 'e-': 'Elétrons'}

    for pname in ['proton', 'e-']:
        mask = event_data["ParticleName"] == pname
        if not np.any(mask):
            continue

        de = event_data["EdepLayer0"][mask]  # ΔE (camada fina)
        e_res = event_data["EdepLayer5"][mask]   # E residual (última camada)

        # Filtrar eventos com deposição em ambas
        valid = (de > 0) & (e_res > 0)
        if np.any(valid):
            ax.scatter(e_res[valid], de[valid],
                       c=colors.get(pname, 'gray'), s=2, alpha=0.3,
                       label=labels.get(pname, pname))

    ax.set_xlabel("E residual (Camada 5) [MeV]")
    ax.set_ylabel("ΔE (Camada 0) [MeV]")
    ax.set_title("SAMA-Suite: Identificação de Partículas — Técnica ΔE-E")
    ax.set_xscale('log')
    ax.set_yscale('log')
    ax.legend(markerscale=10, fontsize=12)
    ax.grid(True, alpha=0.3, which='both')

    plt.tight_layout()
    plt.savefig(os.path.join(output_dir, "delta_e_e.png"))
    plt.close()
    print("  → delta_e_e.png salvo")


def plot_count_rate(event_data, output_dir):
    """Taxa de contagem por camada."""
    fig, ax = plt.subplots(figsize=(10, 6))

    n_events = len(event_data["EventID"])
    counts_per_layer = []

    for i in range(6):
        col = f"EdepLayer{i}"
        if col in event_data:
            n_detected = np.sum(event_data[col] > 0)
            counts_per_layer.append(n_detected)
        else:
            counts_per_layer.append(0)

    layers = np.arange(6)
    rates = np.array(counts_per_layer) / n_events * 100

    bars = ax.bar(layers, rates, color='steelblue', edgecolor='navy',
                  alpha=0.8, width=0.6)

    # Adicionar valores nas barras
    for bar, rate in zip(bars, rates):
        ax.text(bar.get_x() + bar.get_width() / 2., bar.get_height() + 0.5,
                f'{rate:.1f}%', ha='center', va='bottom', fontsize=11)

    ax.set_xlabel("Camada de Si")
    ax.set_ylabel("Taxa de Detecção [%]")
    ax.set_title(f"SAMA-Suite: Taxa de Detecção por Camada ({n_events} eventos)")
    ax.set_xticks(layers)
    ax.set_xticklabels([f"Si-{i}" for i in range(6)])
    ax.grid(True, alpha=0.3, axis='y')

    plt.tight_layout()
    plt.savefig(os.path.join(output_dir, "count_rates.png"))
    plt.close()
    print("  → count_rates.png salvo")


def compute_geometric_factor(event_data):
    """
    Calcula o fator geométrico efetivo do telescópio.
    G = A × Ω × ε
    onde A = área do detector, Ω = ângulo sólido, ε = eficiência
    """
    R = 15.0  # mm — raio do detector
    A = np.pi * R**2  # mm² → cm²
    A_cm2 = A / 100.0

    # Ângulo sólido do colimador (cone de 30°)
    theta_max = 30.0 * np.pi / 180.0
    omega = 2 * np.pi * (1 - np.cos(theta_max))  # sr

    # Eficiência (todas as partículas)
    n_total = len(event_data["EventID"])
    n_detected = np.sum(event_data["NHits"] > 0)
    efficiency = n_detected / max(n_total, 1)

    G = A_cm2 * omega * efficiency  # cm² sr

    return {
        'area_cm2': A_cm2,
        'omega_sr': omega,
        'efficiency': efficiency,
        'G_cm2sr': G
    }


def main():
    if len(sys.argv) < 2:
        print("Uso: python plot_particle_flux.py <arquivo.root>")
        sys.exit(1)

    filename = sys.argv[1]
    if not os.path.exists(filename):
        print(f"ERRO: '{filename}' não encontrado")
        sys.exit(1)

    output_dir = "plots"
    os.makedirs(output_dir, exist_ok=True)

    print(f"Carregando {filename}...")
    event_data, hit_data = load_data(filename)

    n_events = len(event_data["EventID"])
    print(f"  {n_events} eventos carregados")

    print("\nGerando gráficos de fluxo...")
    plot_detection_efficiency(event_data, output_dir)
    plot_delta_e_e(event_data, output_dir)
    plot_count_rate(event_data, output_dir)

    # Fator geométrico
    gf = compute_geometric_factor(event_data)
    print("\n" + "=" * 50)
    print("FATOR GEOMÉTRICO DO TELESCÓPIO")
    print("=" * 50)
    print(f"  Área do detector:    {gf['area_cm2']:.2f} cm²")
    print(f"  Ângulo sólido:       {gf['omega_sr']:.4f} sr")
    print(f"  Eficiência global:   {gf['efficiency'] * 100:.1f}%")
    print(f"  Fator geométrico G:  {gf['G_cm2sr']:.4f} cm² sr")
    print("=" * 50)

    # Salvar resultado em arquivo
    with open(os.path.join(output_dir, "geometric_factor.txt"), 'w') as f:
        f.write("SAMA-Suite — Fator Geométrico Efetivo\n")
        f.write("=" * 50 + "\n")
        f.write(f"Área do detector:    {gf['area_cm2']:.4f} cm²\n")
        f.write(f"Ângulo sólido:       {gf['omega_sr']:.6f} sr\n")
        f.write(f"Eficiência global:   {gf['efficiency']:.4f}\n")
        f.write(f"Fator geométrico G:  {gf['G_cm2sr']:.6f} cm² sr\n")

    print(f"\nResultados salvos em '{output_dir}/'")


if __name__ == "__main__":
    main()
