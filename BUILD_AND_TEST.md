# Build and Test Instructions

## Quick Start

### 1. Install Dependencies

#### macOS:
```bash
# Install Homebrew if not already installed
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install Eigen
brew install eigen

# Install CMake
brew install cmake
```

#### Ubuntu/Debian:
```bash
sudo apt-get update
sudo apt-get install cmake libeigen3-dev
```

### 2. Get External Libraries

```bash
cd /Users/kaji/Library/CloudStorage/Dropbox/maple/mayac/NWayBlender-Standalone/external

# Clone libigl
git clone https://github.com/libigl/libigl.git

# Clone Polyscope
git clone https://github.com/nmwsharp/polyscope.git

cd ..
```

### 3. Build

```bash
# Create build directory
mkdir -p build
cd build

# Configure
cmake ..

# Build (use -j4 for parallel build with 4 cores)
make -j4

# If successful, you should see:
# [100%] Built target nway_blender
```

### 4. Test with Example Meshes

If you have the original Maya plugin examples:

```bash
# From the build directory
./nway_blender ../../NWayBlender/examples/sphere_base.obj \
               ../../NWayBlender/examples/sphere_blend1.obj \
               ../../NWayBlender/examples/sphere_blend2.obj
```

Or create simple test meshes in Blender:

**In Blender:**
1. Create a UV Sphere (Add → Mesh → UV Sphere)
2. Export as sphere_base.obj
3. Deform the sphere (Scale, Move vertices, etc.)
4. Export as sphere_blend1.obj
5. Deform differently
6. Export as sphere_blend2.obj

**Run:**
```bash
./nway_blender sphere_base.obj sphere_blend1.obj sphere_blend2.obj
```

### 5. Use the Application

When the window opens:

1. **Check Status Panel**:
   - Should show base mesh name and stats
   - Should show number of blend meshes

2. **Open Blending Panel**:
   - You'll see weight sliders (one per blend mesh)
   - Adjust "Weight 0" slider (0.0 to 1.0)
   - Click "Compute Blend" button

3. **Experiment**:
   - Try different blend modes from dropdown
   - Adjust iterations (1-10)
   - Enable "Visualize Energy" to see deformation quality
   - Enable "Rotation Consistency" for smoother results

## Troubleshooting

### Build Issues

**Error: "Eigen3 not found"**
```bash
# macOS:
brew install eigen

# Ubuntu:
sudo apt-get install libeigen3-dev

# Or manually specify:
cmake -DEigen3_DIR=/path/to/eigen3/cmake ..
```

**Error: "libigl not found"**
```bash
cd external
git clone https://github.com/libigl/libigl.git
cd ../build
cmake ..
make
```

**Error: "polyscope not found"**
```bash
cd external
git clone https://github.com/nmwsharp/polyscope.git
cd ../build
cmake ..
make
```

### Runtime Issues

**"Cannot initialize: need base mesh and at least one blend mesh"**
- You need to provide at least 2 meshes (base + 1 blend)
- Usage: `./nway_blender base.obj blend1.obj`

**"Mesh topology doesn't match"**
- All meshes must have exactly the same number of vertices and faces
- They must have the same connectivity (vertex order matters!)
- Export all meshes from the same base mesh

**"ARAP precompute failed"**
- Mesh has degenerate geometry (zero-area faces, zero-length edges)
- Clean up mesh in Blender or MeshLab:
  - Blender: Mesh → Clean Up → Delete Loose, Merge by Distance
  - MeshLab: Filters → Cleaning and Repairing → Remove Duplicate Vertices

**Segmentation Fault**
- Check that you have all dependencies installed
- Verify mesh files are valid OBJ format
- Try with simpler/smaller meshes first

**Slow Performance**
- Reduce iteration count to 1-2
- Use simpler blend mode (LOG3 instead of SRL)
- Try smaller meshes first (<10K vertices)

## Verification Tests

Run these tests to verify everything works:

### Test 1: Basic Blending
```bash
# With 2 meshes
./nway_blender mesh1.obj mesh2.obj

# Set Weight 0 = 0.0 → Should see base mesh
# Set Weight 0 = 1.0 → Should see blend mesh 1
# Set Weight 0 = 0.5 → Should see halfway blend
```

### Test 2: Multiple Blend Targets
```bash
# With 3 meshes
./nway_blender mesh1.obj mesh2.obj mesh3.obj

# Adjust Weight 0 and Weight 1 independently
# Should see smooth interpolation
```

### Test 3: Blend Modes
```bash
./nway_blender mesh1.obj mesh2.obj

# Try each blend mode:
# - LOG3 (default, recommended)
# - SQL (quaternion-based)
# - SRL (most accurate)
# - SlRL, SSE, AFF
# Each should produce slightly different results
```

### Test 4: ARAP Iterations
```bash
./nway_blender mesh1.obj mesh2.obj

# Set iterations = 1 (fast)
# Set iterations = 5 (better quality)
# Set iterations = 10 (best quality, slower)
# Should see refinement in local details
```

### Test 5: Energy Visualization
```bash
./nway_blender mesh1.obj mesh2.obj

# Enable "Visualize Energy" checkbox
# Compute blend
# Mesh should show colors (white=low, red=high energy)
# High energy areas indicate where ARAP has difficulty
```

## Expected Console Output

Successful run should show:

```
N-Way Blender - Standalone Application
=======================================
Loading base mesh from: sphere_base.obj
Loaded mesh 'sphere_base.obj': 482 vertices, 960 faces
Base mesh loaded successfully
Loading blend mesh from: sphere_blend1.obj
Loaded mesh 'sphere_blend1.obj': 482 vertices, 960 faces
Blend mesh 0 added: sphere_blend1.obj
Initializing blending engine...
NWayBlender: Initializing with 1 blend meshes...
  Built 1922 tetrahedra, dim=484
  ARAP solver initialized
  Parametrized blend mesh 0
Initialization complete

Ready to blend! Use the UI to adjust weights.
```

## Performance Benchmarks

Expected performance on modern hardware:

| Vertices | Faces | Initialization | Blend Time | FPS |
|----------|-------|----------------|------------|-----|
| 500      | 1K    | <1s           | <10ms      | 60+ |
| 5K       | 10K   | 1-2s          | 50-100ms   | 20-30 |
| 20K      | 40K   | 5-10s         | 200-500ms  | 5-10 |
| 50K+     | 100K+ | 20s+          | 1s+        | 1-5 |

With OpenMP enabled, performance scales with CPU cores.

## Next Steps

Once basic blending works:

1. **Try rotation consistency**: Enable checkbox, see smoother results
2. **Experiment with blend modes**: Each has different characteristics
3. **Test with real meshes**: Character faces, animated meshes, etc.
4. **Visualize energy**: Understand where deformations are challenging
5. **Export results**: Use "Export Output" (Phase 3) to save blended meshes

## Getting Help

If you're stuck:

1. Check console output for detailed error messages
2. Verify mesh files load correctly in Blender/MeshLab first
3. Start with very simple meshes (sphere with <1K vertices)
4. Try LOG3 blend mode with 1 iteration first
5. Check that all meshes have identical topology

## Success!

If you can:
- ✅ Load meshes from command line
- ✅ See meshes in Polyscope viewer
- ✅ Adjust weight sliders
- ✅ Click "Compute Blend" and see result update
- ✅ Try different blend modes

**Congratulations! Your N-Way Blender is working!** 🎉

The core blending engine is fully functional. Phases 3-5 will add advanced features like file dialogs, weight controllers, and optimizations.
