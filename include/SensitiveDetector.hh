//====================================================================
// SensitiveDetector.hh
// Detector sensível para wafers de silício e Faraday Cup
//====================================================================
#ifndef SENSITIVE_DETECTOR_HH
#define SENSITIVE_DETECTOR_HH

#include "G4VSensitiveDetector.hh"
#include "DetectorHit.hh"

class SensitiveDetector : public G4VSensitiveDetector
{
public:
    SensitiveDetector(const G4String& name, const G4String& hitsCollName);
    ~SensitiveDetector() override;

    void   Initialize(G4HCofThisEvent* hitsCE) override;
    G4bool ProcessHits(G4Step* step, G4TouchableHistory* history) override;
    void   EndOfEvent(G4HCofThisEvent* hitsCE) override;

private:
    DetectorHitsCollection* fHitsCollection;
    G4int fHCID;
};

#endif
