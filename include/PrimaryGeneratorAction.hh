//====================================================================
// PrimaryGeneratorAction.hh
// Gerador de partículas primárias simulando o espectro de partículas
// aprisionadas na região da SAMA (prótons e elétrons dos cinturões
// de radiação de Van Allen)
//====================================================================
#ifndef PRIMARY_GENERATOR_ACTION_HH
#define PRIMARY_GENERATOR_ACTION_HH

#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4ParticleGun.hh"
#include "G4GenericMessenger.hh"
#include "G4Event.hh"
#include "globals.hh"

class PrimaryGeneratorAction : public G4VUserPrimaryGeneratorAction
{
public:
    PrimaryGeneratorAction();
    ~PrimaryGeneratorAction() override;

    void GeneratePrimaries(G4Event* event) override;

    // Modos de geração
    enum SpectrumMode {
        kMono,              // Energia monocromática
        kTrappedProtons,    // Espectro AP-8/AP-9 para prótons aprisionados
        kTrappedElectrons,  // Espectro AE-8/AE-9 para elétrons aprisionados
        kGCR,               // Raios cósmicos galácticos (background)
        kSAMAFlux           // Fluxo misto representativo da região SAMA
    };

    void SetSpectrumMode(const G4String& mode);

private:
    void DefineCommands();

    // Amostragem de energia segundo modelos empíricos
    G4double SampleTrappedProtonEnergy() const;
    G4double SampleTrappedElectronEnergy() const;
    G4double SampleGCREnergy() const;
    G4double SampleSAMAFluxEnergy(G4int& particleChoice) const;

    // Direção isotrópica dentro de cone de aceitação
    G4ThreeVector SampleIsotropicDirection(G4double maxTheta) const;

    G4ParticleGun*      fParticleGun;
    G4GenericMessenger*  fMessenger;
    SpectrumMode         fSpectrumMode;
    G4double             fMaxTheta;   // Semi-ângulo do cone de incidência
};

#endif
