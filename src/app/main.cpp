/**
 * @file main.cpp
 * @brief N-Way Blender standalone application entry point (Phase 3 Enhanced)
 * @section LICENSE The MIT License
 * @version 1.0
 * @date 2025
 */

#include <iostream>
#include <polyscope/polyscope.h>
#include <polyscope/surface_mesh.h>

#include "Application.h"

// Global application instance
Application* app = nullptr;

// UI state
static char baseMeshPath[512] = "";
static char blendMeshPath[512] = "";
static char exportPath[512] = "output.obj";
static bool showBaseMesh = true;
static bool showBlendMeshes = true;
static bool showOutputMesh = true;
static float baseMeshOpacity = 0.5f;
static float blendMeshOpacity = 0.5f;
static float outputMeshOpacity = 1.0f;

// Callback function for ImGui UI
void callback() {
    ImGui::Begin("N-Way Blender");

    // File operations
    if (ImGui::CollapsingHeader("File", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Text("Load Base Mesh:");
        ImGui::InputText("##basepath", baseMeshPath, 512);
        ImGui::SameLine();
        if (ImGui::Button("Load##base")) {
            if (strlen(baseMeshPath) > 0) {
                std::cout << "Loading base mesh from: " << baseMeshPath << std::endl;
                if (app->loadBaseMesh(baseMeshPath)) {
                    // Register base mesh with Polyscope
                    auto* mesh = polyscope::registerSurfaceMesh("Base Mesh",
                                                                app->baseMesh.V,
                                                                app->baseMesh.F);
                    mesh->setTransparency(baseMeshOpacity);
                    mesh->setEnabled(showBaseMesh);
                    std::cout << "Base mesh loaded successfully" << std::endl;
                } else {
                    std::cerr << "Failed to load base mesh" << std::endl;
                }
            }
        }

        ImGui::Text("Add Blend Mesh:");
        ImGui::InputText("##blendpath", blendMeshPath, 512);
        ImGui::SameLine();
        if (ImGui::Button("Add##blend")) {
            if (strlen(blendMeshPath) > 0) {
                std::cout << "Adding blend mesh from: " << blendMeshPath << std::endl;
                int idx = app->addBlendMesh(blendMeshPath);
                if (idx >= 0) {
                    const Mesh& mesh = app->getBlendMesh(idx);
                    std::string name = "Blend Mesh " + std::to_string(idx);
                    auto* pmesh = polyscope::registerSurfaceMesh(name, mesh.V, mesh.F);
                    pmesh->setTransparency(blendMeshOpacity);
                    pmesh->setEnabled(showBlendMeshes);
                    std::cout << "Blend mesh " << idx << " added: " << mesh.name << std::endl;
                } else {
                    std::cerr << "Failed to add blend mesh" << std::endl;
                }
            }
        }

        if (app && app->isReadyToBlend()) {
            ImGui::Separator();
            ImGui::Text("Export Output:");
            ImGui::InputText("##exportpath", exportPath, 512);
            ImGui::SameLine();
            if (ImGui::Button("Export##output")) {
                if (strlen(exportPath) > 0) {
                    std::cout << "Exporting output mesh to: " << exportPath << std::endl;
                    if (app->outputMesh.saveToFile(exportPath)) {
                        std::cout << "Output mesh exported successfully" << std::endl;
                    } else {
                        std::cerr << "Failed to export output mesh" << std::endl;
                    }
                }
            }
        }
    }

    // Status
    if (ImGui::CollapsingHeader("Status", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (app->baseMesh.isValid()) {
            ImGui::Text("Base mesh: %s", app->baseMesh.name.c_str());
            ImGui::Text("  %d vertices, %d faces",
                       app->baseMesh.numVertices(),
                       app->baseMesh.numFaces());
        } else {
            ImGui::TextDisabled("No base mesh loaded");
        }

        ImGui::Separator();

        if (app->numBlendMeshes() > 0) {
            ImGui::Text("Blend meshes: %d", app->numBlendMeshes());
        } else {
            ImGui::TextDisabled("No blend meshes loaded");
        }
    }

    // Visualization controls
    if (ImGui::CollapsingHeader("Visualization")) {
        ImGui::Text("Mesh Visibility:");
        if (ImGui::Checkbox("Show Base Mesh", &showBaseMesh)) {
            if (polyscope::hasSurfaceMesh("Base Mesh")) {
                polyscope::getSurfaceMesh("Base Mesh")->setEnabled(showBaseMesh);
            }
        }

        if (ImGui::Checkbox("Show Blend Meshes", &showBlendMeshes)) {
            for (int i = 0; i < app->numBlendMeshes(); i++) {
                std::string name = "Blend Mesh " + std::to_string(i);
                if (polyscope::hasSurfaceMesh(name)) {
                    polyscope::getSurfaceMesh(name)->setEnabled(showBlendMeshes);
                }
            }
        }

        if (ImGui::Checkbox("Show Output Mesh", &showOutputMesh)) {
            if (polyscope::hasSurfaceMesh("Output Mesh")) {
                polyscope::getSurfaceMesh("Output Mesh")->setEnabled(showOutputMesh);
            }
        }

        ImGui::Separator();
        ImGui::Text("Mesh Transparency:");

        if (ImGui::SliderFloat("Base Opacity", &baseMeshOpacity, 0.0f, 1.0f)) {
            if (polyscope::hasSurfaceMesh("Base Mesh")) {
                polyscope::getSurfaceMesh("Base Mesh")->setTransparency(baseMeshOpacity);
            }
        }

        if (ImGui::SliderFloat("Blend Opacity", &blendMeshOpacity, 0.0f, 1.0f)) {
            for (int i = 0; i < app->numBlendMeshes(); i++) {
                std::string name = "Blend Mesh " + std::to_string(i);
                if (polyscope::hasSurfaceMesh(name)) {
                    polyscope::getSurfaceMesh(name)->setTransparency(blendMeshOpacity);
                }
            }
        }

        if (ImGui::SliderFloat("Output Opacity", &outputMeshOpacity, 0.0f, 1.0f)) {
            if (polyscope::hasSurfaceMesh("Output Mesh")) {
                polyscope::getSurfaceMesh("Output Mesh")->setTransparency(outputMeshOpacity);
            }
        }
    }

    // Blending controls
    if (app->isReadyToBlend()) {
        if (ImGui::CollapsingHeader("Blending", ImGuiTreeNodeFlags_DefaultOpen)) {
            // Weight sliders
            ImGui::Text("Blend Weights:");
            for (int i = 0; i < app->numBlendMeshes(); i++) {
                std::string label = "Weight " + std::to_string(i);
                float weight = (float)app->meshWeights[i];
                if (ImGui::SliderFloat(label.c_str(), &weight, 0.0f, 1.0f)) {
                    app->meshWeights[i] = (double)weight;
                    app->needsRecompute = true;  // Mark blend as needing update
                }
            }

            ImGui::Separator();

            // Blend mode
            const char* blend_modes[] = { "SRL", "SSE", "SQL", "LOG3", "LOG4", "SlRL", "AFF" };
            int current_mode = app->blendMode;
            if (ImGui::Combo("Blend Mode", &current_mode, blend_modes, 7)) {
                app->onBlendModeChanged((short)current_mode);
            }

            // Tet mode
            const char* tet_modes[] = { "Face", "Edge", "Vertex", "VFace" };
            int current_tet_mode = app->tetMode;
            if (ImGui::Combo("Tet Mode", &current_tet_mode, tet_modes, 4)) {
                app->tetMode = (short)current_tet_mode;
                app->onParameterChanged();
                std::cout << "Tet mode changed to: " << tet_modes[current_tet_mode] << std::endl;
                std::cout << "Note: Changing tet mode requires reloading meshes" << std::endl;
            }

            ImGui::Separator();

            // Parameters
            int iters = app->numIterations;
            if (ImGui::SliderInt("Iterations", &iters, 1, 10)) {
                app->numIterations = (short)iters;
                app->onParameterChanged();
            }

            bool rotCons = app->rotationConsistency;
            if (ImGui::Checkbox("Rotation Consistency", &rotCons)) {
                app->rotationConsistency = rotCons;
                app->onParameterChanged();
            }

            bool areaWeighted = app->areaWeighted;
            if (ImGui::Checkbox("Area Weighted", &areaWeighted)) {
                app->areaWeighted = areaWeighted;
                app->onParameterChanged();
            }

            ImGui::Separator();

            // Energy visualization
            bool visEnergy = app->visualizeEnergy;
            if (ImGui::Checkbox("Visualize Energy", &visEnergy)) {
                app->visualizeEnergy = visEnergy;
                app->onParameterChanged();
            }

            if (app->visualizeEnergy) {
                float visMult = (float)app->visualizationMultiplier;
                if (ImGui::SliderFloat("Energy Multiplier", &visMult, 0.1f, 10.0f)) {
                    app->visualizationMultiplier = (double)visMult;
                }
            }

            ImGui::Separator();

            // Compute blend button
            if (app->needsRecompute) {
                if (ImGui::Button("Compute Blend", ImVec2(-1, 30))) {
                    std::cout << "\nComputing blend..." << std::endl;
                    if (app->computeBlend()) {
                        std::cout << "Blend computation successful" << std::endl;
                        app->needsRecompute = false;  // Clear flag after successful blend

                        // Update or create output mesh visualization
                        if (polyscope::hasSurfaceMesh("Output Mesh")) {
                            polyscope::getSurfaceMesh("Output Mesh")->updateVertexPositions(app->outputMesh.V);
                        } else {
                            auto* mesh = polyscope::registerSurfaceMesh("Output Mesh",
                                                                        app->outputMesh.V,
                                                                        app->outputMesh.F);
                            mesh->setTransparency(outputMeshOpacity);
                            mesh->setEnabled(showOutputMesh);
                        }

                        // Update energy visualization if enabled
                        if (app->visualizeEnergy && app->outputMesh.vertexEnergy.size() > 0) {
                            polyscope::getSurfaceMesh("Output Mesh")
                                ->addVertexScalarQuantity("Energy", app->outputMesh.vertexEnergy)
                                ->setEnabled(true);
                            std::cout << "Energy visualization updated" << std::endl;
                        }
                    } else {
                        std::cerr << "Blend computation failed" << std::endl;
                    }
                }
            } else {
                ImGui::TextDisabled("Blend is up to date");
            }
        }
    }

    ImGui::End();
}

int main(int argc, char** argv) {
    std::cout << "N-Way Blender - Standalone Application" << std::endl;
    std::cout << "=======================================" << std::endl;

    // Create application instance
    app = new Application();

    // Initialize Polyscope
    polyscope::init();

    // Set up ImGui callback
    polyscope::state::userCallback = callback;

    // For testing: load meshes from command line if provided
    // Usage: ./nway_blender base.obj blend1.obj blend2.obj ...
    if (argc > 1) {
        std::cout << "Loading base mesh from: " << argv[1] << std::endl;
        if (app->loadBaseMesh(argv[1])) {
            // Register base mesh with Polyscope
            auto* mesh = polyscope::registerSurfaceMesh("Base Mesh",
                                                        app->baseMesh.V,
                                                        app->baseMesh.F);
            mesh->setTransparency(baseMeshOpacity);
            mesh->setEnabled(showBaseMesh);

            // Load blend meshes if provided
            for (int i = 2; i < argc; i++) {
                std::cout << "Loading blend mesh from: " << argv[i] << std::endl;
                int idx = app->addBlendMesh(argv[i]);
                if (idx >= 0) {
                    const Mesh& blendMesh = app->getBlendMesh(idx);
                    std::string name = "Blend Mesh " + std::to_string(idx);
                    auto* bmesh = polyscope::registerSurfaceMesh(name, blendMesh.V, blendMesh.F);
                    bmesh->setTransparency(blendMeshOpacity);
                    bmesh->setEnabled(showBlendMeshes);
                    std::cout << "Blend mesh " << idx << " added: " << blendMesh.name << std::endl;
                }
            }

            // Initialize blending engine
            std::cout << "Initializing blending engine..." << std::endl;
            if (app->initialize()) {
                std::cout << "Initialization complete" << std::endl;
            }
        }
    }

    std::cout << "\nReady to blend! Use the UI to adjust weights." << std::endl;

    // Show the GUI
    polyscope::show();

    // Cleanup
    delete app;

    return 0;
}
