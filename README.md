# SAMA-Suite Detector Simulation

**Simulação GEANT4 do sistema de detectores para monitoramento de fluxo de partículas e densidade do plasma na Anomalia Magnética do Atlântico Sul (SAMA)**
---

## Quick Start — Rodar com 1 Comando

**Único pré-requisito:** [Docker Desktop](https://www.docker.com/products/docker-desktop/) instalado e rodando.

```bash
# Windows (PowerShell)
git clone https://github.com/jardelva96/SAMA-Suite.git
cd SAMA-Suite
.\run.ps1

# Linux / macOS
git clone https://github.com/jardelva96/SAMA-Suite.git
cd SAMA-Suite
chmod +x run.sh && ./run.sh
```

**O que acontece:** compila GEANT4 + projeto automaticamente, executa simulação com espectro misto da SAMA (50k eventos), gera gráficos de análise. Os resultados ficam na pasta `output/`.

> **Tempo de execução:**
> - **Primeira vez: ~40-60 minutos** — o Docker compila o GEANT4 inteiro (~1.5M linhas de C++). Isso é normal e acontece **apenas uma vez**. O Docker faz cache da compilação.
> - **Execuções seguintes: ~2-5 minutos** — usa o cache do Docker, só executa a simulação.

### Simulações específicas (mais rápidas)

```bash
.\run.ps1 protons      # Só prótons aprisionados (10k eventos, ~1 min)
.\run.ps1 electrons    # Só elétrons aprisionados (10k eventos, ~1 min)
.\run.ps1 sama         # Espectro misto SAMA (50k eventos, ~5 min)
.\run.ps1 shielding    # Estudo de blindagem 1/2/4/8 mm Al (~10 min)
.\run.ps1 bash         # Shell interativo no container
```

### Resultados gerados

```
output/
├── sama_suite_output.root    # Dados brutos (histogramas + ntuples)
└── plots/
    ├── energy_spectra.png    # Espectros de energia e perfil de Bragg
    ├── bragg_curves.png      # Deposição por camada (p vs e⁻)
    ├── dedx_vs_energy.png    # dE/dx vs E (identificação de partículas)
    ├── hit_positions.png     # Distribuição espacial XY
    ├── detection_efficiency.png  # Eficiência vs energia
    ├── delta_e_e.png         # Técnica ΔE-E (separação p/e⁻)
    └── count_rates.png       # Taxa de contagem por camada
```

---

## Sumário

- [Quick Start](#quick-start--rodar-com-1-comando)
- [Descrição do Projeto](#descrição-do-projeto)
- [Física da SAMA](#física-da-sama)
- [Arquitetura dos Detectores](#arquitetura-dos-detectores)
- [Estrutura do Código](#estrutura-do-código)
- [Modelos Físicos Implementados](#modelos-físicos-implementados)
- [Compilação Manual (sem Docker)](#compilação-manual-sem-docker)
- [Macros de Simulação](#macros-de-simulação)
- [Análise de Dados](#análise-de-dados)
- [Resultados Esperados](#resultados-esperados)
- [Referências](#referências)

---

## Descrição do Projeto

Esta simulação implementa em GEANT4 o ambiente de teste virtual para o **SAMA-Suite** — um conjunto de instrumentos projetados para medir o fluxo de partículas carregadas e a densidade do plasma na região da Anomalia Magnética do Atlântico Sul (South Atlantic Magnetic Anomaly).

O sistema simulado compreende:

1. **Telescópio de partículas**: Stack de 6 wafers de silício (300 µm) com geometria de colimador cônico, implementando a técnica **ΔE-E** para identificação de partículas e medida de espectro energético.

2. **Faraday Cup**: Sensor de corrente para medida de densidade de plasma ionosférico, modelado como cilindro de cobre com abertura controlada.

3. **Blindagem estrutural**: Casca de alumínio simulando a estrutura do satélite em órbita LEO (~500 km), com espessura parametrizável para estudo de otimização.

---

## Física da SAMA

A Anomalia Magnética do Atlântico Sul é uma região onde o campo geomagnético apresenta intensidade anormalmente baixa (~22.000 nT vs ~60.000 nT nos pólos). Esta depressão magnética permite que partículas dos cinturões de radiação de Van Allen penetrem até altitudes mais baixas, criando um ambiente de radiação significativamente mais intenso para satélites em órbita baixa.

### Populações de partículas simuladas

| População | Energia típica | Modelo de referência | Fração no fluxo SAMA |
|-----------|---------------|---------------------|---------------------|
| Prótons aprisionados | 10 – 400 MeV | AP-8/AP-9 | ~70% |
| Elétrons aprisionados | 0.1 – 7 MeV | AE-8/AE-9 | ~25% |
| Raios cósmicos galácticos | 100 MeV – 10 GeV | Power-law (γ=2.7) | ~5% |

### Espectros implementados

- **Prótons aprisionados**: `dJ/dE ∝ E^{-1.5} × exp(-E/200 MeV)` — parametrização do modelo AP-8 MIN para L ≈ 1.2, B/B₀ ≈ 3
- **Elétrons aprisionados**: `dJ/dE ∝ exp(-E/1 MeV)` — parametrização do modelo AE-8 MAX
- **GCR**: `dJ/dE ∝ E^{-2.7}` — espectro de raios cósmicos galácticos no cutoff geomagnético da SAMA

---

## Arquitetura dos Detectores

```
                    ╔══════════════╗
                    ║  Colimador   ║  ← Al cônico (FOV = 60°)
                    ║  (Al cone)   ║
                    ╠══════════════╣
                    ║  Kapton MLI  ║  ← Isolamento térmico (50 µm)
                    ╠══════════════╣
                    ║   Si Layer 0 ║  ← ΔE (300 µm Si)
    Blindagem       ╠══════════════╣
    Al (2 mm)  ═════║   Si Layer 1 ║
                    ╠══════════════╣
                    ║   Si Layer 2 ║
                    ╠══════════════╣
                    ║   Si Layer 3 ║
                    ╠══════════════╣
                    ║   Si Layer 4 ║
                    ╠══════════════╣
                    ║   Si Layer 5 ║  ← E residual
                    ╠══════════════╣
                    ║ Faraday Cup  ║  ← Medida de plasma (Cu)
                    ╚══════════════╝
```

### Parâmetros geométricos (configuráveis via macro)

| Parâmetro | Valor padrão | Comando macro |
|-----------|-------------|---------------|
| Nº de camadas Si | 6 | `/sama/detector/nLayers` |
| Espessura Si | 300 µm | `/sama/detector/siThickness` |
| Blindagem Al | 2 mm | `/sama/detector/shieldThickness` |
| Raio do detector | 15 mm | `/sama/detector/detectorRadius` |
| Espaçamento | 5 mm | `/sama/detector/layerSpacing` |

---

## Estrutura do Código

```
SAMA-Suite-Sim/
├── run.ps1                         # ← RODAR AQUI (Windows)
├── run.sh                          # ← RODAR AQUI (Linux/macOS)
├── Dockerfile                      # Build GEANT4 + projeto
├── docker-compose.yml              # Orquestração
├── .dockerignore
├── CMakeLists.txt                  # Build system (CMake)
├── sama_suite.cc                   # Programa principal
├── include/
│   ├── DetectorConstruction.hh     # Geometria do detector
│   ├── PhysicsList.hh              # Lista de física
│   ├── PrimaryGeneratorAction.hh   # Gerador de partículas
│   ├── ActionInitialization.hh     # Inicialização MT
│   ├── RunAction.hh                # Ações por run
│   ├── EventAction.hh             # Ações por evento
│   ├── SteppingAction.hh          # Ações por step
│   ├── DetectorHit.hh             # Classe de hit
│   └── SensitiveDetector.hh       # Detector sensível
├── src/
│   ├── DetectorConstruction.cc
│   ├── PhysicsList.cc
│   ├── PrimaryGeneratorAction.cc
│   ├── ActionInitialization.cc
│   ├── RunAction.cc
│   ├── EventAction.cc
│   ├── SteppingAction.cc
│   ├── DetectorHit.cc
│   └── SensitiveDetector.cc
├── macros/
│   ├── init_vis.mac                # Inicialização interativa
│   ├── vis.mac                     # Configuração de visualização
│   ├── run_protons.mac             # Prótons aprisionados
│   ├── run_electrons.mac           # Elétrons aprisionados
│   ├── run_trapped_spectrum.mac    # Espectro misto SAMA
│   └── run_shielding_study.mac     # Estudo paramétrico de blindagem
└── analysis/
    ├── requirements.txt            # Dependências Python
    ├── plot_energy_deposit.py      # Análise de deposição de energia
    ├── plot_particle_flux.py       # Análise de fluxo e eficiência
    └── analyze_shielding.py        # Estudo de blindagem
```

---

## Modelos Físicos Implementados

### Lista de Física (`PhysicsList.cc`)

| Módulo | Descrição | Relevância |
|--------|-----------|-----------|
| `G4EmStandardPhysics_option4` | EM de alta precisão (keV–GeV) | dE/dx em Si fino |
| `G4EmExtraPhysics` | Gamma-nuclear, muon-nuclear | Reações secundárias |
| `G4HadronElasticPhysicsHP` | Espalhamento elástico HP | Nêutrons < 20 MeV |
| `G4HadronPhysicsQGSP_BIC_HP` | Cascata intranuclear (BIC) | Reações nucleares |
| `G4IonPhysics` | Física de íons pesados | GCR (He, C, Fe) |
| `G4RadioactiveDecayPhysics` | Decaimento radioativo | Ativação de materiais |
| `G4StepLimiterPhysics` | Controle de step | Precisão em volumes finos |

**Production cuts**: 10 µm para γ, e⁻, e⁺, p — otimizado para Si de 300 µm.

### Detectores sensíveis

- Cada wafer de Si é um **detector sensível independente** que registra:
  - Energia depositada (dE)
  - Posição do hit (x, y, z)
  - Energia cinética da partícula incidente
  - Tipo de partícula
  - Tempo global
  - Direção do momento

- A **Faraday Cup** registra partículas que penetram no volume interno (medida de corrente/fluência).

---

## Compilação Manual (sem Docker)

### Pré-requisitos

- **GEANT4** ≥ 11.0 (com datasets de física instalados)
- **CMake** ≥ 3.16
- **Compilador C++17** (GCC ≥ 8, Clang ≥ 7, MSVC ≥ 19.14)
- **ROOT** (opcional, para output em formato ROOT)

### Compilação

```bash
# Criar diretório de build
mkdir build && cd build

# Configurar (ajustar caminho do GEANT4)
cmake -DGeant4_DIR=/path/to/geant4/lib/cmake/Geant4 ..

# Compilar
make -j$(nproc)
```

### Execução

```bash
# Modo interativo com visualização
./sama_suite

# Modo batch — prótons aprisionados (10k eventos)
./sama_suite macros/run_protons.mac

# Modo batch com multi-threading (4 threads)
./sama_suite -m macros/run_trapped_spectrum.mac -t 4

# Estudo de blindagem
./sama_suite macros/run_shielding_study.mac
```

---

## Macros de Simulação

### `run_protons.mac`
Simula 10.000 prótons com espectro AP-8 MIN (10–400 MeV) incidindo no telescópio.

### `run_electrons.mac`
Simula 10.000 elétrons com espectro AE-8 MAX (0.1–7 MeV).

### `run_trapped_spectrum.mac`
Espectro misto representativo da SAMA: 70% prótons + 25% elétrons + 5% GCR. 50.000 eventos para boa estatística.

### `run_shielding_study.mac`
Estudo paramétrico variando espessura de blindagem Al (1, 2, 4, 8 mm) com 20.000 eventos cada configuração.

---

## Análise de Dados

### Instalação das dependências Python

```bash
pip install uproot awkward matplotlib numpy
```

### Scripts de análise

```bash
# Espectros de energia e perfil de Bragg
python analysis/plot_energy_deposit.py build/sama_suite_output.root

# Eficiência de detecção e técnica ΔE-E
python analysis/plot_particle_flux.py build/sama_suite_output.root

# Estudo de blindagem (múltiplos arquivos)
python analysis/analyze_shielding.py shield_1mm.root shield_2mm.root shield_4mm.root shield_8mm.root
```

### Outputs gerados

| Gráfico | Descrição |
|---------|-----------|
| `energy_spectra.png` | Espectros de energia primária e depositada, perfil de Bragg |
| `bragg_curves.png` | Perfil de deposição por camada separado por tipo de partícula |
| `dedx_vs_energy.png` | Curva dE/dx vs E (Bethe-Bloch) para identificação de partículas |
| `hit_positions.png` | Distribuição espacial XY de hits na camada frontal |
| `detection_efficiency.png` | Eficiência de detecção vs energia para p e e⁻ |
| `delta_e_e.png` | Diagrama ΔE-E para separação p/e⁻ |
| `count_rates.png` | Taxa de contagem por camada |
| `shielding_study.png` | Comparação de blindagens: Edep, detecção, espectros |

---

## Resultados Esperados

### Identificação de partículas (ΔE-E)

O telescópio de 6 camadas permite a técnica ΔE-E: a deposição na primeira camada (fina, ΔE) correlacionada com a energia residual (última camada, E) produz bandas distintas para prótons e elétrons no plano (E_res, ΔE), possibilitando a identificação partícula-a-partícula.

### Curva de Bragg

O perfil de deposição ao longo das camadas reproduz a curva de Bragg característica para prótons (pico de Bragg nas últimas camadas para energias próximas ao cutoff) e a deposição relativamente uniforme para elétrons relativísticos.

### Fator geométrico

O fator geométrico efetivo do telescópio é calculado como:

$$G = A \times \Omega \times \varepsilon$$

onde $A$ é a área do detector, $\Omega$ é o ângulo sólido do colimador, e $\varepsilon$ é a eficiência de detecção.

Para a configuração padrão ($R = 15$ mm, $\theta_{max} = 30°$):
- $A \approx 7.07$ cm²
- $\Omega \approx 0.842$ sr
- $G \sim 5.95 \times \varepsilon$ cm² sr

### Estudo de blindagem

A variação da espessura de Al permite otimizar a razão entre proteção contra radiação ambiente e sensibilidade do detector. Espera-se:
- **1 mm Al**: transparente para p > 20 MeV, e⁻ > 1 MeV
- **2 mm Al**: blindagem padrão para LEO — cutoff ~30 MeV para p
- **4 mm Al**: atenuação significativa de prótons < 50 MeV
- **8 mm Al**: forte blindagem, reduz sensibilidade para e⁻ de baixa energia

---

## Referências

1. **AP-8/AP-9 Trapped Proton Models**: Sawyer & Vette, 1976; Ginet et al., 2013
2. **AE-8/AE-9 Trapped Electron Models**: Vette, 1991; O'Brien et al., 2013
3. **GEANT4**: Agostinelli et al., *NIM A* 506 (2003) 250; Allison et al., *NIM A* 835 (2016) 186
4. **SAMA**: Heirtzler, *J. Atmos. Sol.-Terr. Phys.* 64 (2002) 1701
5. **Técnica ΔE-E**: Beringer et al. (PDG), *Phys. Rev.* D86 (2012) 010001
6. **Radiação em LEO**: Barth et al., *IEEE Trans. Nucl. Sci.* 50 (2003) 466

---

## Autor

Desenvolvido como demonstração de competência em simulação GEANT4 aplicada ao estudo do ambiente de radiação da Anomalia Magnética do Atlântico Sul (SAMA).

---

## Licença

Este projeto é disponibilizado para fins acadêmicos e de avaliação técnica.
