//====================================================================
// RunAction.cc
// Configura o sistema de análise (histogramas e ntuples) usando
// G4AnalysisManager — output em ROOT ou CSV
//====================================================================
#include "RunAction.hh"

#include "G4Run.hh"
#include "G4RunManager.hh"
#include "G4AnalysisManager.hh"
#include "G4SystemOfUnits.hh"

RunAction::RunAction()
    : G4UserRunAction()
{}

RunAction::~RunAction()
{}

void RunAction::BeginOfRunAction(const G4Run* run)
{
    auto analysisManager = G4AnalysisManager::Instance();
    analysisManager->SetDefaultFileType("root");
    analysisManager->SetVerboseLevel(1);

    G4String fileName = "sama_suite_output";
    analysisManager->SetFileName(fileName);

    // ================================================================
    // HISTOGRAMAS
    // ================================================================

    // H1-0: Espectro de energia das partículas primárias
    analysisManager->CreateH1("Eprimary",
        "Energia das particulas primarias;E [MeV];Contagem",
        200, 0., 500.);

    // H1-1: Energia depositada total (todas as camadas)
    analysisManager->CreateH1("EdepTotal",
        "Energia depositada total;Edep [MeV];Contagem",
        200, 0., 50.);

    // H1-2 a H1-7: Energia depositada por camada (6 camadas)
    for (G4int i = 0; i < 6; ++i)
    {
        G4String name = "EdepLayer" + std::to_string(i);
        G4String title = "Edep na camada " + std::to_string(i) +
                         ";Edep [MeV];Contagem";
        analysisManager->CreateH1(name, title, 200, 0., 10.);
    }

    // H1-8: Multiplicidade de hits por evento
    analysisManager->CreateH1("HitMult",
        "Multiplicidade de hits;N hits;Contagem",
        50, 0., 50.);

    // H1-9: Espectro de energia na Faraday Cup
    analysisManager->CreateH1("EfcUp",
        "Energia cinetica na Faraday Cup;E [MeV];Contagem",
        200, 0., 500.);

    // H2-0: dE/dx vs E (Bethe-Bloch plot para identificação de partículas)
    analysisManager->CreateH2("dEdxVsE",
        "dE/dx vs E;E [MeV];dE/dx [MeV/mm]",
        200, 0., 500., 200, 0., 50.);

    // H2-1: Posição XY dos hits na primeira camada
    analysisManager->CreateH2("HitXY",
        "Posicao XY dos hits (camada 0);X [mm];Y [mm]",
        100, -20., 20., 100, -20., 20.);

    // ================================================================
    // NTUPLE — dados evento-a-evento para análise offline
    // ================================================================
    analysisManager->CreateNtuple("EventData", "Dados por evento");
    analysisManager->CreateNtupleIColumn("EventID");        // 0
    analysisManager->CreateNtupleDColumn("Eprimary");       // 1
    analysisManager->CreateNtupleDColumn("EdepTotal");      // 2
    analysisManager->CreateNtupleDColumn("EdepLayer0");     // 3
    analysisManager->CreateNtupleDColumn("EdepLayer1");     // 4
    analysisManager->CreateNtupleDColumn("EdepLayer2");     // 5
    analysisManager->CreateNtupleDColumn("EdepLayer3");     // 6
    analysisManager->CreateNtupleDColumn("EdepLayer4");     // 7
    analysisManager->CreateNtupleDColumn("EdepLayer5");     // 8
    analysisManager->CreateNtupleIColumn("NHits");          // 9
    analysisManager->CreateNtupleSColumn("ParticleName");   // 10
    analysisManager->FinishNtuple();

    // Ntuple para hits individuais
    analysisManager->CreateNtuple("HitData", "Dados por hit");
    analysisManager->CreateNtupleIColumn("EventID");        // 0
    analysisManager->CreateNtupleIColumn("LayerID");        // 1
    analysisManager->CreateNtupleDColumn("Edep");           // 2
    analysisManager->CreateNtupleDColumn("KinEnergy");      // 3
    analysisManager->CreateNtupleDColumn("PosX");           // 4
    analysisManager->CreateNtupleDColumn("PosY");           // 5
    analysisManager->CreateNtupleDColumn("PosZ");           // 6
    analysisManager->CreateNtupleDColumn("Time");           // 7
    analysisManager->CreateNtupleSColumn("Particle");       // 8
    analysisManager->FinishNtuple();

    analysisManager->OpenFile();

    G4cout << "=== SAMA-Suite Run #" << run->GetRunID()
           << " started ===" << G4endl;
}

void RunAction::EndOfRunAction(const G4Run* run)
{
    auto analysisManager = G4AnalysisManager::Instance();

    G4int nEvents = run->GetNumberOfEvent();
    if (nEvents == 0) return;

    G4cout << "\n=== SAMA-Suite Run #" << run->GetRunID()
           << " completed: " << nEvents << " eventos ===" << G4endl;

    analysisManager->Write();
    analysisManager->CloseFile();
}
