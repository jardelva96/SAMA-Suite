//====================================================================
// DetectorHit.cc
//====================================================================
#include "DetectorHit.hh"

G4ThreadLocal G4Allocator<DetectorHit>* DetectorHitAllocator = nullptr;

DetectorHit::DetectorHit()
    : G4VHit(),
      fLayerID(-1),
      fParticleDef(nullptr),
      fEdep(0.),
      fPosition(G4ThreeVector()),
      fTime(0.),
      fKineticEnergy(0.),
      fTrackID(-1),
      fMomDir(G4ThreeVector())
{}

DetectorHit::~DetectorHit()
{}
