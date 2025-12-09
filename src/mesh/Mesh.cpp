/**
 * @file Mesh.cpp
 * @brief Mesh data structure implementation
 */

#include "Mesh.h"
#include "MeshUtils.h"
#include <igl/readOBJ.h>
#include <igl/readPLY.h>
#include <igl/writeOBJ.h>
#include <igl/writePLY.h>
#include <iostream>

Mesh::Mesh() : numTet(0), dim(0) {
}

bool Mesh::loadFromFile(const std::string& path) {
    clear();

    // Extract file extension
    size_t dotPos = path.find_last_of(".");
    if (dotPos == std::string::npos) {
        std::cerr << "Error: No file extension in " << path << std::endl;
        return false;
    }

    std::string ext = path.substr(dotPos + 1);

    // Convert extension to lowercase
    for (char& c : ext) {
        c = std::tolower(c);
    }

    bool success = false;

    if (ext == "obj") {
        success = igl::readOBJ(path, V, F);
    } else if (ext == "ply") {
        success = igl::readPLY(path, V, F);
    } else {
        std::cerr << "Error: Unsupported file format '" << ext << "'" << std::endl;
        return false;
    }

    if (!success) {
        std::cerr << "Error: Failed to load mesh from " << path << std::endl;
        return false;
    }

    // Extract filename for mesh name
    size_t slashPos = path.find_last_of("/\\");
    if (slashPos != std::string::npos) {
        name = path.substr(slashPos + 1);
    } else {
        name = path;
    }

    std::cout << "Loaded mesh '" << name << "': "
              << V.rows() << " vertices, "
              << F.rows() << " faces" << std::endl;

    // Build topology
    buildTopology();

    return true;
}

bool Mesh::saveToFile(const std::string& path) const {
    if (!isValid()) {
        std::cerr << "Error: Cannot save invalid mesh" << std::endl;
        return false;
    }

    // Extract file extension
    size_t dotPos = path.find_last_of(".");
    if (dotPos == std::string::npos) {
        std::cerr << "Error: No file extension in " << path << std::endl;
        return false;
    }

    std::string ext = path.substr(dotPos + 1);

    // Convert extension to lowercase
    for (char& c : ext) {
        c = std::tolower(c);
    }

    bool success = false;

    if (ext == "obj") {
        success = igl::writeOBJ(path, V, F);
    } else if (ext == "ply") {
        success = igl::writePLY(path, V, F);
    } else {
        std::cerr << "Error: Unsupported file format '" << ext << "'" << std::endl;
        return false;
    }

    if (!success) {
        std::cerr << "Error: Failed to save mesh to " << path << std::endl;
        return false;
    }

    std::cout << "Saved mesh to " << path << std::endl;
    return true;
}

bool Mesh::computeTetStructure(short tetMode) {
    if (!isValid()) {
        std::cerr << "Error: Cannot compute tet structure for invalid mesh" << std::endl;
        return false;
    }

    // Convert vertices to Vector3d format
    std::vector<Vector3d> pts = getVerticesAsVector3d();

    // Build tetrahedral structure using existing tetrise.h functions
    dim = MeshUtils::buildTetStructure(tetMode, pts, tetList, faceList,
                                       edgeList, vertexList, tetMatrix, tetWeight);

    numTet = (int)tetList.size() / 4;

    // Precompute inverse matrices
    tetMatrixInverse.resize(numTet);
    for (int i = 0; i < numTet; i++) {
        tetMatrixInverse[i] = tetMatrix[i].inverse().eval();
    }

    std::cout << "Computed tet structure: " << numTet << " tetrahedra, dim=" << dim << std::endl;

    return true;
}

std::vector<Vector3d> Mesh::getVerticesAsVector3d() const {
    std::vector<Vector3d> pts(V.rows());
    for (int i = 0; i < V.rows(); i++) {
        pts[i] = V.row(i);
    }
    return pts;
}

void Mesh::updateFromVector3d(const std::vector<Vector3d>& pts) {
    int n = std::min((int)pts.size(), (int)V.rows());
    for (int i = 0; i < n; i++) {
        V.row(i) = pts[i];
    }
}

bool Mesh::isValid() const {
    return V.rows() > 0 && F.rows() > 0;
}

void Mesh::clear() {
    V.resize(0, 3);
    F.resize(0, 3);
    tetList.clear();
    tetMatrix.clear();
    tetMatrixInverse.clear();
    tetWeight.clear();
    faceList.clear();
    edgeList.clear();
    vertexList.clear();
    vertexEnergy.resize(0);
    numTet = 0;
    dim = 0;
    name.clear();
}

void Mesh::buildTopology() {
    int numVerts = (int)V.rows();
    int numFaces = (int)F.rows();

    // Build face list (flattened)
    faceList.resize(numFaces * 3);
    for (int i = 0; i < numFaces; i++) {
        faceList[3*i + 0] = F(i, 0);
        faceList[3*i + 1] = F(i, 1);
        faceList[3*i + 2] = F(i, 2);
    }

    // Build vertex list (connectivity)
    vertexList.resize(numVerts);
    for (int i = 0; i < numVerts; i++) {
        vertexList[i].index = i;
        vertexList[i].connectedTriangles.clear();
    }

    // Populate vertex connectivity from faces
    for (int i = 0; i < numFaces; i++) {
        for (int j = 0; j < 3; j++) {
            int v = F(i, j);
            int vNext = F(i, (j+1) % 3);
            int vPrev = F(i, (j+2) % 3);
            vertexList[v].connectedTriangles.push_back(vNext);
            vertexList[v].connectedTriangles.push_back(vPrev);
        }
    }

    // Build edge list using makeEdgeList from tetrise.h
    Tetrise::makeEdgeList(faceList, edgeList);
}
