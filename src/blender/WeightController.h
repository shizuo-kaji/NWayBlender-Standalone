/**
 * @file WeightController.h
 * @brief Weight controller using 2D Mean Value Coordinates
 * @section LICENSE The MIT License
 * @version 1.0
 * @date 2025
 */

#pragma once

#include "Mesh.h"
#include <Eigen/Dense>
#include <vector>

/**
 * @brief Weight controller for automatic weight assignment from control points
 *
 * Uses 2D Mean Value Coordinates to compute smooth weight distributions.
 * Control points are placed in 2D space, and weights are computed for each
 * mesh vertex based on their spatial relationship to control points.
 */
class WeightController {
public:
    WeightController();
    ~WeightController();

    /**
     * @brief Set control points
     * @param points Vector of 3D control point positions
     */
    void setControlPoints(const std::vector<Eigen::Vector3d>& points);

    /**
     * @brief Clear all control points
     */
    void clearControlPoints();

    /**
     * @brief Add a control point
     * @param point 3D position
     */
    void addControlPoint(const Eigen::Vector3d& point);

    /**
     * @brief Remove a control point
     * @param index Control point index
     */
    void removeControlPoint(int index);

    /**
     * @brief Update a control point position
     * @param index Control point index
     * @param point New 3D position
     */
    void updateControlPoint(int index, const Eigen::Vector3d& point);

    /**
     * @brief Compute blend weights for a single query point
     *
     * Uses 2D Mean Value Coordinates (MVC) to compute smooth weights.
     * Projects 3D points to XY plane for 2D computation.
     *
     * @param queryPoint 3D query position
     * @return Vector of weights (one per control point, sum to 1.0)
     */
    std::vector<double> computeWeights(const Eigen::Vector3d& queryPoint);

    /**
     * @brief Compute weights for all mesh vertices
     *
     * For each vertex, computes weights relative to all control points.
     * Result: vertexWeights[i][j] = weight of control point j for vertex i
     *
     * @param meshVertices Vector of mesh vertex positions
     * @param vertexWeights Output: weights per vertex (numVertices x numControlPoints)
     */
    void computeVertexWeights(
        const std::vector<Eigen::Vector3d>& meshVertices,
        std::vector<std::vector<double>>& vertexWeights);

    /**
     * @brief Get number of control points
     */
    int getNumControlPoints() const;

    /**
     * @brief Get control points
     */
    const std::vector<Eigen::Vector3d>& getControlPoints() const;

    /**
     * @brief 2D Mean Value Coordinates algorithm (PUBLIC for GUI access)
     *
     * Ported from weightController.cpp lines 38-83.
     * Computes barycentric coordinates for a point inside a 2D polygon.
     * Also supports extrapolation when point is outside the polygon.
     *
     * @param loc Query point in 2D
     * @param vertices Polygon vertices in 2D (assumed to form a closed loop)
     * @return Weights for each vertex (inside: sum to 1.0, outside: can be negative)
     */
    std::vector<double> computeMVC2D(
        const Eigen::Vector2d& loc,
        const std::vector<Eigen::Vector2d>& vertices);

private:
    std::vector<Eigen::Vector3d> controlPoints;  ///< Control point positions
};
