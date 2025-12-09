/**
 * @file WeightController.cpp
 * @brief Weight controller implementation using Mean Value Coordinates
 * @section LICENSE The MIT License
 * @version 1.0
 * @date 2025
 */

#include "WeightController.h"
#include <iostream>
#include <cmath>

#ifndef EPSILON
#define EPSILON 1e-10
#endif

WeightController::WeightController() {
}

WeightController::~WeightController() {
}

void WeightController::setControlPoints(const std::vector<Eigen::Vector3d>& points) {
    controlPoints = points;
}

void WeightController::clearControlPoints() {
    controlPoints.clear();
}

void WeightController::addControlPoint(const Eigen::Vector3d& point) {
    controlPoints.push_back(point);
}

void WeightController::removeControlPoint(int index) {
    if (index >= 0 && index < (int)controlPoints.size()) {
        controlPoints.erase(controlPoints.begin() + index);
    }
}

void WeightController::updateControlPoint(int index, const Eigen::Vector3d& point) {
    if (index >= 0 && index < (int)controlPoints.size()) {
        controlPoints[index] = point;
    }
}

std::vector<double> WeightController::computeWeights(const Eigen::Vector3d& queryPoint) {
    int num = (int)controlPoints.size();
    std::vector<double> weights(num, 0.0);

    if (num == 0) {
        return weights;
    }

    if (num == 1) {
        weights[0] = 1.0;
        return weights;
    }

    // For 2D MVC, we need to project control points to 2D plane
    // Use XY plane for simplicity (can be extended to arbitrary plane)
    std::vector<Eigen::Vector2d> controlPoints2D(num);
    Eigen::Vector2d queryPoint2D(queryPoint[0], queryPoint[1]);

    for (int i = 0; i < num; i++) {
        controlPoints2D[i] = Eigen::Vector2d(controlPoints[i][0], controlPoints[i][1]);
    }

    // Compute 2D Mean Value Coordinates
    weights = computeMVC2D(queryPoint2D, controlPoints2D);

    return weights;
}

std::vector<double> WeightController::computeMVC2D(
    const Eigen::Vector2d& loc,
    const std::vector<Eigen::Vector2d>& vertices) {

    int num = (int)vertices.size();
    std::vector<double> weights(num, 0.0);

    if (num == 0) return weights;
    if (num == 1) {
        weights[0] = 1.0;
        return weights;
    }

    // Algorithm from weightController.cpp lines 38-83
    // Compute vectors from query point to vertices
    std::vector<Eigen::Vector2d> v(num);
    std::vector<double> r(num);

    for (int i = 0; i < num; i++) {
        v[i] = vertices[i] - loc;
        r[i] = v[i].norm();
    }

    // Compute cross products (2D: z-component) and dot products
    std::vector<double> A(num), D(num);
    for (int i = 0; i < num; i++) {
        int j = (i + 1) % num;
        // 2D cross product: v[i].x * v[j].y - v[i].y * v[j].x
        A[i] = std::abs(v[i][0] * v[j][1] - v[i][1] * v[j][0]);
        D[i] = v[i].dot(v[j]);
    }

    bool flag = true;

    // Check if query point is on the boundary
    for (int i = 0; i < num; i++) {
        if (r[i] < EPSILON) {
            // At a vertex
            weights[i] = 1.0;
            flag = false;
            break;
        } else if (std::abs(A[i]) < EPSILON && D[i] < 0) {
            // On an edge
            int j = (i + 1) % num;
            weights[i] = r[j];
            weights[j] = r[i];
            flag = false;
            break;
        }
    }

    // If not on boundary, compute Mean Value Coordinates
    if (flag) {
        for (int i = 0; i < num; i++) {
            int k = (i - 1 + num) % num;
            if (std::abs(A[k]) > EPSILON) {
                weights[i] += (r[k] - D[k] / r[i]) / A[k];
            }
            if (std::abs(A[i]) > EPSILON) {
                weights[i] += (r[(i + 1) % num] - D[i] / r[i]) / A[i];
            }
        }
    }

    // Normalize weights
    double sum = 0.0;
    for (int i = 0; i < num; i++) {
        sum += weights[i];
    }

    if (sum > EPSILON) {
        for (int i = 0; i < num; i++) {
            weights[i] /= sum;
        }
    }

    return weights;
}

void WeightController::computeVertexWeights(
    const std::vector<Eigen::Vector3d>& meshVertices,
    std::vector<std::vector<double>>& vertexWeights) {

    int numVertices = (int)meshVertices.size();
    int numControlPoints = (int)controlPoints.size();

    vertexWeights.resize(numVertices);

    if (numControlPoints == 0) {
        // No control points, assign equal weights
        for (int i = 0; i < numVertices; i++) {
            vertexWeights[i].resize(numControlPoints, 0.0);
        }
        return;
    }

    // Compute weights for each vertex based on control points
    for (int i = 0; i < numVertices; i++) {
        vertexWeights[i] = computeWeights(meshVertices[i]);
    }
}

int WeightController::getNumControlPoints() const {
    return (int)controlPoints.size();
}

const std::vector<Eigen::Vector3d>& WeightController::getControlPoints() const {
    return controlPoints;
}
