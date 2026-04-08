//====================================================================
// PhysicsList.hh
// Lista de física customizada para simulação de partículas aprisionadas
// nos cinturões de Van Allen — processos EM, hadrônicos e decaimento
//====================================================================
#ifndef PHYSICS_LIST_HH
#define PHYSICS_LIST_HH

#include "G4VModularPhysicsList.hh"
#include "globals.hh"

class PhysicsList : public G4VModularPhysicsList
{
public:
    PhysicsList();
    ~PhysicsList() override;

    void SetCuts() override;
};

#endif
