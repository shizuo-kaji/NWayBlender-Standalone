# N-Way Blender (WIP)

Standalone C++ application for N-way mesh blending using libigl and Polyscope.
Ported from the [Maya plugin](https://github.com/shizuo-kaji/NWayBlenderMaya) to be independent of proprietary 3D software.

### Dependencies

- **CMake** 3.15 or higher
- **C++14** compatible compiler
- **Eigen3** 3.3 or higher
- **libigl** (included as submodule or installed separately)
- **Polyscope** (included as submodule or installed separately)
- **OpenMP** (optional, for parallelization)

## Installation

### 1. Clone the Repository

```bash
cd /path/to/NWayBlender-Standalone
```

### 2. Get Dependencies

#### Option A: Using Git Submodules (Recommended)

```bash
cd external

# Clone libigl
git clone https://github.com/libigl/libigl.git

# Clone Polyscope
git clone https://github.com/nmwsharp/polyscope.git

cd ..
```

#### Option B: System-Wide Installation

Install Eigen3:
```bash
# macOS
brew install eigen

# Ubuntu/Debian
sudo apt-get install libeigen3-dev

# Or download from http://eigen.tuxfamily.org/
```

Install libigl and Polyscope following their respective documentation.

### 3. Build

```bash
mkdir build
cd build
cmake ..
make -j4
```

#### Build Options

```bash
# Disable OpenMP
cmake -DUSE_OPENMP=OFF ..

# Specify Eigen location
cmake -DEigen3_DIR=/path/to/eigen3 ..
```

## Usage

### Basic Usage

```bash
./nway_blender base_mesh.obj
```

### Loading Meshes

1. Run the application
2. Use "Load Base Mesh" to load the reference mesh
3. Use "Add Blend Mesh" to add target shapes
4. Adjust weight sliders to blend between shapes

### Command Line

```bash
# Load base mesh and blend targets
./nway_blender base.obj blend1.obj blend2.obj

# Example: Blend between 3 shape variations
./nway_blender sphere.obj sphere_stretched.obj sphere_squashed.obj

# Load just base mesh (no blending yet)
./nway_blender base.obj

# No arguments starts with empty scene
./nway_blender
```

### Quick Test

After building, try this:

```bash
# 1. Create or find 2-3 meshes with identical topology
# 2. Run with multiple meshes:
./nway_blender mesh1.obj mesh2.obj mesh3.obj

# 3. In the UI:
#    - Adjust "Weight 0" and "Weight 1" sliders
#    - Click "Compute Blend"
#    - Try different blend modes
#    - Enable "Visualize Energy"
```

### Controls

**Weight Sliders**: Control influence of each blend mesh (0.0 = no influence, 1.0 = full)

**Blend Mode**: Choose blending algorithm
- **LOG3**: Best for general shape blending (recommended)
- **SQL**: Good for rotation-heavy deformations
- **SRL**: Most accurate, preserves local rotations

**Iterations**: Number of ARAP refinement iterations (1-10)
- 1-2: Fast, good quality
- 3-5: Better detail preservation
- 10: Maximum quality, slower

**Rotation Consistency**: Ensures smooth rotation fields (slower but better quality)

**Visualize Energy**: Show deformation energy as vertex colors (red = high energy)

## Architecture

```
NWayBlender-Standalone/
├── src/
│   ├── core/          # Maya-independent algorithms (kept from original)
│   │   ├── affinelib.h          # Affine transformations
│   │   ├── blendAff.h           # Blending parametrization
│   │   ├── tetrise.h            # Tetrahedralization
│   │   ├── laplacian.h          # ARAP solver
│   │   ├── distance.h           # Weight computation
│   │   └── deformerConst.h      # Constants
│   ├── mesh/          # Mesh data structures
│   │   ├── Mesh.h/.cpp          # Mesh class
│   │   └── MeshUtils.h/.cpp     # Utility functions
│   ├── blender/       # Blending engine
│   │   ├── NWayBlender.h/.cpp   # Main blending logic
│   │   └── WeightController.h/.cpp # Weight computation
│   ├── app/           # Application layer
│   │   ├── Application.h/.cpp   # State management
│   │   └── main.cpp             # Entry point
│   └── ui/            # User interface
│       └── UIManager.h/.cpp     # Polyscope/ImGui UI
└── external/          # External dependencies
    ├── eigen/
    ├── libigl/
    └── polyscope/
```

## Blend Modes

- **BM_SRL**: Shear-Rotation-Linear decomposition
- **BM_LOG3**: 3×3 matrix logarithm
- **BM_LOG4**: 4×4 affine matrix logarithm
- **BM_SQL**: Quaternion-based rotation + scale
- **BM_SSE**: Special Similarity Euclidean group
- **BM_AFF**: Direct affine blending

## Tet Modes

- **TM_FACE**: Face-based tetrahedralization
- **TM_EDGE**: Edge-based tetrahedralization
- **TM_VERTEX**: Vertex-based tetrahedralization
- **TM_VFACE**: Vertex-face combined

## References

- S. Kaji, "Tetrisation of triangular meshes and its application in shape blending", Mathematical Progress in Expressive Image Synthesis III, pp. 7-19, Springer-Japan, 2016. DOI: 10.1007/978-981-10-1076-7
- http://arxiv.org/abs/1601.04816

## License

MIT License (same as original Maya plugin)

## Credits

- Original Maya plugin by Shizuo KAJI
- Standalone port: 2025

### Runtime Errors

**"ARAP precompute failed"**
- Mesh may have degenerate faces or zero-length edges
- Try "Cleanup" mesh in another tool before loading
- Reduce iteration count or disable ARAP

**"Mesh topology doesn't match"**
- All blend meshes must have identical vertex/face counts as base mesh
- Ensure meshes are in correspondence
