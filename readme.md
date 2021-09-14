# Realtime Ray Tracing with Spacial & Temporal Filtering

This is an example implementation of a basic ray tracer supporting indirect lighting and diffuse albedo textures. 
The noisy raytraced data is filtered using a variety of techniques:
* temporal accumulation, making use of temporal reprojection
* [SVGF](https://research.nvidia.com/publication/2017-07_Spatiotemporal-Variance-Guided-Filtering%3A) using Edge-Avoiding and Edge-Blocking A-Trous

This repository is the result of a semester worth of work from a student project as part of the master's degree "Game Engineering and Visual Computing". The goal was to explore methods of filtering noisy output data from realtime raytracing.
The project was headed by Prof. Dr. Bernd Dreier at the [University of Applied Sciences Kempten](https://hs-kempten.de) in Bavaria, Germany.

## Contributors
* Maurice Ach
* Tobi Eder
* Joseph Heetel
* Marvin Potempa
* Ahmed Salame
* Bernhard Strobl
* Max Thiel

## Basis
The project structure was based on [Sascha Willems' Vulkan Examples](https://github.com/SaschaWillems/Vulkan), more specifically the base library. We use its functions to manage Vulkan Devices, Instance and GLTF Model loading, and other things.