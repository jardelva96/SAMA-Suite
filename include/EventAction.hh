//====================================================================
// EventAction.hh
// Ações por evento: acumula energia depositada por camada
//====================================================================
#ifndef EVENT_ACTION_HH
#define EVENT_ACTION_HH

#include "G4UserEventAction.hh"
#include "globals.hh"
#include <vector>

class EventAction : public G4UserEventAction
{
public:
    EventAction();
    ~EventAction() override;

    void BeginOfEventAction(const G4Event* event) override;
    void EndOfEventAction(const G4Event* event) override;

    // Acumuladores de energia por camada
    void AddEdep(G4int layerID, G4double edep);
    G4double GetEdep(G4int layerID) const;

    // Contagem de partículas
    void IncrementParticleCount(G4int layerID);
    G4int GetParticleCount(G4int layerID) const;

    // Número de camadas
    void SetNLayers(G4int n);

private:
    std::vector<G4double> fEdepPerLayer;
    std::vector<G4int>    fCountPerLayer;
    G4int fNLayers;
};

#endif
