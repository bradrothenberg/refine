# C# example code

Examples of interacting with the nTop Core library fromm C#.

Note: At present these do not function with \*nix environments.

### Setup

These examples require [.NET 7](https://dotnet.microsoft.com/en-us/download/dotnet/7.0) on the host system.

If you are working from a Unix-like environment you will need to modify the variable `_dllLocation` in [CoreWrapper/ImplicitBody.cs](CoreWrapper/ImplicitBody.cs#l15) to point at the location of the shared object file.

### Mesher

This is a simple mesh generation implementation using marching cubes which highlights use of the `ntop_core_query_field` and `ntop_core_query_bounding_box`.

To run it from the `examples/dotnet` directory:

```shell
dotnet run --project Mesher/Mesher.csproj
```

It will output the location of the STL which you can load in the viewer of your choice.

Note that the implementation may not produce closed or non-self-intersecting meshes.
This is just to highlight how you may integrate nTop Core into your workflow.

For a more production-ready implementation consider using [OpenVDB](https://www.openvdb.org/).

### Slicer

This produces images of the contours of a slice of the median axis-aligned plane of the implicit body.
This highlights use of `ntop_core_query_contours`.

To run it from the `examples/dotnet` directory:

```shell
dotnet run --project Slicer/Slicer.csproj
```

This will output `slice.png` in this directory.

### Volume

This calculates the volume of an implicit body using a volumetric calculation.

To run it from the `examples/dotnet` directory:

```shell
dotnet run --project Volume/Volume.csproj
```
