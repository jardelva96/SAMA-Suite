//====================================================================
// DetectorConstruction.cc
// Construção da geometria do SAMA-Suite:
//   - Telescópio de partículas: stack de wafers de Si com colimador
//   - Faraday Cup para medida de corrente/densidade de plasma
//   - Blindagem de alumínio simulando estrutura do satélite
//====================================================================
#include "DetectorConstruction.hh"
#include "SensitiveDetector.hh"

#include "G4RunManager.hh"
#include "G4NistManager.hh"
#include "G4Box.hh"
#include "G4Tubs.hh"
#include "G4Cons.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4SystemOfUnits.hh"
#include "G4VisAttributes.hh"
#include "G4Colour.hh"
#include "G4SDManager.hh"
#include "G4MultiFunctionalDetector.hh"
#include "G4SubtractionSolid.hh"

DetectorConstruction::DetectorConstruction()
    : G4VUserDetectorConstruction(),
      fVacuum(nullptr), fSilicon(nullptr), fAluminium(nullptr),
      fKapton(nullptr), fCopper(nullptr),
      fFaradayCupLogical(nullptr),
      fNLayers(6),
      fSiThickness(300.0 * um),
      fShieldThickness(2.0 * mm),
      fDetectorRadius(15.0 * mm),
      fLayerSpacing(5.0 * mm),
      fApertureAngle(30.0 * deg),
      fMessenger(nullptr)
{
    DefineCommands();
}

DetectorConstruction::~DetectorConstruction()
{
    delete fMessenger;
}

void DetectorConstruction::DefineMaterials()
{
    auto nist = G4NistManager::Instance();

    // Vácuo espacial (pressão ~1e-10 Pa em LEO)
    fVacuum = nist->FindOrBuildMaterial("G4_Galactic");

    // Silício para detectores de estado sólido
    fSilicon = nist->FindOrBuildMaterial("G4_Si");

    // Alumínio para blindagem estrutural (liga 6061 ~ Al puro para simulação)
    fAluminium = nist->FindOrBuildMaterial("G4_Al");

    // Kapton para isolamento térmico (MLI)
    fKapton = nist->FindOrBuildMaterial("G4_KAPTON");

    // Cobre para Faraday Cup
    fCopper = nist->FindOrBuildMaterial("G4_Cu");
}

G4VPhysicalVolume* DetectorConstruction::Construct()
{
    DefineMaterials();

    // ================================================================
    // WORLD — volume de vácuo representando ambiente espacial
    // ================================================================
    G4double worldSize = 50.0 * cm;
    auto worldSolid = new G4Box("World", worldSize, worldSize, worldSize);
    auto worldLogical = new G4LogicalVolume(worldSolid, fVacuum, "World");
    worldLogical->SetVisAttributes(G4VisAttributes::GetInvisible());

    auto worldPhysical = new G4PVPlacement(
        nullptr, G4ThreeVector(), worldLogical, "World", nullptr, false, 0, true);

    // ================================================================
    // BLINDAGEM ESTRUTURAL — cilindro de alumínio (satélite simplificado)
    // ================================================================
    G4double shieldOuterR = fDetectorRadius + fShieldThickness + 10.0 * mm;
    G4double shieldLength = (fNLayers + 2) * fLayerSpacing + 20.0 * mm;

    auto shieldSolid = new G4Tubs("ShieldOuter", 0., shieldOuterR,
                                   shieldLength / 2.0, 0., 360. * deg);
    auto shieldInnerSolid = new G4Tubs("ShieldInner", 0.,
                                        shieldOuterR - fShieldThickness,
                                        shieldLength / 2.0 - fShieldThickness,
                                        0., 360. * deg);
    auto shieldShell = new G4SubtractionSolid("ShieldShell",
                                               shieldSolid, shieldInnerSolid);

    auto shieldLogical = new G4LogicalVolume(shieldShell, fAluminium, "Shield");
    auto shieldVis = new G4VisAttributes(G4Colour(0.7, 0.7, 0.8, 0.3));
    shieldVis->SetForceSolid(true);
    shieldLogical->SetVisAttributes(shieldVis);

    new G4PVPlacement(nullptr, G4ThreeVector(), shieldLogical,
                      "Shield", worldLogical, false, 0, true);

    // ================================================================
    // COLIMADOR CÔNICO — define o campo de visão (FOV) do telescópio
    // ================================================================
    G4double collLength = 10.0 * mm;
    G4double collInnerR1 = fDetectorRadius;
    G4double collInnerR2 = fDetectorRadius * 0.5;
    G4double collOuterR = shieldOuterR - fShieldThickness - 1.0 * mm;

    G4double collZPos = shieldLength / 2.0 - fShieldThickness - collLength / 2.0 - 2.0 * mm;

    auto collSolid = new G4Cons("Collimator",
                                 collInnerR2, collOuterR,  // face -z
                                 collInnerR1, collOuterR,  // face +z
                                 collLength / 2.0,
                                 0., 360. * deg);

    auto collLogical = new G4LogicalVolume(collSolid, fAluminium, "Collimator");
    auto collVis = new G4VisAttributes(G4Colour(0.5, 0.5, 0.6));
    collVis->SetForceSolid(true);
    collLogical->SetVisAttributes(collVis);

    new G4PVPlacement(nullptr, G4ThreeVector(0., 0., collZPos),
                      collLogical, "Collimator", worldLogical, false, 0, true);

    // ================================================================
    // TELESCÓPIO DE PARTÍCULAS — stack de N wafers de Si
    // Cada camada mede dE/dx → identifica partícula (técnica ΔE-E)
    // ================================================================
    fSiLayerLogicals.clear();

    auto siVis = new G4VisAttributes(G4Colour(0.2, 0.6, 1.0));
    siVis->SetForceSolid(true);

    G4double firstLayerZ = collZPos - collLength / 2.0 - fLayerSpacing;

    for (G4int i = 0; i < fNLayers; ++i)
    {
        G4String layerName = "SiLayer_" + std::to_string(i);

        auto siSolid = new G4Tubs(layerName, 0., fDetectorRadius,
                                   fSiThickness / 2.0, 0., 360. * deg);

        auto siLogical = new G4LogicalVolume(siSolid, fSilicon, layerName);
        siLogical->SetVisAttributes(siVis);
        fSiLayerLogicals.push_back(siLogical);

        G4double zPos = firstLayerZ - i * fLayerSpacing;

        new G4PVPlacement(nullptr, G4ThreeVector(0., 0., zPos),
                          siLogical, layerName, worldLogical, false, i, true);
    }

    // ================================================================
    // CAMADA DE KAPTON (MLI) — isolamento térmico entre detector e estrutura
    // ================================================================
    G4double kaptonThickness = 50.0 * um;
    G4double kaptonZ = firstLayerZ + fLayerSpacing / 2.0;

    auto kaptonSolid = new G4Tubs("KaptonMLI", 0., fDetectorRadius + 5.0 * mm,
                                   kaptonThickness / 2.0, 0., 360. * deg);
    auto kaptonLogical = new G4LogicalVolume(kaptonSolid, fKapton, "KaptonMLI");
    auto kaptonVis = new G4VisAttributes(G4Colour(0.9, 0.7, 0.1, 0.5));
    kaptonVis->SetForceSolid(true);
    kaptonLogical->SetVisAttributes(kaptonVis);

    new G4PVPlacement(nullptr, G4ThreeVector(0., 0., kaptonZ),
                      kaptonLogical, "KaptonMLI", worldLogical, false, 0, true);

    // ================================================================
    // FARADAY CUP — medida de corrente de íons / densidade de plasma
    // Cilindro de cobre com abertura no topo
    // ================================================================
    G4double fcRadius = 10.0 * mm;
    G4double fcDepth  = 20.0 * mm;
    G4double fcWall   = 1.0  * mm;

    G4double fcZPos = firstLayerZ - fNLayers * fLayerSpacing - 15.0 * mm;

    // Copo externo
    auto fcOuterSolid = new G4Tubs("FCOuter", 0., fcRadius + fcWall,
                                    fcDepth / 2.0, 0., 360. * deg);
    auto fcInnerSolid = new G4Tubs("FCInner", 0., fcRadius,
                                    fcDepth / 2.0 - fcWall, 0., 360. * deg);
    auto fcShellSolid = new G4SubtractionSolid("FCShell", fcOuterSolid, fcInnerSolid,
                                                nullptr, G4ThreeVector(0., 0., fcWall));

    auto fcShellLogical = new G4LogicalVolume(fcShellSolid, fCopper, "FaradayCupShell");
    auto fcVis = new G4VisAttributes(G4Colour(0.8, 0.5, 0.2));
    fcVis->SetForceSolid(true);
    fcShellLogical->SetVisAttributes(fcVis);

    new G4PVPlacement(nullptr, G4ThreeVector(0., 0., fcZPos),
                      fcShellLogical, "FaradayCupShell", worldLogical, false, 0, true);

    // Volume sensível interno da Faraday Cup (vácuo — mede partículas que entram)
    auto fcSensitiveSolid = new G4Tubs("FCSensitive", 0., fcRadius,
                                        (fcDepth / 2.0 - fcWall), 0., 360. * deg);
    fFaradayCupLogical = new G4LogicalVolume(fcSensitiveSolid, fVacuum, "FaradayCup");
    auto fcSensVis = new G4VisAttributes(G4Colour(1.0, 0.8, 0.3, 0.3));
    fcSensVis->SetForceSolid(true);
    fFaradayCupLogical->SetVisAttributes(fcSensVis);

    new G4PVPlacement(nullptr, G4ThreeVector(0., 0., fcZPos + fcWall),
                      fFaradayCupLogical, "FaradayCup", worldLogical, false, 0, true);

    return worldPhysical;
}

void DetectorConstruction::ConstructSDandField()
{
    auto sdManager = G4SDManager::GetSDMpointer();

    // Detector sensível para cada camada de silício
    for (G4int i = 0; i < static_cast<G4int>(fSiLayerLogicals.size()); ++i)
    {
        G4String sdName = "SiLayer_SD_" + std::to_string(i);
        G4String hcName = "SiLayer_HC_" + std::to_string(i);

        auto sd = new SensitiveDetector(sdName, hcName);
        sdManager->AddNewDetector(sd);
        fSiLayerLogicals[i]->SetSensitiveDetector(sd);
    }

    // Detector sensível para Faraday Cup
    if (fFaradayCupLogical)
    {
        auto fcSD = new SensitiveDetector("FaradayCup_SD", "FaradayCup_HC");
        sdManager->AddNewDetector(fcSD);
        fFaradayCupLogical->SetSensitiveDetector(fcSD);
    }
}

void DetectorConstruction::DefineCommands()
{
    fMessenger = new G4GenericMessenger(this, "/sama/detector/",
                                        "SAMA-Suite detector configuration");

    fMessenger->DeclarePropertyWithUnit("siThickness", "um", fSiThickness,
                                         "Espessura do wafer de Si");

    fMessenger->DeclarePropertyWithUnit("shieldThickness", "mm", fShieldThickness,
                                         "Espessura da blindagem de Al");

    fMessenger->DeclareProperty("nLayers", fNLayers,
                                 "Número de camadas de Si no telescópio");

    fMessenger->DeclarePropertyWithUnit("detectorRadius", "mm", fDetectorRadius,
                                         "Raio dos detectores");

    fMessenger->DeclarePropertyWithUnit("layerSpacing", "mm", fLayerSpacing,
                                         "Espaçamento entre camadas de Si");
}
