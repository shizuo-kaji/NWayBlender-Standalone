cmake_minimum_required(VERSION 3.15)
project(NWayBlender-Standalone)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Options
option(USE_OPENMP "Enable OpenMP parallelization" ON)

# Find Eigen3 (accept any version 3.x or higher)
find_package(Eigen3 REQUIRED NO_MODULE)

# libigl
option(LIBIGL_WITH_OPENGL            "Use OpenGL"         ON)
option(LIBIGL_WITH_OPENGL_GLFW       "Use GLFW"           ON)

# Add external dependencies as subdirectories if they exist
# Otherwise, users need to install them separately or add as git submodules
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/libigl/CMakeLists.txt")
    add_subdirectory(external/libigl)
else()
    # Try to find system libigl
    find_package(libigl QUIET)
    if(NOT libigl_FOUND)
        message(WARNING "libigl not found. Please add it to external/libigl or install system-wide.")
    endif()
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/polyscope/CMakeLists.txt")
    add_subdirectory(external/polyscope)
else()
    find_package(polyscope QUIET)
    if(NOT polyscope_FOUND)
        message(WARNING "Polyscope not found. Please add it to external/polyscope or install system-wide.")
    endif()
endif()

# OpenMP
if(USE_OPENMP)
    find_package(OpenMP)
    if(OPENMP_FOUND)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    endif()

    # macOS specific: add homebrew libomp paths
    if(APPLE)
        include_directories("/opt/homebrew/opt/libomp/include")
        link_directories("/opt/homebrew/opt/libomp/lib")
    endif()
endif()

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/mesh
    ${CMAKE_CURRENT_SOURCE_DIR}/src/blender
    ${CMAKE_CURRENT_SOURCE_DIR}/src/app
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui
)

# Source files
set(CORE_SOURCES
    src/core/affinelib.h
    src/core/blendAff.h
    src/core/tetrise.h
    src/core/laplacian.h
    src/core/distance.h
    src/core/deformerConst.h
)

set(MESH_SOURCES
    src/mesh/Mesh.h
    src/mesh/Mesh.cpp
    src/mesh/MeshUtils.h
    src/mesh/MeshUtils.cpp
)

set(BLENDER_SOURCES
    src/blender/NWayBlender.h
    src/blender/NWayBlender.cpp
    src/blender/WeightController.h
    src/blender/WeightController.cpp
)

set(APP_SOURCES
    src/app/Application.h
    src/app/Application.cpp
    src/app/main.cpp
)

set(UI_SOURCES
    src/ui/UIManager.h
    src/ui/UIManager.cpp
)

# Executable
add_executable(nway_blender
    ${CORE_SOURCES}
    ${MESH_SOURCES}
    ${BLENDER_SOURCES}
    ${APP_SOURCES}
    ${UI_SOURCES}
)

# Link libraries
target_link_libraries(nway_blender
    Eigen3::Eigen
)

# Conditionally link libigl and polyscope if found
if(TARGET igl::core)
    target_link_libraries(nway_blender igl::core)
endif()

if(TARGET polyscope)
    target_link_libraries(nway_blender polyscope)
endif()

# OpenMP linking
if(USE_OPENMP AND OPENMP_FOUND)
    if(APPLE)
        # On macOS, explicitly link libomp
        target_link_libraries(nway_blender omp)
    endif()
endif()

# Platform-specific settings
if(APPLE)
    target_compile_definitions(nway_blender PRIVATE EIGEN_DONT_ALIGN_STATICALLY)
endif()

if(MSVC)
    target_compile_options(nway_blender PRIVATE /W4)
else()
    target_compile_options(nway_blender PRIVATE -Wall -Wextra)
endif()
