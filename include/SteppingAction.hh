//====================================================================
// SteppingAction.hh
// Coleta informações a cada step para análise detalhada
//====================================================================
#ifndef STEPPING_ACTION_HH
#define STEPPING_ACTION_HH

#include "G4UserSteppingAction.hh"
#include "globals.hh"

class EventAction;

class SteppingAction : public G4UserSteppingAction
{
public:
    SteppingAction(EventAction* eventAction);
    ~SteppingAction() override;

    void UserSteppingAction(const G4Step* step) override;

private:
    EventAction* fEventAction;
};

#endif
