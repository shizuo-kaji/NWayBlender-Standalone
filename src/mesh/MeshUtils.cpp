/**
 * @file MeshUtils.cpp
 * @brief Mesh utility functions implementation
 */

#include "MeshUtils.h"
#include <algorithm>
#include <cmath>

namespace MeshUtils {

int buildTetStructure(short tetMode,
                     const std::vector<Vector3d>& pts,
                     std::vector<int>& tetList,
                     std::vector<int>& faceList,
                     std::vector<edge>& edgeList,
                     std::vector<vertex>& vertexList,
                     std::vector<Matrix4d>& tetMatrix,
                     std::vector<double>& tetWeight) {

    int numPts = (int)pts.size();

    // Build edge list from face list
    Tetrise::makeEdgeList(faceList, edgeList);

    // Build tetrahedral list based on mode
    int dim = Tetrise::makeTetList(tetMode, numPts, faceList, edgeList, vertexList, tetList);

    // Compute tet matrices and weights
    Tetrise::makeTetMatrix(tetMode, pts, tetList, faceList, edgeList, vertexList, tetMatrix, tetWeight);

    return dim;
}

void computeVertexColors(const std::vector<double>& vertexValues,
                        Eigen::MatrixXd& colors) {
    computeVertexColors(vertexValues, 1.0, colors);
}

void computeVertexColors(const std::vector<double>& vertexValues,
                        double multiplier,
                        Eigen::MatrixXd& colors) {
    int n = (int)vertexValues.size();
    colors.resize(n, 3);

    for (int i = 0; i < n; i++) {
        double value = vertexValues[i] * multiplier;

        // Clamp value to [0, 1]
        value = std::max(0.0, std::min(1.0, value));

        // Convert value to HSV color (H = 0, S = value, V = 1)
        // This creates a grayscale where 0=white, 1=red
        double h = 0.0;     // Hue (red)
        double s = value;   // Saturation
        double v = 1.0;     // Value

        // HSV to RGB conversion
        double c = v * s;
        double x = c * (1.0 - std::abs(std::fmod(h / 60.0, 2.0) - 1.0));
        double m = v - c;

        double r, g, b;
        if (h < 60.0) {
            r = c; g = x; b = 0;
        } else if (h < 120.0) {
            r = x; g = c; b = 0;
        } else if (h < 180.0) {
            r = 0; g = c; b = x;
        } else if (h < 240.0) {
            r = 0; g = x; b = c;
        } else if (h < 300.0) {
            r = x; g = 0; b = c;
        } else {
            r = c; g = 0; b = x;
        }

        colors(i, 0) = r + m;
        colors(i, 1) = g + m;
        colors(i, 2) = b + m;
    }
}

} // namespace MeshUtils
