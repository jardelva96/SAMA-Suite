//====================================================================
// EventAction.cc
// Processa hits no fim de cada evento, preenche histogramas e ntuple
//====================================================================
#include "EventAction.hh"

#include "G4Event.hh"
#include "G4HCofThisEvent.hh"
#include "G4SDManager.hh"
#include "G4AnalysisManager.hh"
#include "G4SystemOfUnits.hh"

#include "DetectorHit.hh"

EventAction::EventAction()
    : G4UserEventAction(),
      fNLayers(6)
{
    fEdepPerLayer.resize(fNLayers, 0.);
    fCountPerLayer.resize(fNLayers, 0);
}

EventAction::~EventAction()
{}

void EventAction::SetNLayers(G4int n)
{
    fNLayers = n;
    fEdepPerLayer.resize(fNLayers, 0.);
    fCountPerLayer.resize(fNLayers, 0);
}

void EventAction::AddEdep(G4int layerID, G4double edep)
{
    if (layerID >= 0 && layerID < fNLayers)
        fEdepPerLayer[layerID] += edep;
}

G4double EventAction::GetEdep(G4int layerID) const
{
    if (layerID >= 0 && layerID < fNLayers)
        return fEdepPerLayer[layerID];
    return 0.;
}

void EventAction::IncrementParticleCount(G4int layerID)
{
    if (layerID >= 0 && layerID < fNLayers)
        fCountPerLayer[layerID]++;
}

G4int EventAction::GetParticleCount(G4int layerID) const
{
    if (layerID >= 0 && layerID < fNLayers)
        return fCountPerLayer[layerID];
    return 0;
}

void EventAction::BeginOfEventAction(const G4Event*)
{
    std::fill(fEdepPerLayer.begin(), fEdepPerLayer.end(), 0.);
    std::fill(fCountPerLayer.begin(), fCountPerLayer.end(), 0);
}

void EventAction::EndOfEventAction(const G4Event* event)
{
    auto analysisManager = G4AnalysisManager::Instance();
    auto sdManager = G4SDManager::GetSDMpointer();

    G4int totalHits = 0;
    G4double totalEdep = 0.;
    G4String primaryParticleName = "unknown";

    // Obter nome da partícula primária
    auto primaryVertex = event->GetPrimaryVertex();
    if (primaryVertex && primaryVertex->GetPrimary())
    {
        auto primaryDef = primaryVertex->GetPrimary()->GetParticleDefinition();
        if (primaryDef) primaryParticleName = primaryDef->GetParticleName();
    }

    // Energia primária
    G4double ePrimary = 0.;
    if (primaryVertex && primaryVertex->GetPrimary())
        ePrimary = primaryVertex->GetPrimary()->GetKineticEnergy();

    analysisManager->FillH1(0, ePrimary / MeV);

    // Processar hits de cada camada de Si
    for (G4int layer = 0; layer < fNLayers; ++layer)
    {
        G4String hcName = "SiLayer_HC_" + std::to_string(layer);
        G4int hcID = sdManager->GetCollectionID(hcName);

        if (hcID < 0) continue;

        auto hitsCollection = dynamic_cast<DetectorHitsCollection*>(
            event->GetHCofThisEvent()->GetHC(hcID));

        if (!hitsCollection) continue;

        G4double layerEdep = 0.;
        G4int nHits = hitsCollection->entries();
        totalHits += nHits;

        for (G4int i = 0; i < nHits; ++i)
        {
            auto hit = (*hitsCollection)[i];
            G4double edep = hit->GetEdep();
            layerEdep += edep;

            // Preencher ntuple de hits (ntuple ID = 1)
            analysisManager->FillNtupleIColumn(1, 0, event->GetEventID());
            analysisManager->FillNtupleIColumn(1, 1, layer);
            analysisManager->FillNtupleDColumn(1, 2, edep / MeV);
            analysisManager->FillNtupleDColumn(1, 3, hit->GetKineticEnergy() / MeV);
            analysisManager->FillNtupleDColumn(1, 4, hit->GetPosition().x() / mm);
            analysisManager->FillNtupleDColumn(1, 5, hit->GetPosition().y() / mm);
            analysisManager->FillNtupleDColumn(1, 6, hit->GetPosition().z() / mm);
            analysisManager->FillNtupleDColumn(1, 7, hit->GetTime() / ns);

            G4String partName = "unknown";
            if (hit->GetParticleDef())
                partName = hit->GetParticleDef()->GetParticleName();
            analysisManager->FillNtupleSColumn(1, 8, partName);
            analysisManager->AddNtupleRow(1);

            // H2: posição XY na camada 0
            if (layer == 0)
            {
                analysisManager->FillH2(1,
                    hit->GetPosition().x() / mm,
                    hit->GetPosition().y() / mm);
            }

            // H2: dE/dx vs E (usando espessura nominal de 300 µm)
            if (edep > 0.)
            {
                G4double dEdx = edep / (0.3 * mm);  // MeV/mm
                analysisManager->FillH2(0,
                    hit->GetKineticEnergy() / MeV,
                    dEdx / (MeV / mm));
            }
        }

        totalEdep += layerEdep;

        // Histograma por camada (H1 ids: 2 a 7)
        if (layer < 6)
            analysisManager->FillH1(2 + layer, layerEdep / MeV);
    }

    // Processar hits da Faraday Cup
    G4int fcHCID = sdManager->GetCollectionID("FaradayCup_HC");
    if (fcHCID >= 0)
    {
        auto fcHC = dynamic_cast<DetectorHitsCollection*>(
            event->GetHCofThisEvent()->GetHC(fcHCID));

        if (fcHC)
        {
            for (G4int i = 0; i < fcHC->entries(); ++i)
            {
                auto hit = (*fcHC)[i];
                analysisManager->FillH1(9, hit->GetKineticEnergy() / MeV);
            }
        }
    }

    // Histogramas globais
    analysisManager->FillH1(1, totalEdep / MeV);
    analysisManager->FillH1(8, totalHits);

    // Ntuple de evento (ntuple ID = 0)
    analysisManager->FillNtupleIColumn(0, 0, event->GetEventID());
    analysisManager->FillNtupleDColumn(0, 1, ePrimary / MeV);
    analysisManager->FillNtupleDColumn(0, 2, totalEdep / MeV);

    for (G4int layer = 0; layer < std::min(fNLayers, 6); ++layer)
    {
        // Recalcular do hitsCollection
        G4String hcName = "SiLayer_HC_" + std::to_string(layer);
        G4int hcID = sdManager->GetCollectionID(hcName);
        G4double lEdep = 0.;
        if (hcID >= 0) {
            auto hc = dynamic_cast<DetectorHitsCollection*>(
                event->GetHCofThisEvent()->GetHC(hcID));
            if (hc) {
                for (G4int i = 0; i < hc->entries(); ++i)
                    lEdep += (*hc)[i]->GetEdep();
            }
        }
        analysisManager->FillNtupleDColumn(0, 3 + layer, lEdep / MeV);
    }

    analysisManager->FillNtupleIColumn(0, 9, totalHits);
    analysisManager->FillNtupleSColumn(0, 10, primaryParticleName);
    analysisManager->AddNtupleRow(0);
}
