Illumina is a strong chess engine that supports the [UCI protocol](https://en.wikipedia.org/wiki/Universal_Chess_Interface). 

## Building Prerequisites

- C++17 compliant compiler
- CMake 3.20 or higher (build system generator)

## Building

Illumina supports CMake in order to allow cross-platform compiling. 
If you are using an IDE such as Visual Studio or CLion, import Illumina as a CMake project and building should work out of the box.
Note that if building tests is desired, submodules must have been cloned.

## Honorable Mentions

- [Marlinflow](https://github.com/jnlt3/marlinflow): Used to train Illumina's evaluation network.
- [Weather Factory](https://github.com/jnlt3/weather-factory): Used to tune search and time control parameters using SPSA.
- [Engine Programming Discord](https://discord.com/invite/F6W6mMsTGN): Highly skilled and helpful members provided very useful advice whenever I needed.