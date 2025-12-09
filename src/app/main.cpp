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

// Weight controller widget state
static bool useWeightController = false;
static Eigen::Vector2d controlPoint(0.0, 0.0);  // Control point position in normalized coordinates [-1,1]

// Real-time update mode
static bool realtimeUpdate = false;

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
            // Weight controller mode toggle
            if (ImGui::Checkbox("Use Weight Controller", &useWeightController)) {
                if (useWeightController && app->numBlendMeshes() >= 1) {
                    // Initialize control point at center
                    controlPoint = Eigen::Vector2d(0.0, 0.0);
                }
            }
            ImGui::SameLine();
            ImGui::TextDisabled("(?)");
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Interactive N-gon weight controller using Mean Value Coordinates.\n"
                                  "Includes base mesh as vertex 'B'. Drag the control point inside for\n"
                                  "interpolation, outside for extrapolation.");
            }

            ImGui::Separator();

            if (useWeightController && app->numBlendMeshes() >= 1) {
                // Interactive 2D weight controller widget
                ImGui::Text("Weight Controller (MVC):");

                // Canvas settings
                const float canvasSize = 300.0f;
                const float radius = canvasSize * 0.35f;  // N-gon radius
                const ImVec2 canvasPos = ImGui::GetCursorScreenPos();
                const ImVec2 canvasCenter = ImVec2(canvasPos.x + canvasSize * 0.5f, canvasPos.y + canvasSize * 0.5f);

                // Draw canvas
                ImDrawList* draw_list = ImGui::GetWindowDrawList();
                ImGui::InvisibleButton("weight_canvas", ImVec2(canvasSize, canvasSize));

                // Canvas background
                draw_list->AddRectFilled(canvasPos, ImVec2(canvasPos.x + canvasSize, canvasPos.y + canvasSize),
                                        IM_COL32(50, 50, 50, 255));
                draw_list->AddRect(canvasPos, ImVec2(canvasPos.x + canvasSize, canvasPos.y + canvasSize),
                                  IM_COL32(100, 100, 100, 255));

                // Compute N-gon vertices (regular polygon)
                // N = numBlendMeshes + 1 (including base mesh)
                int numBlendMeshes = app->numBlendMeshes();
                int N = numBlendMeshes + 1;  // +1 for base mesh
                std::vector<Eigen::Vector2d> ngonVertices(N);
                std::vector<ImVec2> ngonVerticesScreen(N);

                for (int i = 0; i < N; i++) {
                    double angle = -M_PI / 2.0 + (2.0 * M_PI * i) / N;  // Start from top
                    ngonVertices[i] = Eigen::Vector2d(std::cos(angle), std::sin(angle));
                    ngonVerticesScreen[i] = ImVec2(
                        canvasCenter.x + ngonVertices[i].x() * radius,
                        canvasCenter.y + ngonVertices[i].y() * radius
                    );
                }

                // Draw N-gon
                for (int i = 0; i < N; i++) {
                    int next = (i + 1) % N;
                    draw_list->AddLine(ngonVerticesScreen[i], ngonVerticesScreen[next],
                                      IM_COL32(150, 150, 255, 255), 2.0f);
                }

                // Draw N-gon vertices (base + blend mesh labels)
                for (int i = 0; i < N; i++) {
                    // Different color for base mesh (vertex 0)
                    ImU32 color = (i == 0) ? IM_COL32(100, 255, 100, 255) : IM_COL32(200, 200, 255, 255);
                    draw_list->AddCircleFilled(ngonVerticesScreen[i], 6.0f, color);

                    // Label: "B" for base, "0", "1", "2", ... for blend meshes
                    std::string label = (i == 0) ? "B" : std::to_string(i - 1);
                    ImVec2 labelPos = ImVec2(
                        ngonVerticesScreen[i].x - 4.0f + ngonVertices[i].x() * 15.0f,
                        ngonVerticesScreen[i].y - 8.0f + ngonVertices[i].y() * 15.0f
                    );
                    draw_list->AddText(labelPos, IM_COL32(255, 255, 255, 255), label.c_str());
                }

                // Handle control point dragging
                bool controlPointMoved = false;
                if (ImGui::IsItemActive()) {
                    ImVec2 mousePos = ImGui::GetMousePos();
                    // Convert screen coordinates to normalized [-1, 1]
                    controlPoint.x() = (mousePos.x - canvasCenter.x) / radius;
                    controlPoint.y() = (mousePos.y - canvasCenter.y) / radius;
                    controlPointMoved = true;
                }

                // Draw control point
                ImVec2 controlPointScreen = ImVec2(
                    canvasCenter.x + controlPoint.x() * radius,
                    canvasCenter.y + controlPoint.y() * radius
                );
                draw_list->AddCircleFilled(controlPointScreen, 8.0f, IM_COL32(255, 100, 100, 255));
                draw_list->AddCircle(controlPointScreen, 8.0f, IM_COL32(255, 255, 255, 255), 16, 2.0f);

                // Compute weights using MVC
                if (controlPointMoved) {
                    std::vector<double> weights = app->getWeightController().computeMVC2D(controlPoint, ngonVertices);

                    // weights[0] = base mesh weight
                    // weights[1..N] = blend mesh weights
                    // Apply blend mesh weights (indices 1 to N-1 in MVC weights)
                    for (int i = 0; i < numBlendMeshes; i++) {
                        app->meshWeights[i] = weights[i + 1];
                    }

                    app->needsRecompute = true;
                }

                // Display computed weights
                // Re-compute for display (in case not moved this frame)
                std::vector<double> displayWeights = app->getWeightController().computeMVC2D(controlPoint, ngonVertices);

                ImGui::Text("Computed Weights:");
                ImGui::Text("  Base: %.3f", displayWeights[0]);
                for (int i = 0; i < numBlendMeshes; i++) {
                    ImGui::Text("  Mesh %d: %.3f", i, displayWeights[i + 1]);
                }

                // Check if inside or outside (using all MVC weights including base)
                double weightSum = 0.0;
                bool hasNegative = false;
                for (int i = 0; i < N; i++) {
                    weightSum += displayWeights[i];
                    if (displayWeights[i] < -0.001) {  // Small tolerance for numerical errors
                        hasNegative = true;
                    }
                }

                if (hasNegative || std::abs(weightSum - 1.0) > 0.01) {
                    ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Mode: Extrapolation");
                } else {
                    ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.5f, 1.0f), "Mode: Interpolation");
                }

            } else {
                // Manual weight sliders
                ImGui::Text("Blend Weights:");
                for (int i = 0; i < app->numBlendMeshes(); i++) {
                    std::string label = "Weight " + std::to_string(i);
                    float weight = (float)app->meshWeights[i];
                    if (ImGui::SliderFloat(label.c_str(), &weight, 0.0f, 1.0f)) {
                        app->meshWeights[i] = (double)weight;
                        app->needsRecompute = true;  // Mark blend as needing update
                    }
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

            // Real-time update toggle
            ImGui::Checkbox("Real-time Update", &realtimeUpdate);
            ImGui::SameLine();
            ImGui::TextDisabled("(?)");
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Automatically recompute blend when parameters change.\n"
                                  "Disable for manual control with 'Compute Blend' button.");
            }

            ImGui::Separator();

            // Auto-compute in real-time mode
            if (realtimeUpdate && app->needsRecompute) {
                if (app->computeBlend()) {
                    app->needsRecompute = false;  // Clear flag after successful blend

                    // Create translated copy for output mesh (position below base mesh)
                    Eigen::MatrixXd V_output = app->outputMesh.V;
                    V_output.col(1).array() -= 3.0;  // Offset down in Y

                    // Update or create output mesh visualization
                    if (polyscope::hasSurfaceMesh("Output Mesh")) {
                        polyscope::getSurfaceMesh("Output Mesh")->updateVertexPositions(V_output);
                    } else {
                        auto* mesh = polyscope::registerSurfaceMesh("Output Mesh",
                                                                    V_output,
                                                                    app->outputMesh.F);
                        mesh->setTransparency(outputMeshOpacity);
                        mesh->setEnabled(showOutputMesh);
                    }

                    // Update energy visualization if enabled
                    if (app->visualizeEnergy && app->outputMesh.vertexEnergy.size() > 0) {
                        polyscope::getSurfaceMesh("Output Mesh")
                            ->addVertexScalarQuantity("Energy", app->outputMesh.vertexEnergy)
                            ->setEnabled(true);
                    }
                }
            }

            // Manual compute blend button (only in non-realtime mode)
            if (!realtimeUpdate) {
                if (app->needsRecompute) {
                    if (ImGui::Button("Compute Blend", ImVec2(-1, 30))) {
                        std::cout << "\nComputing blend..." << std::endl;
                        if (app->computeBlend()) {
                            std::cout << "Blend computation successful" << std::endl;
                            app->needsRecompute = false;  // Clear flag after successful blend

                            // Create translated copy for output mesh (position below base mesh)
                            Eigen::MatrixXd V_output = app->outputMesh.V;
                            V_output.col(1).array() -= 3.0;  // Offset down in Y

                            // Update or create output mesh visualization
                            if (polyscope::hasSurfaceMesh("Output Mesh")) {
                                polyscope::getSurfaceMesh("Output Mesh")->updateVertexPositions(V_output);
                            } else {
                                auto* mesh = polyscope::registerSurfaceMesh("Output Mesh",
                                                                            V_output,
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
            } else {
                // Real-time mode status
                if (app->needsRecompute) {
                    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Computing...");
                } else {
                    ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Real-time: Active");
                }
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
            // Register base mesh with Polyscope (at origin)
            auto* mesh = polyscope::registerSurfaceMesh("Base Mesh",
                                                        app->baseMesh.V,
                                                        app->baseMesh.F);
            mesh->setTransparency(baseMeshOpacity);
            mesh->setEnabled(showBaseMesh);

            // Load blend meshes if provided
            // Position them in a row to the right
            const double spacing = 3.0;  // Distance between meshes
            for (int i = 2; i < argc; i++) {
                std::cout << "Loading blend mesh from: " << argv[i] << std::endl;
                int idx = app->addBlendMesh(argv[i]);
                if (idx >= 0) {
                    const Mesh& blendMesh = app->getBlendMesh(idx);

                    // Create translated copy of vertices
                    Eigen::MatrixXd V_translated = blendMesh.V;
                    double offset_x = spacing * (idx + 1);  // Offset each blend mesh to the right
                    V_translated.col(0).array() += offset_x;

                    std::string name = "Blend Mesh " + std::to_string(idx);
                    auto* bmesh = polyscope::registerSurfaceMesh(name, V_translated, blendMesh.F);
                    bmesh->setTransparency(blendMeshOpacity);
                    bmesh->setEnabled(showBlendMeshes);
                    std::cout << "Blend mesh " << idx << " added at offset (" << offset_x << ", 0, 0)" << std::endl;
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
