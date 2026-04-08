//====================================================================
// DetectorHit.hh
// Classe de hit para detectores de silício do SAMA-Suite
//====================================================================
#ifndef DETECTOR_HIT_HH
#define DETECTOR_HIT_HH

#include "G4VHit.hh"
#include "G4THitsCollection.hh"
#include "G4Allocator.hh"
#include "G4ThreeVector.hh"
#include "G4ParticleDefinition.hh"

class DetectorHit : public G4VHit
{
public:
    DetectorHit();
    DetectorHit(const DetectorHit&) = default;
    ~DetectorHit() override;

    // Operadores para alocação rápida
    inline void* operator new(size_t);
    inline void  operator delete(void*);

    // Setters
    void SetLayerID(G4int id)                    { fLayerID = id; }
    void SetParticleDef(G4ParticleDefinition* p) { fParticleDef = p; }
    void SetEdep(G4double de)                    { fEdep = de; }
    void SetPosition(G4ThreeVector pos)          { fPosition = pos; }
    void SetTime(G4double t)                     { fTime = t; }
    void SetKineticEnergy(G4double ke)           { fKineticEnergy = ke; }
    void SetTrackID(G4int id)                    { fTrackID = id; }
    void SetMomentumDirection(G4ThreeVector d)   { fMomDir = d; }

    // Getters
    G4int                 GetLayerID()       const { return fLayerID; }
    G4ParticleDefinition* GetParticleDef()   const { return fParticleDef; }
    G4double              GetEdep()          const { return fEdep; }
    G4ThreeVector         GetPosition()      const { return fPosition; }
    G4double              GetTime()          const { return fTime; }
    G4double              GetKineticEnergy() const { return fKineticEnergy; }
    G4int                 GetTrackID()       const { return fTrackID; }
    G4ThreeVector         GetMomentumDirection() const { return fMomDir; }

private:
    G4int                 fLayerID;
    G4ParticleDefinition* fParticleDef;
    G4double              fEdep;
    G4ThreeVector         fPosition;
    G4double              fTime;
    G4double              fKineticEnergy;
    G4int                 fTrackID;
    G4ThreeVector         fMomDir;
};

// Tipos
using DetectorHitsCollection = G4THitsCollection<DetectorHit>;

// Alocador global
extern G4ThreadLocal G4Allocator<DetectorHit>* DetectorHitAllocator;

inline void* DetectorHit::operator new(size_t)
{
    if (!DetectorHitAllocator)
        DetectorHitAllocator = new G4Allocator<DetectorHit>;
    return (void*) DetectorHitAllocator->MallocSingle();
}

inline void DetectorHit::operator delete(void* hit)
{
    DetectorHitAllocator->FreeSingle((DetectorHit*) hit);
}

#endif
