//====================================================================
// SteppingAction.cc
// Coleta dE/dx por step para análise detalhada
//====================================================================
#include "SteppingAction.hh"
#include "EventAction.hh"

#include "G4Step.hh"
#include "G4LogicalVolume.hh"
#include "G4SystemOfUnits.hh"

SteppingAction::SteppingAction(EventAction* eventAction)
    : G4UserSteppingAction(),
      fEventAction(eventAction)
{}

SteppingAction::~SteppingAction()
{}

void SteppingAction::UserSteppingAction(const G4Step* step)
{
    // Verifica se o step está em um detector de Si
    auto volume = step->GetPreStepPoint()->GetTouchableHandle()
                       ->GetVolume()->GetLogicalVolume();

    G4String volName = volume->GetName();

    // Detectores de Si têm nome "SiLayer_N"
    if (volName.find("SiLayer_") != std::string::npos)
    {
        G4int layerID = step->GetPreStepPoint()->GetTouchableHandle()
                             ->GetCopyNumber();
        G4double edep = step->GetTotalEnergyDeposit();

        if (edep > 0.)
            fEventAction->AddEdep(layerID, edep);
    }
}
