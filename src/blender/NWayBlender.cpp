/**
 * @file NWayBlender.cpp
 * @brief N-Way blending engine implementation
 */

#include "NWayBlender.h"
#include "MeshUtils.h"
#include <iostream>
#include <cmath>

// Template helper functions for blending (from original nwayBlender.cpp)

template<typename T>
void blendMatList(const std::vector<std::vector<T>>& A, const std::vector<double>& weight, std::vector<T>& X) {
    int numMesh = (int)A.size();
    if (numMesh == 0) return;
    int numTet = (int)A[0].size();
    for (int i = 0; i < numTet; i++) {
        X[i].setZero();
        for (int j = 0; j < numMesh; j++) {
            X[i] += weight[j] * A[j][i];
        }
    }
}

template<typename T>
void blendMatLinList(const std::vector<std::vector<T>>& A, const std::vector<double>& weight, std::vector<T>& X) {
    int numMesh = (int)A.size();
    if (numMesh == 0) return;
    int numTet = (int)A[0].size();
    for (int i = 0; i < numTet; i++) {
        double sum = 0.0;
        X[i].setZero();
        for (int j = 0; j < numMesh; j++) {
            X[i] += weight[j] * A[j][i];
            sum += weight[j];
        }
        X[i] += (1.0 - sum) * T::Identity();
    }
}

void blendQuatList(const std::vector<std::vector<Vector4d>>& A, const std::vector<double>& weight,
                  std::vector<Vector4d>& X) {
    int numMesh = (int)A.size();
    if (numMesh == 0) return;
    int numTet = (int)A[0].size();
    Vector4d I(0, 0, 0, 1);
    for (int i = 0; i < numTet; i++) {
        double sum = 0.0;
        X[i].setZero();
        for (int j = 0; j < numMesh; j++) {
            X[i] += weight[j] * A[j][i];
            sum += weight[j];
        }
        X[i] += (1.0 - sum) * I;
        X[i] = X[i].normalized();
    }
}

// ========== NWayBlender Implementation ==========

NWayBlender::NWayBlender()
    : numPts(0)
    , blendMode(BM_LOG3)
    , tetMode(TM_FACE)
    , numIterations(1)
    , rotationConsistency(false)
    , areaWeighted(false)
    , initRotationAngle(0.0)
    , needsInitialization(true)
    , needsParametrization(true)
    , numParametrized(0) {
}

NWayBlender::~NWayBlender() {
}

void NWayBlender::setBaseMesh(const Mesh& mesh) {
    baseMesh = mesh;
    pts = baseMesh.getVerticesAsVector3d();
    numPts = (int)pts.size();
    needsInitialization = true;
    needsParametrization = true;
    numParametrized = 0;
}

void NWayBlender::addBlendMesh(const Mesh& mesh) {
    blendMeshes.push_back(mesh);
    needsParametrization = true;
}

void NWayBlender::clearMeshes() {
    baseMesh.clear();
    blendMeshes.clear();
    pts.clear();
    numPts = 0;
    needsInitialization = true;
    needsParametrization = true;
    numParametrized = 0;
}

bool NWayBlender::initialize() {
    if (!baseMesh.isValid()) {
        std::cerr << "NWayBlender::initialize() - No valid base mesh" << std::endl;
        return false;
    }

    std::cout << "NWayBlender: Initializing with " << blendMeshes.size() << " blend meshes..." << std::endl;

    // Build tetrahedral structure from base mesh
    faceList = baseMesh.faceList;
    int dim = MeshUtils::buildTetStructure(tetMode, pts, solver.tetList, faceList,
                                          edgeList, vertexList, solver.tetMatrix, solver.tetWeight);

    // Remove degenerate tetrahedra
    solver.dim = Tetrise::removeDegenerate(tetMode, numPts, solver.tetList, faceList,
                                          edgeList, vertexList, solver.tetMatrix);

    // Recompute tet matrices after cleanup
    Tetrise::makeTetMatrix(tetMode, pts, solver.tetList, faceList, edgeList, vertexList,
                          solver.tetMatrix, solver.tetWeight);

    // Build adjacency list for rotation consistency
    Tetrise::makeAdjacencyList(tetMode, solver.tetList, edgeList, vertexList, adjacencyList);

    solver.numTet = (int)solver.tetList.size() / 4;

    // Compute inverse tet matrices
    solver.computeTetMatrixInverse();

    std::cout << "  Built " << solver.numTet << " tetrahedra, dim=" << solver.dim << std::endl;

    // Setup ARAP solver
    if (!areaWeighted) {
        solver.tetWeight.clear();
        solver.tetWeight.resize(solver.numTet, 1.0);
    }

    // Set soft constraint at first vertex
    solver.constraintWeight.resize(1);
    solver.constraintWeight[0] = std::make_pair(0, 1.0);
    solver.constraintVal.resize(1, 3);
    solver.constraintVal(0, 0) = pts[0][0];
    solver.constraintVal(0, 1) = pts[0][1];
    solver.constraintVal(0, 2) = pts[0][2];

    int error = solver.ARAPprecompute();
    if (error > 0) {
        std::cerr << "NWayBlender::initialize() - ARAP precompute failed" << std::endl;
        return false;
    }

    std::cout << "  ARAP solver initialized" << std::endl;

    needsInitialization = false;
    needsParametrization = true;
    numParametrized = 0;

    return true;
}

void NWayBlender::parametrizeBlendMesh(int meshIndex) {
    if (meshIndex < 0 || meshIndex >= (int)blendMeshes.size()) {
        std::cerr << "Invalid blend mesh index: " << meshIndex << std::endl;
        return;
    }

    const Mesh& blendMesh = blendMeshes[meshIndex];
    std::vector<Vector3d> bpts = blendMesh.getVerticesAsVector3d();

    if ((int)bpts.size() != numPts) {
        std::cerr << "Blend mesh " << meshIndex << " has incompatible vertex count" << std::endl;
        return;
    }

    // Compute tet matrices for blend mesh
    Tetrise::makeTetMatrix(tetMode, bpts, solver.tetList, faceList, edgeList, vertexList, Q, dummy_weight);

    // Compute relative transformation per tet
    logR[meshIndex].resize(solver.numTet);
    logS[meshIndex].resize(solver.numTet);
    R[meshIndex].resize(solver.numTet);
    S[meshIndex].resize(solver.numTet);
    GL[meshIndex].resize(solver.numTet);
    L[meshIndex].resize(solver.numTet);

    for (int i = 0; i < solver.numTet; i++) {
        Matrix4d aff = solver.tetMatrixInverse[i] * Q[i];
        GL[meshIndex][i] = aff.block(0, 0, 3, 3);
        L[meshIndex][i] = transPart(aff);
        parametriseGL(GL[meshIndex][i], logS[meshIndex][i], R[meshIndex][i]);
    }

    // Parametrize based on blend mode
    if (blendMode == BM_LOG3) {
        logGL[meshIndex].resize(solver.numTet);
        for (int i = 0; i < solver.numTet; i++) {
            logGL[meshIndex][i] = GL[meshIndex][i].log().eval();
        }
    } else if (blendMode == BM_SQL) {
        quat[meshIndex].resize(solver.numTet);
        for (int i = 0; i < solver.numTet; i++) {
            S[meshIndex][i] = expSym(logS[meshIndex][i]);
            Quaternion<double> q(R[meshIndex][i].transpose());
            quat[meshIndex][i] << q.x(), q.y(), q.z(), q.w();
        }
    } else if (blendMode == BM_SlRL) {
        for (int i = 0; i < solver.numTet; i++) {
            S[meshIndex][i] = expSym(logS[meshIndex][i]);
        }
    }

    // Compute rotation consistency if enabled
    if (rotationConsistency) {
        computeRotationConsistency(meshIndex);
    } else {
        for (int i = 0; i < solver.numTet; i++) {
            logR[meshIndex][i] = logSO(R[meshIndex][i]);
        }
    }

    std::cout << "  Parametrized blend mesh " << meshIndex << std::endl;
}

void NWayBlender::computeRotationConsistency(int meshIndex) {
    // Use BFS traversal to pick consistent rotation branches
    std::set<int> remain;
    std::queue<int> later;

    // Initialize with rotation angle
    Matrix3d initR;
    double angle = initRotationAngle * M_PI / 180.0;
    initR << 0, angle, 0,
             -angle, 0, 0,
             0, 0, 0;
    std::vector<Matrix3d> prevSO(solver.numTet, initR);

    // Create adjacency graph to traverse
    for (int i = 0; i < solver.numTet; i++) {
        remain.insert(remain.end(), i);
    }

    while (!remain.empty()) {
        int next;
        if (!later.empty()) {
            next = later.front();
            later.pop();
            remain.erase(next);
        } else {
            next = *remain.begin();
            remain.erase(remain.begin());
        }

        logR[meshIndex][next] = logSOc(R[meshIndex][next], prevSO[next]);

        for (size_t k = 0; k < adjacencyList[next].size(); k++) {
            int f = adjacencyList[next][k];
            if (remain.erase(f) > 0) {
                prevSO[f] = logR[meshIndex][next];
                later.push(f);
            }
        }
    }
}

void NWayBlender::blendTransformations(const std::vector<double>& weights,
                                      std::vector<Matrix3d>& AR,
                                      std::vector<Matrix3d>& AS,
                                      std::vector<Vector3d>& AL) {
    // Blend translation
    blendMatList(L, weights, AL);

    if (blendMode == BM_SRL) {
        // Blend log rotations and log symmetric parts
        blendMatList(logR, weights, AR);
        blendMatList(logS, weights, AS);
        #pragma omp parallel for
        for (int i = 0; i < solver.numTet; i++) {
            AR[i] = expSO(AR[i]);
            AS[i] = expSym(AS[i]);
        }
    } else if (blendMode == BM_LOG3) {
        // Blend log matrices
        blendMatList(logGL, weights, AR);
        #pragma omp parallel for
        for (int i = 0; i < solver.numTet; i++) {
            AR[i] = AR[i].exp().eval();
            AS[i] = Matrix3d::Identity();
        }
    } else if (blendMode == BM_SQL) {
        // Blend quaternions and scale
        std::vector<Vector4d> Aq(solver.numTet);
        blendMatLinList(S, weights, AS);
        blendQuatList(quat, weights, Aq);
        #pragma omp parallel for
        for (int i = 0; i < solver.numTet; i++) {
            Quaternion<double> Q(Aq[i]);
            AR[i] = Q.matrix().transpose();
        }
    } else if (blendMode == BM_SlRL) {
        // Blend log rotations and scale linearly
        blendMatList(logR, weights, AR);
        blendMatLinList(S, weights, AS);
        #pragma omp parallel for
        for (int i = 0; i < solver.numTet; i++) {
            AR[i] = expSO(AR[i]);
        }
    } else if (blendMode == BM_AFF) {
        // Linear blending
        blendMatLinList(GL, weights, AR);
        for (int i = 0; i < solver.numTet; i++) {
            AS[i] = Matrix3d::Identity();
        }
    }
}

void NWayBlender::computeEnergy(const std::vector<Vector3d>& newPts,
                               const std::vector<Matrix3d>& AS,
                               std::vector<Matrix3d>& AR,
                               std::vector<double>& tetEnergy) {
    Tetrise::makeTetMatrix(tetMode, newPts, solver.tetList, faceList, edgeList, vertexList, Q, dummy_weight);

    Matrix3d S, Rfit;
    #pragma omp parallel for private(S, Rfit)
    for (int i = 0; i < solver.numTet; i++) {
        polarHigham((solver.tetMatrixInverse[i] * Q[i]).block(0, 0, 3, 3), S, Rfit);
        AR[i] = Rfit;
        tetEnergy[i] = (S - AS[i]).squaredNorm();
    }
}

bool NWayBlender::computeBlend(const std::vector<double>& weights,
                              Mesh& output,
                              bool visualizeEnergy,
                              double visualizationMultiplier) {
    if (needsInitialization) {
        std::cerr << "NWayBlender::computeBlend() - Not initialized" << std::endl;
        return false;
    }

    int numMesh = (int)blendMeshes.size();
    if (numMesh == 0) {
        std::cerr << "NWayBlender::computeBlend() - No blend meshes" << std::endl;
        return false;
    }

    if ((int)weights.size() != numMesh) {
        std::cerr << "NWayBlender::computeBlend() - Weight count mismatch" << std::endl;
        return false;
    }

    // Resize parametrization arrays
    logR.resize(numMesh);
    logS.resize(numMesh);
    R.resize(numMesh);
    S.resize(numMesh);
    GL.resize(numMesh);
    logGL.resize(numMesh);
    quat.resize(numMesh);
    L.resize(numMesh);

    // Parametrize any new or modified blend meshes
    for (int j = numParametrized; j < numMesh; j++) {
        parametrizeBlendMesh(j);
    }
    numParametrized = numMesh;

    // Blend transformations
    std::vector<Matrix3d> AR(solver.numTet);
    std::vector<Matrix3d> AS(solver.numTet);
    std::vector<Vector3d> AL(solver.numTet);

    blendTransformations(weights, AR, AS, AL);

    // Prepare for ARAP iteration
    std::vector<Vector3d> new_pts(numPts);
    std::vector<Matrix4d> A(solver.numTet);
    std::vector<double> tetEnergy(solver.numTet);

    // Iterate to determine vertex positions
    for (int k = 0; k < numIterations; k++) {
        // Compose target matrices
        for (int i = 0; i < solver.numTet; i++) {
            A[i] = pad(AS[i] * AR[i], AL[i]);
        }

        // Solve ARAP
        solver.ARAPSolve(A);

        // Extract new vertex positions
        for (int i = 0; i < numPts; i++) {
            new_pts[i][0] = solver.Sol(i, 0);
            new_pts[i][1] = solver.Sol(i, 1);
            new_pts[i][2] = solver.Sol(i, 2);
        }

        // If iterating or visualizing, recompute rotations
        if (k + 1 < numIterations || visualizeEnergy) {
            computeEnergy(new_pts, AS, AR, tetEnergy);
        }
    }

    // Update output mesh
    output.updateFromVector3d(new_pts);

    // Compute vertex energy for visualization
    if (visualizeEnergy) {
        Tetrise::makePtsWeightList(tetMode, numPts, solver.tetList, faceList, edgeList,
                                   vertexList, tetEnergy, ptsEnergy);

        // Scale energy for visualization
        for (int i = 0; i < numPts; i++) {
            ptsEnergy[i] *= visualizationMultiplier;
        }

        // Store in output mesh
        output.vertexEnergy.resize(numPts);
        for (int i = 0; i < numPts; i++) {
            output.vertexEnergy[i] = ptsEnergy[i];
        }
    }

    return true;
}
