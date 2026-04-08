//====================================================================
// DetectorConstruction.hh
// Geometria do sistema SAMA-Suite: telescópio de partículas com
// múltiplas camadas de silício + blindagem de alumínio para satélite LEO
//====================================================================
#ifndef DETECTOR_CONSTRUCTION_HH
#define DETECTOR_CONSTRUCTION_HH

#include "G4VUserDetectorConstruction.hh"
#include "G4LogicalVolume.hh"
#include "G4VPhysicalVolume.hh"
#include "G4Material.hh"
#include "G4GenericMessenger.hh"
#include "globals.hh"

class DetectorConstruction : public G4VUserDetectorConstruction
{
public:
    DetectorConstruction();
    ~DetectorConstruction() override;

    G4VPhysicalVolume* Construct() override;
    void ConstructSDandField() override;

    // Getters para análise
    G4int    GetNumberOfLayers()    const { return fNLayers; }
    G4double GetSiliconThickness()  const { return fSiThickness; }
    G4double GetShieldThickness()   const { return fShieldThickness; }
    G4double GetDetectorRadius()    const { return fDetectorRadius; }

private:
    void DefineMaterials();
    void DefineCommands();

    // Materiais
    G4Material* fVacuum;
    G4Material* fSilicon;
    G4Material* fAluminium;
    G4Material* fKapton;
    G4Material* fCopper;

    // Volumes lógicos dos detectores sensíveis
    std::vector<G4LogicalVolume*> fSiLayerLogicals;
    G4LogicalVolume* fFaradayCupLogical;

    // Parâmetros configuráveis via macro
    G4int    fNLayers;          // Número de camadas Si no telescópio
    G4double fSiThickness;      // Espessura de cada wafer de Si
    G4double fShieldThickness;  // Espessura da blindagem de Al
    G4double fDetectorRadius;   // Raio do detector
    G4double fLayerSpacing;     // Espaçamento entre camadas
    G4double fApertureAngle;    // Ângulo de abertura do colimador

    G4GenericMessenger* fMessenger;
};

#endif
