# Mesh Utils Module

This is a c++ engine module for the Godot engine, containing my mesh merging utilities.

## Godot Version Support

This branch tries to follow godot's master branch (as much as I have time).

For different godot versions look at the other branches.

Status for this branch: Update for 4.0 is work in progress.

# Pre-built binaries

You can grab a pre-built editor binary from the [Broken Seals](https://github.com/Relintai/broken_seals/releases) 
repo, should you want to. It contains all my modules.

# Optional Dependencies

[Mesh Data Resource](https://github.com/Relintai/mesh_data_resource): Support for merged meshes, even in gles2. 
Adds MeshMerger a few helper methods.

# MeshUtils Singleton

Contains generic algorithms that manipulate meshes.

# Mesh Merger

Works similarly to SurfaceTool, but it has more utility functions.

# Fast Quadratic Mesh Simplifier

A port of https://github.com/Whinarn/UnityMeshSimplifier .
For future reference it's based on e8ff4e8862735197c3308cfe926eeba68e0d2edb.
Porting is mostly done, but it does needs some debugging (it has a crash if smart linking is enabled).

I might just return to using the original FQMS. As if meshes are merged together using `MeshUtils.merge_mesh_array`, or
`bake_mesh_array_uv` the original algortithm will work fine. Still on the fence about it.

# Building

1. Get the source code for the engine.

If you want Godot 3.2:
```git clone -b 3.2 https://github.com/godotengine/godot.git godot```

If you want Godot 4.0:
```git clone https://github.com/godotengine/godot.git godot```


2. Go into Godot's modules directory.

```
cd ./godot/modules/
```

3. Clone this repository

```
git clone https://github.com/Relintai/mesh_utils mesh_utils
```

4. Build Godot. [Tutorial](https://docs.godotengine.org/en/latest/development/compiling/index.html)


