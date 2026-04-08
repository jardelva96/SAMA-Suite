#============================================================================
# SAMA-Suite Detector Simulation
# Simulação GEANT4 do sistema de detectores para monitoramento de fluxo
# de partículas e densidade do plasma na Anomalia Magnética do Atlântico Sul
#============================================================================
cmake_minimum_required(VERSION 3.16...3.27)
project(SAMA_Suite_Sim VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#----------------------------------------------------------------------------
# Find Geant4
#----------------------------------------------------------------------------
find_package(Geant4 REQUIRED ui_all vis_all)
include(${Geant4_USE_FILE})

#----------------------------------------------------------------------------
# Source files
#----------------------------------------------------------------------------
file(GLOB headers ${PROJECT_SOURCE_DIR}/include/*.hh)
file(GLOB sources ${PROJECT_SOURCE_DIR}/src/*.cc)

#----------------------------------------------------------------------------
# Executable
#----------------------------------------------------------------------------
add_executable(sama_suite sama_suite.cc ${sources} ${headers})
target_include_directories(sama_suite PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(sama_suite ${Geant4_LIBRARIES})

#----------------------------------------------------------------------------
# Copy macros to build directory
#----------------------------------------------------------------------------
set(SAMA_MACROS
    macros/init_vis.mac
    macros/vis.mac
    macros/run_protons.mac
    macros/run_electrons.mac
    macros/run_trapped_spectrum.mac
    macros/run_shielding_study.mac
)

foreach(_macro ${SAMA_MACROS})
    configure_file(
        ${PROJECT_SOURCE_DIR}/${_macro}
        ${PROJECT_BINARY_DIR}/${_macro}
        COPYONLY
    )
endforeach()

#----------------------------------------------------------------------------
# Install
#----------------------------------------------------------------------------
install(TARGETS sama_suite DESTINATION bin)
