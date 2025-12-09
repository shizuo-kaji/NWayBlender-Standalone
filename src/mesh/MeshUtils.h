/**
 * @file MeshUtils.h
 * @brief Mesh utility functions (replacement for MeshMaya.h)
 * @section LICENSE The MIT License
 * @version 1.0
 * @date 2025
 */

#pragma once

#include <vector>
#include <Eigen/Dense>
#include "tetrise.h"

using namespace Eigen;
using namespace Tetrise;

/**
 * @brief Mesh utility functions wrapping tetrise.h for standalone use
 */
namespace MeshUtils {

    /**
     * @brief Build tetrahedral structure from vertex positions
     *
     * This function wraps the tetrise.h functions to build tet structure.
     * Replaces getMeshData() from MeshMaya.h.
     *
     * @param tetMode Tetrahedralization mode (TM_FACE, TM_EDGE, TM_VERTEX, TM_VFACE)
     * @param pts Vertex positions
     * @param tetList Output: flattened tet indices [4*numTet]
     * @param faceList Input/Output: triangulated face indices
     * @param edgeList Output: edge topology
     * @param vertexList Output: vertex connectivity
     * @param tetMatrix Output: tet transformation matrices
     * @param tetWeight Output: tet weights
     * @return Total dimension (including ghost vertices)
     */
    int buildTetStructure(short tetMode,
                         const std::vector<Vector3d>& pts,
                         std::vector<int>& tetList,
                         std::vector<int>& faceList,
                         std::vector<edge>& edgeList,
                         std::vector<vertex>& vertexList,
                         std::vector<Matrix4d>& tetMatrix,
                         std::vector<double>& tetWeight);

    /**
     * @brief Compute vertex colors from scalar values
     *
     * Maps scalar values (e.g., energy) to HSV colors.
     * Replaces visualise() from MeshMaya.h.
     *
     * @param vertexValues Scalar value per vertex
     * @param colors Output: RGB colors (n×3 matrix, values 0-1)
     */
    void computeVertexColors(const std::vector<double>& vertexValues,
                            Eigen::MatrixXd& colors);

    /**
     * @brief Compute vertex colors with multiplier
     *
     * @param vertexValues Scalar value per vertex
     * @param multiplier Scaling factor for visualization
     * @param colors Output: RGB colors (n×3 matrix)
     */
    void computeVertexColors(const std::vector<double>& vertexValues,
                            double multiplier,
                            Eigen::MatrixXd& colors);

} // namespace MeshUtils
