/**
 * @file Mesh.h
 * @brief Mesh data structure for N-Way Blender
 * @section LICENSE The MIT License
 * @version 1.0
 * @date 2025
 */

#pragma once

#include <string>
#include <vector>
#include <Eigen/Dense>
#include <Eigen/StdVector>

#include "tetrise.h"

using namespace Eigen;

// edge and vertex types are available from tetrise.h (in global namespace)

/**
 * @brief Mesh data structure using libigl conventions
 *
 * Stores mesh geometry (vertices, faces) and topological information
 * (tetrahedra, edges, vertex connectivity) needed for N-way blending.
 */
class Mesh {
public:
    // Geometry (libigl format)
    Eigen::MatrixXd V;                    // Vertices (n × 3)
    Eigen::MatrixXi F;                    // Faces (m × 3), triangulated

    // Tetrahedral structure (from tetrise.h)
    std::vector<int> tetList;             // Flattened tet indices [4*numTet]
    std::vector<Matrix4d> tetMatrix;      // Tet transformation matrices
    std::vector<Matrix4d> tetMatrixInverse; // Precomputed inverses
    std::vector<double> tetWeight;        // Per-tet weights
    int numTet;                           // Number of tetrahedra
    int dim;                              // Including ghost vertices

    // Auxiliary structures (from tetrise.h)
    std::vector<int> faceList;            // Flattened triangle indices
    std::vector<edge> edgeList;           // Edge topology
    std::vector<vertex> vertexList;       // Vertex connectivity

    // Visualization
    Eigen::VectorXd vertexEnergy;         // For energy visualization

    // Name/ID
    std::string name;                     // Mesh identifier

    /**
     * @brief Default constructor
     */
    Mesh();

    /**
     * @brief Load mesh from file (OBJ, PLY, etc.)
     * @param path File path
     * @return true if successful
     */
    bool loadFromFile(const std::string& path);

    /**
     * @brief Save mesh to file
     * @param path Output file path
     * @return true if successful
     */
    bool saveToFile(const std::string& path) const;

    /**
     * @brief Compute tetrahedral structure for blending
     * @param tetMode Tetrahedralization mode (TM_FACE, TM_EDGE, TM_VERTEX, TM_VFACE)
     * @return true if successful
     */
    bool computeTetStructure(short tetMode);

    /**
     * @brief Get vertices as vector of Vector3d (for compatibility with existing code)
     * @return Vector of 3D vertices
     */
    std::vector<Vector3d> getVerticesAsVector3d() const;

    /**
     * @brief Update vertex positions from Vector3d array
     * @param pts New vertex positions
     */
    void updateFromVector3d(const std::vector<Vector3d>& pts);

    /**
     * @brief Check if mesh has valid geometry
     * @return true if mesh has vertices and faces
     */
    bool isValid() const;

    /**
     * @brief Get number of vertices
     */
    int numVertices() const { return (int)V.rows(); }

    /**
     * @brief Get number of faces
     */
    int numFaces() const { return (int)F.rows(); }

    /**
     * @brief Clear all mesh data
     */
    void clear();

private:
    /**
     * @brief Build face, edge, and vertex lists from F matrix
     */
    void buildTopology();
};
