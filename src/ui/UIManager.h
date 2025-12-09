/**
 * @file UIManager.h
 * @brief UI management with Polyscope/ImGui (to be implemented in Phase 3)
 * @section LICENSE The MIT License
 * @version 1.0
 * @date 2025
 */

#pragma once

#include "Application.h"

class UIManager {
public:
    UIManager(Application* app);
    ~UIManager();

    // TODO: Implement in Phase 3
    void initialize();
    void render();

private:
    Application* app;
};
