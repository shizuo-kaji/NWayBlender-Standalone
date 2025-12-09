#!/usr/bin/env python3
"""
Convert OBJ files with quads and texture/normal indices to simple triangle meshes.
Strips texture coordinates and normals, triangulates quad faces.
"""

import sys
import os

def convert_obj_to_triangles(input_path, output_path):
    """
    Convert OBJ file to simple triangle format.
    - Only keeps vertices and faces
    - Triangulates quad faces (v1,v2,v3,v4) -> (v1,v2,v3) + (v1,v3,v4)
    - Removes texture coordinates and normals from face definitions
    """
    vertices = []
    faces = []

    with open(input_path, 'r') as f:
        for line in f:
            line = line.strip()

            # Parse vertices
            if line.startswith('v '):
                parts = line.split()
                if len(parts) == 4:  # v x y z
                    vertices.append(line)

            # Parse faces
            elif line.startswith('f '):
                parts = line.split()[1:]  # Skip 'f' prefix

                # Extract only vertex indices (strip /vt/vn)
                vertex_indices = []
                for part in parts:
                    # Handle format v/vt/vn or v/vt or v
                    vertex_idx = part.split('/')[0]
                    vertex_indices.append(vertex_idx)

                # Triangulate if quad
                if len(vertex_indices) == 4:
                    # Quad -> 2 triangles
                    # (v1, v2, v3, v4) -> (v1, v2, v3) + (v1, v3, v4)
                    v1, v2, v3, v4 = vertex_indices
                    faces.append(f"f {v1} {v2} {v3}")
                    faces.append(f"f {v1} {v3} {v4}")
                elif len(vertex_indices) == 3:
                    # Already triangle
                    v1, v2, v3 = vertex_indices
                    faces.append(f"f {v1} {v2} {v3}")
                else:
                    print(f"Warning: Skipping face with {len(vertex_indices)} vertices")

    # Write output
    with open(output_path, 'w') as f:
        f.write("# Converted to triangle mesh (no texture/normals)\n")
        for v in vertices:
            f.write(v + '\n')
        for face in faces:
            f.write(face + '\n')

    print(f"Converted: {input_path} -> {output_path}")
    print(f"  Vertices: {len(vertices)}")
    print(f"  Faces: {len(faces)}")

if __name__ == "__main__":
    # Convert p2.obj, p3.obj, p4.obj
    script_dir = os.path.dirname(os.path.abspath(__file__))

    for filename in ['p2.obj', 'p3.obj', 'p4.obj']:
        input_path = os.path.join(script_dir, filename)
        output_path = os.path.join(script_dir, f"{filename[:-4]}_tri.obj")

        if os.path.exists(input_path):
            convert_obj_to_triangles(input_path, output_path)
        else:
            print(f"Warning: {input_path} not found")
