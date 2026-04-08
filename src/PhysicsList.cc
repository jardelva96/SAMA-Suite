//====================================================================
// PhysicsList.cc
// Lista de física modular para simulações de ambiente espacial:
//   - EM Standard Option 4 (alta precisão para baixa energia)
//   - Processos hadrônicos (QGSP_BIC_HP)
//   - Decaimento radioativo
//   - Ionização de íons
//====================================================================
#include "PhysicsList.hh"

#include "G4EmStandardPhysics_option4.hh"
#include "G4EmExtraPhysics.hh"
#include "G4DecayPhysics.hh"
#include "G4RadioactiveDecayPhysics.hh"
#include "G4HadronElasticPhysicsHP.hh"
#include "G4HadronPhysicsQGSP_BIC_HP.hh"
#include "G4StoppingPhysics.hh"
#include "G4IonPhysics.hh"
#include "G4StepLimiterPhysics.hh"

#include "G4SystemOfUnits.hh"

PhysicsList::PhysicsList()
    : G4VModularPhysicsList()
{
    SetVerboseLevel(1);

    // EM physics — Option 4 é a mais precisa para energias de ~keV a ~GeV
    // Essencial para simular corretamente dE/dx em Si fino (300 µm)
    RegisterPhysics(new G4EmStandardPhysics_option4());

    // Processos EM extras (gamma-nuclear, muon-nuclear, etc.)
    RegisterPhysics(new G4EmExtraPhysics());

    // Decaimento de partículas instáveis
    RegisterPhysics(new G4DecayPhysics());
    RegisterPhysics(new G4RadioactiveDecayPhysics());

    // Física hadrônica com HP (High Precision) para nêutrons < 20 MeV
    RegisterPhysics(new G4HadronElasticPhysicsHP());
    RegisterPhysics(new G4HadronPhysicsQGSP_BIC_HP());

    // Captura de partículas em repouso
    RegisterPhysics(new G4StoppingPhysics());

    // Física de íons pesados (relevante para GCR)
    RegisterPhysics(new G4IonPhysics());

    // Step limiter para controle preciso em volumes finos
    RegisterPhysics(new G4StepLimiterPhysics());
}

PhysicsList::~PhysicsList()
{}

void PhysicsList::SetCuts()
{
    // Cuts de produção otimizados para detectores de Si finos
    // Range cut de 10 µm garante tracking preciso em Si de 300 µm
    SetCutValue(10.0 * um, "gamma");
    SetCutValue(10.0 * um, "e-");
    SetCutValue(10.0 * um, "e+");
    SetCutValue(10.0 * um, "proton");

    // Dump das tabelas de cuts
    DumpCutValuesTable();
}
