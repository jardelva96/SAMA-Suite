//====================================================================
// sama_suite.cc
// Programa principal — SAMA-Suite Detector Simulation
//
// Simulação GEANT4 de um sistema de detectores para monitoramento
// de fluxo de partículas e densidade do plasma na Anomalia Magnética
// do Atlântico Sul (SAMA).
//
// Projeto FAPESP 2025/09361-3
// Instituto Nacional de Pesquisas Espaciais (INPE)
//
// Uso:
//   ./sama_suite                      # modo interativo com visualização
//   ./sama_suite macros/run_protons.mac  # modo batch
//   ./sama_suite -m macros/run_trapped_spectrum.mac -t 4  # 4 threads
//====================================================================
#include "DetectorConstruction.hh"
#include "PhysicsList.hh"
#include "ActionInitialization.hh"

#include "G4RunManagerFactory.hh"
#include "G4UImanager.hh"
#include "G4UIExecutive.hh"
#include "G4VisExecutive.hh"
#include "G4String.hh"

#include <string>

int main(int argc, char** argv)
{
    // Parse argumentos de linha de comando
    G4String macroFile = "";
    G4int nThreads = 4;

    for (int i = 1; i < argc; ++i)
    {
        G4String arg = argv[i];
        if (arg == "-m" && i + 1 < argc) {
            macroFile = argv[++i];
        } else if (arg == "-t" && i + 1 < argc) {
            nThreads = std::stoi(argv[++i]);
        } else if (arg[0] != '-') {
            macroFile = arg;
        }
    }

    // Modo interativo se nenhum macro foi fornecido
    G4UIExecutive* ui = nullptr;
    if (macroFile.empty())
        ui = new G4UIExecutive(argc, argv);

    // ================================================================
    // Run Manager — usa MT automaticamente se disponível
    // ================================================================
    auto runManager = G4RunManagerFactory::CreateRunManager(
        G4RunManagerType::Default);
    runManager->SetNumberOfThreads(nThreads);

    // ================================================================
    // Inicialização do detector, física e ações
    // ================================================================
    runManager->SetUserInitialization(new DetectorConstruction());
    runManager->SetUserInitialization(new PhysicsList());
    runManager->SetUserInitialization(new ActionInitialization());

    // ================================================================
    // Visualização e UI
    // ================================================================
    auto visManager = new G4VisExecutive();
    visManager->Initialize();

    auto uiManager = G4UImanager::GetUIpointer();

    if (!ui)
    {
        // Modo batch
        G4String command = "/control/execute ";
        uiManager->ApplyCommand(command + macroFile);
    }
    else
    {
        // Modo interativo
        uiManager->ApplyCommand("/control/execute macros/init_vis.mac");
        ui->SessionStart();
        delete ui;
    }

    // Cleanup
    delete visManager;
    delete runManager;

    return 0;
}
