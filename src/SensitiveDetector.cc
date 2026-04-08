//====================================================================
// SensitiveDetector.cc
// Processa hits em wafers de Si e Faraday Cup
//====================================================================
#include "SensitiveDetector.hh"

#include "G4HCofThisEvent.hh"
#include "G4Step.hh"
#include "G4ThreeVector.hh"
#include "G4SDManager.hh"
#include "G4SystemOfUnits.hh"

SensitiveDetector::SensitiveDetector(const G4String& name,
                                     const G4String& hitsCollName)
    : G4VSensitiveDetector(name),
      fHitsCollection(nullptr),
      fHCID(-1)
{
    collectionName.insert(hitsCollName);
}

SensitiveDetector::~SensitiveDetector()
{}

void SensitiveDetector::Initialize(G4HCofThisEvent* hitsCE)
{
    fHitsCollection = new DetectorHitsCollection(
        SensitiveDetectorName, collectionName[0]);

    if (fHCID < 0)
        fHCID = G4SDManager::GetSDMpointer()->GetCollectionID(fHitsCollection);

    hitsCE->AddHitsCollection(fHCID, fHitsCollection);
}

G4bool SensitiveDetector::ProcessHits(G4Step* step, G4TouchableHistory*)
{
    G4double edep = step->GetTotalEnergyDeposit();

    // Registra hit mesmo com edep=0 (para contagem de fluência na Faraday Cup)
    auto hit = new DetectorHit();

    auto touchable = step->GetPreStepPoint()->GetTouchable();
    hit->SetLayerID(touchable->GetCopyNumber());

    hit->SetParticleDef(
        const_cast<G4ParticleDefinition*>(
            step->GetTrack()->GetParticleDefinition()));

    hit->SetEdep(edep);
    hit->SetPosition(step->GetPreStepPoint()->GetPosition());
    hit->SetTime(step->GetPreStepPoint()->GetGlobalTime());
    hit->SetKineticEnergy(step->GetPreStepPoint()->GetKineticEnergy());
    hit->SetTrackID(step->GetTrack()->GetTrackID());
    hit->SetMomentumDirection(
        step->GetPreStepPoint()->GetMomentumDirection());

    fHitsCollection->insert(hit);

    return true;
}

void SensitiveDetector::EndOfEvent(G4HCofThisEvent*)
{}
