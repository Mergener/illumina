Illumina is a chess engine that supports the [UCI protocol](https://en.wikipedia.org/wiki/Universal_Chess_Interface). 

## Building Prerequisites

- C++17 compliant compiler
- CMake 3.20 or higher (build system generator)

## Building

Illumina supports CMake in order to allow cross-platform compiling. 
If you are using an IDE such as Visual Studio or CLion, import Illumina as a CMake project and building should work out of the box.
Note that if building tests is desired, submodules must have been cloned. This can be done by cloning this repository recursively using
the `--recursive` flag after the clone command.

If you are generating the CMake build system by yourself through the command line, make sure to select the desired 
build configuration (`Debug`, `Release`, `RelWithDebInfo`, `MinSizeRel`) by passing `-DCMAKE_BUILD_TYPE=<YourDesiredBuildType>` to 
the CMake invocation.

Besides the build types CMake offers, Illumina also offers the following CMake options (that can be specified by `-D<OPTION_NAME>=<ON/OFF>`):

- `TUNING`: Specifies that we're creating a tuning build. Tuning build exposes many internal constants as UCI options, such as search and time management constants. `OFF` by default.
- `DEVELOPMENT`: Specifies that we're creating a development build. This is mainly for displaying the correct version name when the engine starts and/or `uci` is called. `ON` by default.
- `INCLUDE_TRACING_MODULE`: Includes Illumina's tracing module, that allows `go trace` commands. 
This option requires the SQLiteCPP submodule to be cloned and may considerably increase executable size.
`OFF` by default.
## Honorable Mentions

- [Marlinflow](https://github.com/jnlt3/marlinflow): Used to train Illumina's evaluation network.
- [Weather Factory](https://github.com/jnlt3/weather-factory): Used to tune search and time control parameters using SPSA.
- [Engine Programming Discord](https://discord.com/invite/F6W6mMsTGN): Highly skilled and helpful members provided very useful advice whenever I needed.
- [OpenBench](https://github.com/AndyGrant/OpenBench): For versions after 2.0, used perform Illumina's tests, tuning sessions and data generation.
