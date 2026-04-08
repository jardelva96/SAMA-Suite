# ============================================================================
# SAMA-Suite Detector Simulation — Dockerfile
# Imagem completa com GEANT4 11.2 + ROOT + Python para build e execução
# ============================================================================

# Stage 1: Build GEANT4 e o projeto
FROM ubuntu:22.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive
ENV G4VERSION=11.2.2

# Dependências de compilação
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    wget \
    ca-certificates \
    libexpat1-dev \
    libxerces-c-dev \
    zlib1g-dev \
    libgl1-mesa-dev \
    libglu1-mesa-dev \
    libx11-dev \
    libxmu-dev \
    libxi-dev \
    libxpm-dev \
    libxft-dev \
    && rm -rf /var/lib/apt/lists/*

# Baixar e compilar GEANT4 (modo batch, sem visualização — mais leve)
WORKDIR /opt
RUN wget -q https://gitlab.cern.ch/geant4/geant4/-/archive/v${G4VERSION}/geant4-v${G4VERSION}.tar.gz \
    && tar xzf geant4-v${G4VERSION}.tar.gz \
    && rm geant4-v${G4VERSION}.tar.gz

RUN mkdir /opt/geant4-build && cd /opt/geant4-build \
    && cmake /opt/geant4-v${G4VERSION} \
        -DCMAKE_INSTALL_PREFIX=/opt/geant4-install \
        -DGEANT4_INSTALL_DATA=ON \
        -DGEANT4_USE_GDML=OFF \
        -DGEANT4_USE_OPENGL_X11=OFF \
        -DGEANT4_USE_QT=OFF \
        -DGEANT4_USE_RAYTRACER_X11=OFF \
        -DGEANT4_BUILD_MULTITHREADED=ON \
        -DCMAKE_BUILD_TYPE=Release \
    && make -j$(nproc) \
    && make install \
    && rm -rf /opt/geant4-build /opt/geant4-v${G4VERSION}

# Compilar o projeto SAMA-Suite
COPY . /opt/sama-suite-src
RUN mkdir /opt/sama-suite-build && cd /opt/sama-suite-build \
    && . /opt/geant4-install/bin/geant4.sh \
    && cmake /opt/sama-suite-src \
        -DGeant4_DIR=/opt/geant4-install/lib/cmake/Geant4 \
        -DCMAKE_BUILD_TYPE=Release \
    && make -j$(nproc)

# ============================================================================
# Stage 2: Runtime (imagem final mais leve)
# ============================================================================
FROM ubuntu:22.04 AS runtime

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    libexpat1 \
    libxerces-c3.2 \
    zlib1g \
    python3 \
    python3-pip \
    && rm -rf /var/lib/apt/lists/*

# Copiar GEANT4 instalado
COPY --from=builder /opt/geant4-install /opt/geant4-install

# Copiar binário e macros
COPY --from=builder /opt/sama-suite-build/sama_suite /opt/sama-suite/sama_suite
COPY --from=builder /opt/sama-suite-build/macros /opt/sama-suite/macros

# Copiar scripts de análise
COPY analysis/ /opt/sama-suite/analysis/

# Dependências Python para análise
RUN pip3 install --no-cache-dir uproot awkward matplotlib numpy

# Script de entrada
COPY entrypoint.sh /opt/sama-suite/entrypoint.sh
RUN chmod +x /opt/sama-suite/entrypoint.sh

WORKDIR /opt/sama-suite

# Variáveis de ambiente do GEANT4
ENV PATH="/opt/geant4-install/bin:${PATH}"
ENV LD_LIBRARY_PATH="/opt/geant4-install/lib:${LD_LIBRARY_PATH}"

ENTRYPOINT ["/opt/sama-suite/entrypoint.sh"]
CMD ["all"]
