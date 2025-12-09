/**
 * @file NWayBlender.h
 * @brief N-Way blending engine
 * @section LICENSE The MIT License
 * @version 1.0
 * @date 2025
 */

#pragma once

#include "Mesh.h"
#include "blendAff.h"
#include "laplacian.h"
#include "affinelib.h"
#include "deformerConst.h"
#include <vector>
#include <set>
#include <queue>

using namespace Eigen;
using namespace AffineLib;
using namespace Tetrise;

/**
 * @brief N-Way blending engine
 *
 * Implements the core blending algorithm from the Maya plugin.
 * Handles tetrahedralization, parametrization, and ARAP-based blending.
 */
class NWayBlender {
public:
    /**
     * @brief Constructor
     */
    NWayBlender();

    /**
     * @brief Destructor
     */
    ~NWayBlender();

    /**
     * @brief Set the base (reference) mesh
     * @param mesh Base mesh
     */
    void setBaseMesh(const Mesh& mesh);

    /**
     * @brief Add a blend target mesh
     * @param mesh Blend mesh (must have same topology as base)
     */
    void addBlendMesh(const Mesh& mesh);

    /**
     * @brief Clear all meshes
     */
    void clearMeshes();

    /**
     * @brief Set blending parameters
     */
    void setBlendMode(short mode) { blendMode = mode; needsParametrization = true; }
    void setTetMode(short mode) { tetMode = mode; needsInitialization = true; }
    void setNumIterations(short iters) { numIterations = iters; }
    void setRotationConsistency(bool enable) { rotationConsistency = enable; needsParametrization = true; }
    void setAreaWeighted(bool enable) { areaWeighted = enable; needsInitialization = true; }
    void setInitRotation(double angle) { initRotationAngle = angle; }

    /**
     * @brief Initialize the blending engine
     *
     * Builds tetrahedral structures, computes adjacency, sets up ARAP solver.
     * Must be called before computeBlend().
     *
     * @return true if successful
     */
    bool initialize();

    /**
     * @brief Compute N-way blended mesh
     *
     * @param weights Per-mesh blend weights
     * @param output Output mesh (will be updated with blended result)
     * @param visualizeEnergy If true, compute and store energy values
     * @param visualizationMultiplier Scaling factor for energy visualization
     * @return true if successful
     */
    bool computeBlend(const std::vector<double>& weights,
                     Mesh& output,
                     bool visualizeEnergy = false,
                     double visualizationMultiplier = 1.0);

    /**
     * @brief Get the last computed energy values
     */
    const std::vector<double>& getVertexEnergy() const { return ptsEnergy; }

    /**
     * @brief Check if blender is initialized
     */
    bool isInitialized() const { return !needsInitialization; }

    /**
     * @brief Get number of blend meshes
     */
    int numBlendMeshes() const { return (int)blendMeshes.size(); }

private:
    // ========== Mesh Data ==========
    Mesh baseMesh;                              // Base mesh
    std::vector<Mesh> blendMeshes;              // Blend target meshes
    std::vector<Vector3d> pts;                  // Base mesh vertices
    int numPts;                                 // Number of vertices

    // ========== Tetrahedral Structure ==========
    Laplacian solver;                           // ARAP solver
    std::vector<int> faceList;                  // Triangulated faces
    std::vector<edge> edgeList;                 // Edge topology
    std::vector<vertex> vertexList;             // Vertex connectivity
    std::vector<std::vector<int>> adjacencyList; // Tet adjacency graph

    // ========== Parametrized Blend Targets ==========
    // One vector per blend mesh, each containing per-tet transformations
    std::vector<std::vector<Matrix3d>> logR;    // Log of rotations
    std::vector<std::vector<Matrix3d>> R;       // Rotations
    std::vector<std::vector<Matrix3d>> logS;    // Log of symmetric part
    std::vector<std::vector<Matrix3d>> S;       // Symmetric part
    std::vector<std::vector<Matrix3d>> GL;      // Linear part of affine
    std::vector<std::vector<Matrix3d>> logGL;   // Log of linear part
    std::vector<std::vector<Vector3d>> L;       // Translation part
    std::vector<std::vector<Vector4d>> quat;    // Quaternions

    // ========== Temporary Storage ==========
    std::vector<Matrix4d> Q;                    // Temp tet matrices
    std::vector<double> dummy_weight;           // Temp weights
    std::vector<double> ptsEnergy;              // Per-vertex energy

    // ========== Parameters ==========
    short blendMode;                            // BM_SRL, BM_LOG3, etc.
    short tetMode;                              // TM_FACE, TM_EDGE, etc.
    short numIterations;                        // ARAP iterations
    bool rotationConsistency;                   // Enable rotation consistency
    bool areaWeighted;                          // Use area-weighted blending
    double initRotationAngle;                   // Initial rotation (degrees)

    // ========== State Flags ==========
    bool needsInitialization;                   // Need to rebuild tet structure
    bool needsParametrization;                  // Need to reparametrize meshes
    int numParametrized;                        // Number of parametrized meshes

    // ========== Internal Methods ==========

    /**
     * @brief Parametrize a single blend mesh
     *
     * Computes relative transformation per tet and parametrizes based on blend mode.
     *
     * @param meshIndex Index of blend mesh to parametrize
     */
    void parametrizeBlendMesh(int meshIndex);

    /**
     * @brief Compute rotation consistency for a blend mesh
     *
     * Uses BFS traversal to pick consistent rotation branches.
     *
     * @param meshIndex Index of blend mesh
     */
    void computeRotationConsistency(int meshIndex);

    /**
     * @brief Blend parametrized transformations
     *
     * Linearly blends the parametrized components based on weights.
     *
     * @param weights Per-mesh weights
     * @param AR Output: blended rotation/linear part
     * @param AS Output: blended symmetric/scale part
     * @param AL Output: blended translation
     */
    void blendTransformations(const std::vector<double>& weights,
                             std::vector<Matrix3d>& AR,
                             std::vector<Matrix3d>& AS,
                             std::vector<Vector3d>& AL);

    /**
     * @brief Compute ARAP energy per tet
     *
     * @param newPts Current vertex positions
     * @param AS Target symmetric part
     * @param AR Output: fitted rotation
     * @param tetEnergy Output: per-tet energy
     */
    void computeEnergy(const std::vector<Vector3d>& newPts,
                      const std::vector<Matrix3d>& AS,
                      std::vector<Matrix3d>& AR,
                      std::vector<double>& tetEnergy);
};
