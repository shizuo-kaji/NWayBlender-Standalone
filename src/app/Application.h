/**
 * @file Application.h
 * @brief Main application state management
 * @section LICENSE The MIT License
 * @version 1.0
 * @date 2025
 */

#pragma once

#include <vector>
#include <string>
#include <Eigen/Dense>
#include "Mesh.h"
#include "NWayBlender.h"
#include "WeightController.h"
#include "deformerConst.h"

using namespace Eigen;

/**
 * @brief Main application class managing all state
 *
 * Coordinates between mesh data, blending parameters, and UI.
 * This class is the central point of the application architecture.
 */
class Application {
public:
    // ========== Mesh Data ==========
    Mesh baseMesh;                              // Reference/base mesh
    std::vector<Mesh> blendMeshes;              // Blend target meshes
    Mesh outputMesh;                            // Real-time blended output

    // ========== Blending Parameters ==========
    std::vector<double> meshWeights;            // Weight per blend mesh
    short blendMode;                            // BM_SRL, BM_LOG3, etc.
    short tetMode;                              // TM_FACE, TM_EDGE, etc.
    short numIterations;                        // ARAP iterations
    double globalRotation;                      // Global rotation parameter
    double visualizationMultiplier;             // Energy visualization scale
    bool rotationConsistency;                   // Enable rotation consistency
    bool areaWeighted;                          // Area-weighted blending
    bool enableARAP;                            // Enable ARAP deformation
    bool visualizeEnergy;                       // Show energy colors

    // ========== Weight Controller ==========
    std::vector<Eigen::Vector3d> controlPoints; // Control point positions
    std::vector<std::vector<double>> barycentricWeights; // Per-vertex weights from control points
    bool weightControllerMode;                  // Enable weight controller
    int selectedControlPoint;                   // -1 if none selected

    // ========== State Flags ==========
    bool needsRecompute;                        // Blend needs recomputation
    bool needsInitialization;                   // Blending engine needs initialization

    /**
     * @brief Constructor with default parameters
     */
    Application();

    /**
     * @brief Destructor
     */
    ~Application();

    // ========== Mesh Management ==========

    /**
     * @brief Load base mesh from file
     * @param path File path
     * @return true if successful
     */
    bool loadBaseMesh(const std::string& path);

    /**
     * @brief Add a blend target mesh
     * @param path File path
     * @return Index of added mesh, or -1 on failure
     */
    int addBlendMesh(const std::string& path);

    /**
     * @brief Remove a blend mesh
     * @param index Index of mesh to remove
     */
    void removeBlendMesh(int index);

    /**
     * @brief Clear all meshes
     */
    void clearAll();

    /**
     * @brief Export output mesh to file
     * @param path Output file path
     * @return true if successful
     */
    bool exportOutput(const std::string& path);

    // ========== Blending Computation ==========

    /**
     * @brief Initialize the blending engine
     *
     * Must be called after loading meshes and before computeBlend().
     * Precomputes tetrahedral structures and ARAP solver.
     *
     * @return true if successful
     */
    bool initialize();

    /**
     * @brief Compute the N-way blended mesh
     *
     * Updates outputMesh based on current weights and parameters.
     * Called whenever weights or parameters change.
     *
     * @return true if successful
     */
    bool computeBlend();

    // ========== Weight Controller ==========

    /**
     * @brief Add a control point at position
     * @param pos 3D position
     * @return Index of added control point
     */
    int addControlPoint(const Eigen::Vector3d& pos);

    /**
     * @brief Remove a control point
     * @param index Control point index
     */
    void removeControlPoint(int index);

    /**
     * @brief Move a control point
     * @param index Control point index
     * @param pos New position
     */
    void updateControlPoint(int index, const Eigen::Vector3d& pos);

    /**
     * @brief Compute barycentric weights from control points
     *
     * Updates barycentricWeights based on control point positions.
     * Then applies these to meshWeights.
     */
    void computeBarycentricWeights();

    // ========== Parameter Callbacks ==========

    /**
     * @brief Called when a mesh weight changes
     * @param meshIndex Index of mesh
     * @param weight New weight value
     */
    void onMeshWeightChanged(int meshIndex, double weight);

    /**
     * @brief Called when blend mode changes
     * @param mode New blend mode
     */
    void onBlendModeChanged(short mode);

    /**
     * @brief Called when tet mode changes
     * @param mode New tet mode
     */
    void onTetModeChanged(short mode);

    /**
     * @brief Called when any parameter changes
     */
    void onParameterChanged();

    // ========== Validation ==========

    /**
     * @brief Check if application is ready to blend
     * @return true if base mesh and at least one blend mesh are loaded
     */
    bool isReadyToBlend() const;

    /**
     * @brief Validate mesh topology consistency
     *
     * Checks that all blend meshes have the same topology as base mesh.
     *
     * @return true if all meshes have matching vertex/face counts
     */
    bool validateMeshTopology() const;

    /**
     * @brief Get number of blend meshes
     */
    int numBlendMeshes() const { return (int)blendMeshes.size(); }

    /**
     * @brief Get a blend mesh by index
     * @param index Index of blend mesh
     * @return Reference to blend mesh
     */
    const Mesh& getBlendMesh(int index) const { return blendMeshes[index]; }

private:
    /**
     * @brief Ensure meshWeights vector has correct size
     */
    void updateWeightsVector();

    /**
     * @brief NWayBlending engine instance
     */
    NWayBlender blender;

    /**
     * @brief Weight controller instance
     */
    WeightController weightController;
};
