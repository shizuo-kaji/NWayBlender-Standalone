/**
 * @file Application.cpp
 * @brief Main application state implementation
 */

#include "Application.h"
#include <iostream>

Application::Application()
    : blendMode(BM_LOG3)
    , tetMode(TM_FACE)
    , numIterations(1)
    , globalRotation(0.0)
    , visualizationMultiplier(1.0)
    , rotationConsistency(false)
    , areaWeighted(false)
    , enableARAP(true)
    , visualizeEnergy(false)
    , weightControllerMode(false)
    , selectedControlPoint(-1)
    , needsRecompute(true)
    , needsInitialization(true) {
}

Application::~Application() {
}

bool Application::loadBaseMesh(const std::string& path) {
    if (!baseMesh.loadFromFile(path)) {
        std::cerr << "Failed to load base mesh from " << path << std::endl;
        return false;
    }

    // Clear existing blend meshes and output
    blendMeshes.clear();
    meshWeights.clear();
    outputMesh.clear();

    needsInitialization = true;
    needsRecompute = true;

    std::cout << "Base mesh loaded successfully" << std::endl;
    return true;
}

int Application::addBlendMesh(const std::string& path) {
    Mesh mesh;
    if (!mesh.loadFromFile(path)) {
        std::cerr << "Failed to load blend mesh from " << path << std::endl;
        return -1;
    }

    // Validate topology matches base mesh
    if (baseMesh.isValid()) {
        if (mesh.numVertices() != baseMesh.numVertices() ||
            mesh.numFaces() != baseMesh.numFaces()) {
            std::cerr << "Error: Blend mesh topology doesn't match base mesh" << std::endl;
            std::cerr << "  Base: " << baseMesh.numVertices() << " verts, "
                      << baseMesh.numFaces() << " faces" << std::endl;
            std::cerr << "  Blend: " << mesh.numVertices() << " verts, "
                      << mesh.numFaces() << " faces" << std::endl;
            return -1;
        }
    }

    blendMeshes.push_back(mesh);
    meshWeights.push_back(0.0);  // Start with zero weight

    needsInitialization = true;
    needsRecompute = true;

    int index = (int)blendMeshes.size() - 1;
    std::cout << "Blend mesh " << index << " added: " << mesh.name << std::endl;
    return index;
}

void Application::removeBlendMesh(int index) {
    if (index < 0 || index >= (int)blendMeshes.size()) {
        std::cerr << "Invalid blend mesh index: " << index << std::endl;
        return;
    }

    blendMeshes.erase(blendMeshes.begin() + index);
    meshWeights.erase(meshWeights.begin() + index);

    needsInitialization = true;
    needsRecompute = true;

    std::cout << "Blend mesh " << index << " removed" << std::endl;
}

void Application::clearAll() {
    baseMesh.clear();
    blendMeshes.clear();
    outputMesh.clear();
    meshWeights.clear();
    controlPoints.clear();
    barycentricWeights.clear();

    needsInitialization = true;
    needsRecompute = true;

    std::cout << "All meshes cleared" << std::endl;
}

bool Application::exportOutput(const std::string& path) {
    if (!outputMesh.isValid()) {
        std::cerr << "No output mesh to export" << std::endl;
        return false;
    }

    return outputMesh.saveToFile(path);
}

bool Application::initialize() {
    if (!isReadyToBlend()) {
        std::cerr << "Cannot initialize: need base mesh and at least one blend mesh" << std::endl;
        return false;
    }

    if (!validateMeshTopology()) {
        std::cerr << "Cannot initialize: mesh topology mismatch" << std::endl;
        return false;
    }

    std::cout << "Initializing blending engine..." << std::endl;

    // Compute tetrahedral structure for base mesh
    if (!baseMesh.computeTetStructure(tetMode)) {
        std::cerr << "Failed to compute tet structure for base mesh" << std::endl;
        return false;
    }

    // Compute tetrahedral structure for all blend meshes
    for (size_t i = 0; i < blendMeshes.size(); i++) {
        if (!blendMeshes[i].computeTetStructure(tetMode)) {
            std::cerr << "Failed to compute tet structure for blend mesh " << i << std::endl;
            return false;
        }
    }

    // Setup NWayBlender engine
    blender.setBlendMode(blendMode);
    blender.setTetMode(tetMode);
    blender.setNumIterations(numIterations);
    blender.setRotationConsistency(rotationConsistency);
    blender.setAreaWeighted(areaWeighted);
    blender.setInitRotation(globalRotation);

    blender.setBaseMesh(baseMesh);
    for (const auto& mesh : blendMeshes) {
        blender.addBlendMesh(mesh);
    }

    if (!blender.initialize()) {
        std::cerr << "Failed to initialize NWayBlender engine" << std::endl;
        return false;
    }

    // Initialize output mesh with base mesh
    outputMesh = baseMesh;

    needsInitialization = false;
    needsRecompute = true;

    std::cout << "Initialization complete" << std::endl;
    return true;
}

bool Application::computeBlend() {
    if (!isReadyToBlend()) {
        std::cerr << "Cannot compute blend: not ready" << std::endl;
        return false;
    }

    if (needsInitialization) {
        if (!initialize()) {
            return false;
        }
    }

    // Update blender parameters if changed
    blender.setBlendMode(blendMode);
    blender.setNumIterations(numIterations);
    blender.setRotationConsistency(rotationConsistency);
    blender.setInitRotation(globalRotation);

    // Compute the blend
    if (!blender.computeBlend(meshWeights, outputMesh, visualizeEnergy, visualizationMultiplier)) {
        std::cerr << "Failed to compute blend" << std::endl;
        return false;
    }

    std::cout << "Blend computed successfully" << std::endl;
    needsRecompute = false;
    return true;
}

int Application::addControlPoint(const Eigen::Vector3d& pos) {
    controlPoints.push_back(pos);
    needsRecompute = true;
    int index = (int)controlPoints.size() - 1;
    std::cout << "Control point " << index << " added at (" << pos.transpose() << ")" << std::endl;
    return index;
}

void Application::removeControlPoint(int index) {
    if (index < 0 || index >= (int)controlPoints.size()) {
        std::cerr << "Invalid control point index: " << index << std::endl;
        return;
    }

    controlPoints.erase(controlPoints.begin() + index);
    if (selectedControlPoint == index) {
        selectedControlPoint = -1;
    } else if (selectedControlPoint > index) {
        selectedControlPoint--;
    }

    needsRecompute = true;
    std::cout << "Control point " << index << " removed" << std::endl;
}

void Application::updateControlPoint(int index, const Eigen::Vector3d& pos) {
    if (index < 0 || index >= (int)controlPoints.size()) {
        std::cerr << "Invalid control point index: " << index << std::endl;
        return;
    }

    controlPoints[index] = pos;
    needsRecompute = true;
}

void Application::computeBarycentricWeights() {
    if (controlPoints.empty()) {
        std::cout << "No control points to compute weights from" << std::endl;
        return;
    }

    if (blendMeshes.empty()) {
        std::cout << "No blend meshes to assign weights to" << std::endl;
        return;
    }

    int numControlPoints = (int)controlPoints.size();
    int numBlendMeshes = (int)blendMeshes.size();

    std::cout << "Computing barycentric weights: " << numControlPoints
              << " control points → " << numBlendMeshes << " blend meshes" << std::endl;

    // Set control points in weight controller
    weightController.setControlPoints(controlPoints);

    // Compute weights at blend mesh centers (simplified version)
    // In a full implementation, you'd compute per-vertex weights or use a query point
    meshWeights.resize(numBlendMeshes);

    for (int i = 0; i < numBlendMeshes; i++) {
        // Use first vertex of each blend mesh as representative point
        std::vector<Eigen::Vector3d> vertices = blendMeshes[i].getVerticesAsVector3d();
        if (!vertices.empty()) {
            Eigen::Vector3d center = vertices[0];  // Simplified: use first vertex
            std::vector<double> weights = weightController.computeWeights(center);

            // For now, use the weight of the first control point
            // In a full implementation, you might map control points to blend meshes
            if (i < (int)weights.size()) {
                meshWeights[i] = weights[i];
            } else {
                meshWeights[i] = 0.0;
            }
        }
    }

    // Normalize mesh weights
    double sum = 0.0;
    for (double w : meshWeights) {
        sum += w;
    }
    if (sum > 0.0) {
        for (double& w : meshWeights) {
            w /= sum;
        }
    }

    needsRecompute = true;
    std::cout << "Barycentric weights computed" << std::endl;
}

void Application::onMeshWeightChanged(int meshIndex, double weight) {
    if (meshIndex < 0 || meshIndex >= (int)meshWeights.size()) {
        std::cerr << "Invalid mesh index: " << meshIndex << std::endl;
        return;
    }

    meshWeights[meshIndex] = weight;
    needsRecompute = true;
}

void Application::onBlendModeChanged(short mode) {
    blendMode = mode;
    blender.setBlendMode(mode);
    needsRecompute = true;
}

void Application::onTetModeChanged(short mode) {
    tetMode = mode;
    blender.setTetMode(mode);
    needsInitialization = true;  // Need to rebuild tet structures
    needsRecompute = true;
}

void Application::onParameterChanged() {
    needsRecompute = true;
}

bool Application::isReadyToBlend() const {
    return baseMesh.isValid() && !blendMeshes.empty();
}

bool Application::validateMeshTopology() const {
    if (!baseMesh.isValid()) {
        return false;
    }

    int baseNumVerts = baseMesh.numVertices();
    int baseNumFaces = baseMesh.numFaces();

    for (const auto& mesh : blendMeshes) {
        if (mesh.numVertices() != baseNumVerts || mesh.numFaces() != baseNumFaces) {
            return false;
        }
    }

    return true;
}

void Application::updateWeightsVector() {
    meshWeights.resize(blendMeshes.size(), 0.0);
}
