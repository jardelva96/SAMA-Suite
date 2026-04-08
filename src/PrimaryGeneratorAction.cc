//====================================================================
// PrimaryGeneratorAction.cc
// Gerador de partículas primárias com espectros realistas:
//   - Prótons aprisionados (modelo tipo AP-8/AP-9 para SAMA)
//   - Elétrons aprisionados (modelo tipo AE-8/AE-9)
//   - Raios cósmicos galácticos (GCR)
//   - Fluxo misto representativo da região SAMA
//====================================================================
#include "PrimaryGeneratorAction.hh"

#include "G4ParticleTable.hh"
#include "G4ParticleDefinition.hh"
#include "G4SystemOfUnits.hh"
#include "G4RandomDirection.hh"
#include "Randomize.hh"

#include <cmath>

PrimaryGeneratorAction::PrimaryGeneratorAction()
    : G4VUserPrimaryGeneratorAction(),
      fParticleGun(nullptr),
      fMessenger(nullptr),
      fSpectrumMode(kTrappedProtons),
      fMaxTheta(30.0 * deg)
{
    fParticleGun = new G4ParticleGun(1);

    // Defaults: próton de 100 MeV incidindo em -z
    auto proton = G4ParticleTable::GetParticleTable()->FindParticle("proton");
    fParticleGun->SetParticleDefinition(proton);
    fParticleGun->SetParticleEnergy(100.0 * MeV);
    fParticleGun->SetParticlePosition(G4ThreeVector(0., 0., 40.0 * cm));
    fParticleGun->SetParticleMomentumDirection(G4ThreeVector(0., 0., -1.));

    DefineCommands();
}

PrimaryGeneratorAction::~PrimaryGeneratorAction()
{
    delete fParticleGun;
    delete fMessenger;
}

void PrimaryGeneratorAction::GeneratePrimaries(G4Event* event)
{
    auto particleTable = G4ParticleTable::GetParticleTable();

    switch (fSpectrumMode)
    {
        case kMono:
            // Usa valores configurados no macro — não muda nada
            break;

        case kTrappedProtons:
        {
            fParticleGun->SetParticleDefinition(
                particleTable->FindParticle("proton"));
            fParticleGun->SetParticleEnergy(SampleTrappedProtonEnergy());
            break;
        }

        case kTrappedElectrons:
        {
            fParticleGun->SetParticleDefinition(
                particleTable->FindParticle("e-"));
            fParticleGun->SetParticleEnergy(SampleTrappedElectronEnergy());
            break;
        }

        case kGCR:
        {
            fParticleGun->SetParticleDefinition(
                particleTable->FindParticle("proton"));
            fParticleGun->SetParticleEnergy(SampleGCREnergy());
            break;
        }

        case kSAMAFlux:
        {
            G4int particleChoice = 0;
            G4double energy = SampleSAMAFluxEnergy(particleChoice);

            if (particleChoice == 0)
                fParticleGun->SetParticleDefinition(
                    particleTable->FindParticle("proton"));
            else
                fParticleGun->SetParticleDefinition(
                    particleTable->FindParticle("e-"));

            fParticleGun->SetParticleEnergy(energy);
            break;
        }
    }

    // Posição de geração: acima do detector, distribuída no disco de abertura
    G4double srcZ = 40.0 * cm;
    G4double srcRadius = 20.0 * mm;
    G4double r = srcRadius * std::sqrt(G4UniformRand());
    G4double phi = 2.0 * CLHEP::pi * G4UniformRand();
    G4double srcX = r * std::cos(phi);
    G4double srcY = r * std::sin(phi);

    fParticleGun->SetParticlePosition(G4ThreeVector(srcX, srcY, srcZ));

    // Direção: isotrópica dentro do cone de aceitação
    G4ThreeVector dir = SampleIsotropicDirection(fMaxTheta);
    fParticleGun->SetParticleMomentumDirection(dir);

    fParticleGun->GeneratePrimaryVertex(event);
}

//--------------------------------------------------------------------
// Espectro de prótons aprisionados na SAMA
// Modelo simplificado baseado em AP-8 MIN (solar minimum)
// dJ/dE ∝ E^(-γ) * exp(-E/E0)
// γ ~ 1.5, E0 ~ 200 MeV, faixa: 10 MeV – 400 MeV
//--------------------------------------------------------------------
G4double PrimaryGeneratorAction::SampleTrappedProtonEnergy() const
{
    // Parâmetros do espectro (AP-8 MIN, L ~ 1.2, B/B0 ~ 3)
    const G4double Emin = 10.0 * MeV;
    const G4double Emax = 400.0 * MeV;
    const G4double gamma = 1.5;
    const G4double E0 = 200.0 * MeV;

    // Amostragem por rejeição
    G4double maxFlux = std::pow(Emin / MeV, -gamma) * std::exp(-Emin / E0);

    G4double energy, flux, trial;
    do {
        energy = Emin + (Emax - Emin) * G4UniformRand();
        flux = std::pow(energy / MeV, -gamma) * std::exp(-energy / E0);
        trial = maxFlux * G4UniformRand();
    } while (trial > flux);

    return energy;
}

//--------------------------------------------------------------------
// Espectro de elétrons aprisionados na SAMA
// Modelo simplificado baseado em AE-8 MAX
// dJ/dE ∝ exp(-E/E0), E0 ~ 1 MeV, faixa: 0.1 – 7 MeV
//--------------------------------------------------------------------
G4double PrimaryGeneratorAction::SampleTrappedElectronEnergy() const
{
    const G4double Emin = 0.1 * MeV;
    const G4double Emax = 7.0 * MeV;
    const G4double E0 = 1.0 * MeV;

    // Amostragem por inversão da CDF (exponencial)
    G4double u = G4UniformRand();
    G4double normFactor = std::exp(-Emin / E0) - std::exp(-Emax / E0);
    G4double energy = -E0 * std::log(std::exp(-Emin / E0) - u * normFactor);

    return energy;
}

//--------------------------------------------------------------------
// Espectro GCR simplificado (prótons)
// Modelo power-law com modulação solar: dJ/dE ∝ E^(-2.7)
// Faixa: 100 MeV – 10 GeV
//--------------------------------------------------------------------
G4double PrimaryGeneratorAction::SampleGCREnergy() const
{
    const G4double Emin = 100.0 * MeV;
    const G4double Emax = 10.0 * GeV;
    const G4double alpha = 2.7;

    // Amostragem por inversão de power-law
    G4double u = G4UniformRand();
    G4double exponent = 1.0 - alpha;
    G4double energy = std::pow(
        std::pow(Emin, exponent) + u *
        (std::pow(Emax, exponent) - std::pow(Emin, exponent)),
        1.0 / exponent);

    return energy;
}

//--------------------------------------------------------------------
// Fluxo misto SAMA: 70% prótons aprisionados, 25% elétrons, 5% GCR
// Razões representativas para altitude de ~500 km na SAMA
//--------------------------------------------------------------------
G4double PrimaryGeneratorAction::SampleSAMAFluxEnergy(G4int& particleChoice) const
{
    G4double r = G4UniformRand();

    if (r < 0.70) {
        particleChoice = 0;  // próton
        return SampleTrappedProtonEnergy();
    } else if (r < 0.95) {
        particleChoice = 1;  // elétron
        return SampleTrappedElectronEnergy();
    } else {
        particleChoice = 0;  // GCR (próton)
        return SampleGCREnergy();
    }
}

//--------------------------------------------------------------------
// Amostra direção isotrópica dentro de cone de semi-ângulo maxTheta
// centrado em -z (partículas vindas de cima)
//--------------------------------------------------------------------
G4ThreeVector PrimaryGeneratorAction::SampleIsotropicDirection(
    G4double maxTheta) const
{
    G4double cosTheta = 1.0 - G4UniformRand() * (1.0 - std::cos(maxTheta));
    G4double sinTheta = std::sqrt(1.0 - cosTheta * cosTheta);
    G4double phi = 2.0 * CLHEP::pi * G4UniformRand();

    // Cone centrado em -z
    return G4ThreeVector(sinTheta * std::cos(phi),
                          sinTheta * std::sin(phi),
                          -cosTheta);
}

void PrimaryGeneratorAction::SetSpectrumMode(const G4String& mode)
{
    if      (mode == "mono")     fSpectrumMode = kMono;
    else if (mode == "protons")  fSpectrumMode = kTrappedProtons;
    else if (mode == "electrons") fSpectrumMode = kTrappedElectrons;
    else if (mode == "gcr")      fSpectrumMode = kGCR;
    else if (mode == "sama")     fSpectrumMode = kSAMAFlux;
    else
        G4cerr << "PrimaryGeneratorAction: modo desconhecido: " << mode << G4endl;
}

void PrimaryGeneratorAction::DefineCommands()
{
    fMessenger = new G4GenericMessenger(this, "/sama/generator/",
                                        "Primary generator configuration");

    fMessenger->DeclareMethod("spectrum", &PrimaryGeneratorAction::SetSpectrumMode,
        "Modo do espectro: mono, protons, electrons, gcr, sama");

    fMessenger->DeclarePropertyWithUnit("maxTheta", "deg", fMaxTheta,
        "Semi-ângulo máximo do cone de incidência");
}
