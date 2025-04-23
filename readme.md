<p align="center">
  <img src="logo.png" alt="Illumina's Logo" width="150"/>
</p>

<h1>
    <p align="center">
        Illumina
    </p>
</h1>

Illumina is a free and open-source chess engine powered by a neural network trained on hundreds of millions of positions from self-play games.

Illumina consists of a command line application that complies to the [Universal Chess Interface (UCI) protocol](https://en.wikipedia.org/wiki/Universal_Chess_Interface). This means that, by itself, it does not provide a graphical user interface (GUI) to play or analyze chess games. For that, you can
use any Chess GUI that supports UCI engines (most modern GUIs do) to play against Illumina or analyze games with it, such as CuteChess, SCID or ChessBase. 

## How it works

The user (or in most cases, a host program such as a GUI) sends a [FEN string](https://en.wikipedia.org/wiki/Forsyth%E2%80%93Edwards_Notation) to Illumina indicating the position to evaluate. Then, another command requests the engine to start searching for a best move in the position.

When requested to search a position, Illumina will start searching for the best move using a combination of the [Alpha-Beta pruning algorithm](https://en.wikipedia.org/wiki/Alpha%E2%80%93beta_pruning) to walk through the search tree of possible moves and an [Efficiently Updatable Neural Network (NNUE)](https://en.wikipedia.org/wiki/Efficiently_updatable_neural_network) to perform static evaluation on the positions it finds.

The user can interrupt the search when it finds reasonable. The search might've also been set to run up to a pre-determined amount of time, in which case it will stop automatically when the time is up. When the search stops, a best move is output.

## Usage

Illumina is a UCI-compliant chess engine. In order to use it with a GUI, simply load it as a UCI engine in your preferred chess GUI.

If you want to interact with it directly via the command line, refer to a UCI protocol reference, such as [this one](https://backscattering.de/chess/uci/).

## Building

Illumina uses CMake to generate cross-platform build systems.

### Prerequisites

- CMake 3.15 or higher (build system generator)
- g++/Clang++/MinGW with C++17 compliant versions
    - Other compilers might work, but they are not tested.
    - MSVC notably might run into issues with INCBIN and hasn't been tested.

### Steps

1. Clone the repository with submodules:
```bash
git clone --recursive 
```

2. Generate the build system with CMake:
```bash
cmake -S . -B build # For debug builds
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release # For release builds
```

CMake allows you to specify a build system generator. By default, it will use the system default generator. You can specify a different generator by passing `-G <GENERATOR_NAME>` to the CMake invocation. For example, to use Ninja as the generator, you would run:
```bash
cmake -S . -B build -G Ninja
```

Once you've generated the build system, you can `cd` into the build directory
and build it with your build system command. For instance, assuming you use Make as the generator, you would run:
```bash
cd build
make
```

And Illumina binaries will be generated under the `build` directory.

### Advanced build options

Illumina also offers the following CMake options (that can be specified by `-D<OPTION_NAME>=<ON/OFF>` after the `cmake` command):

- `TUNING`: Specifies that we're creating a tuning build. Tuning build exposes many internal constants as UCI options, such as search and time management constants. `OFF` by default.
- `DEVELOPMENT`: Specifies that we're creating a development build. This is mainly for displaying the correct version name when the engine starts and/or `uci` is called. `ON` by default.
- `INCLUDE_TRACING_MODULE`: Includes Illumina's tracing module, that allows `go trace` commands. 
This option requires the SQLiteCPP submodule to be cloned and may considerably increase executable size. Note that this is forcibly enabled for non-development builds.
`OFF` by default.
- `PROFILING`: Specifies that we're creating a profiling build. Enables some compiler options (namely -g -pg -fno-omit-frame-pointer). `OFF` by default.
- `OPENBENCH_COMPLIANCE_MODE`: Specifies that we're creating a build that is compliant with the OpenBench testing framework, modifying some outputs and UCI options to properly integrate with clients. Not designed to be used directly -- instead, this should only be used by OpenBench clients itself. `OFF` by default.

## Honorable Mentions

- [Marlinflow](https://github.com/jnlt3/marlinflow): Used to train Illumina's evaluation network.
- [Weather Factory](https://github.com/jnlt3/weather-factory): Used to tune search and time control parameters using SPSA.
- [Engine Programming](https://discord.com/invite/F6W6mMsTGN) and [Stockfish](https://discord.gg/GWDRS3kU6R) Discord servers: Highly skilled and helpful members provided very useful advice whenever I needed.
- [OpenBench](https://github.com/AndyGrant/OpenBench): For versions after 2.0, used perform Illumina's tests, tuning sessions and data generation.
- Thanks to Aron Petkovski (author of [Integral](https://github.com/aronpetko/integral)) and everyone else at [FuryBench](https://furybench.com) for supporting me with compute power and very meaningful advice.
