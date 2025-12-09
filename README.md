# N-Way Blender - Standalone Application

Standalone C++ application for N-way mesh blending using libigl and Polyscope. Ported from the Maya plugin to be independent of proprietary 3D software.

## Features

- Load multiple meshes (OBJ, PLY formats)
- N-way shape interpolation with multiple blending modes
- ARAP (As-Rigid-As-Possible) deformation
- Weight controller using barycentric coordinates
- Energy visualization
- Real-time preview
- Export blended meshes

## Requirements

### Dependencies

- **CMake** 3.15 or higher
- **C++14** compatible compiler
- **Eigen3** 3.3 or higher
- **libigl** (included as submodule or installed separately)
- **Polyscope** (included as submodule or installed separately)
- **OpenMP** (optional, for parallelization)

### Platform Support

- macOS (tested)
- Linux (should work)
- Windows (should work with MSVC)

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

## Implementation Status

### Phase 1: Foundation ✅ COMPLETE

- [x] Project structure
- [x] CMake build system
- [x] Core algorithm headers (affinelib, blendAff, tetrise, laplacian, distance)
- [x] Mesh data structure with libigl I/O
- [x] Application state management
- [x] Basic Polyscope visualization
- [x] Mesh loading from command line

### Phase 2: Core Blending ✅ COMPLETE

- [x] NWayBlender engine
  - [x] Tetrahedral structure initialization
  - [x] Blend mesh parametrization
  - [x] Rotation consistency
  - [x] Matrix blending computation
- [x] ARAP integration
  - [x] Solver setup
  - [x] Iterative solving
- [x] Blend modes
  - [x] BM_SRL (Shear-Rotation-Linear)
  - [x] BM_LOG3 (3D matrix logarithm)
  - [x] BM_LOG4 (4D matrix logarithm)
  - [x] BM_SQL (Quaternion-based)
  - [x] BM_SSE (Special Similarity Euclidean)
  - [x] BM_SlRL (Linear scale + rotation)
  - [x] BM_AFF (Direct affine)
- [x] Energy computation
- [x] Interactive UI with weight sliders
- [x] Real-time blending

### Phase 3: UI (TODO)

- [ ] File dialogs for mesh loading
- [ ] Dynamic weight sliders
- [ ] Blend mode dropdown
- [ ] Tet mode dropdown
- [ ] Parameter controls
- [ ] Mesh visibility toggles
- [ ] Energy visualization
- [ ] Export functionality

### Phase 4: Weight Controller (TODO)

- [ ] 2D Mean Value Coordinates
- [ ] Control point placement
- [ ] Control point visualization
- [ ] Automatic weight computation

### Phase 5: Polish (TODO)

- [ ] Performance optimization
- [ ] Bug fixes
- [ ] Documentation
- [ ] Example meshes

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
- Uses Eigen, libigl, and Polyscope libraries

## Troubleshooting

### Build Errors

**"Eigen3 not found"**
```bash
# macOS
brew install eigen

# Specify location manually
cmake -DEigen3_DIR=/usr/local/share/eigen3/cmake ..
```

**"libigl not found"**
```bash
cd external
git clone https://github.com/libigl/libigl.git
cd ..
mkdir build && cd build
cmake ..
```

### Runtime Errors

**"ARAP precompute failed"**
- Mesh may have degenerate faces or zero-length edges
- Try "Cleanup" mesh in another tool before loading
- Reduce iteration count or disable ARAP

**"Mesh topology doesn't match"**
- All blend meshes must have identical vertex/face counts as base mesh
- Ensure meshes are in correspondence

## TODO for Future Versions

- [ ] GPU acceleration for ARAP solver
- [ ] Keyframe animation support
- [ ] Mesh editing within the application
- [ ] Python bindings
- [ ] Web-based version using WebAssembly

## Development

### Running Tests

```bash
cd build
ctest
```

### Code Style

- C++14
- 4-space indentation
- Follow existing code conventions

### Contributing

Contributions welcome! Please:
1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Submit a pull request

## Contact

For questions or issues, please open a GitHub issue.
