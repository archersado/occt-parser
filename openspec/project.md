# Project Context

## Purpose
OCCT Parser is a command-line tool that converts CAD files (STEP, IGES, BREP formats) into structured JSON format containing:
- Hierarchical scene graph (nodes/children structure)
- Triangulated mesh data (vertices, normals, indices)
- BREP face topology information
- Color attributes at both mesh and face levels
- Configurable mesh tessellation parameters

**Primary use case**: Convert industry-standard CAD files into a format suitable for web-based 3D visualization, game engines, or custom renderers.

## Tech Stack
- **Language**: C++11
- **Build System**: CMake 3.6+
- **Core Library**: Open CASCADE Technology (OCCT) - comprehensive CAD kernel
  - Includes 200+ OCCT modules for geometry, topology, visualization, and data exchange
- **JSON Library**: nlohmann/json (included in source)
- **Graphics Support**: FreeType 2 (for text rendering in OCCT)
- **Platform**: Cross-platform (Linux, macOS, Windows)
- **Output Format**: JSON with compressed mesh data

## Project Conventions

### Code Style
- **File Organization**:
  - Headers use `.hpp` extension
  - Implementation files use `.cpp` extension
  - One class per file pair (e.g., `importer-step.hpp` / `importer-step.cpp`)
- **Naming Conventions**:
  - Classes: PascalCase (e.g., `ImporterStep`, `JsonWriter`)
  - Member variables: camelCase (e.g., `meshCount`, `linearUnit`)
  - Functions: PascalCase for public methods (e.g., `GetName`, `LoadFile`)
  - Enums: PascalCase with scoped enum class (e.g., `LinearUnit::Millimeter`)
- **Memory Management**:
  - Use `std::shared_ptr` for node hierarchy (`NodePtr`)
  - Virtual destructors for base classes
  - RAII principles throughout
- **Code Comments**: Chinese comments in implementation (现有代码中)
- **Includes**: OCCT headers use `.hxx` extension (e.g., `BRepTools.hxx`)

### Architecture Patterns
- **Importer Pattern**: Abstract base class `Importer` with format-specific implementations:
  - `ImporterStep` - STEP/STP file format
  - `ImporterIges` - IGES/IGS file format
  - `ImporterBrep` - BREP/BRP native OpenCASCADE format
- **Composite Pattern**: Node hierarchy with `Node`, `Mesh`, and `Face` abstractions
- **Visitor Pattern**: Enumeration callbacks for traversing geometry (`EnumerateVertices`, `EnumerateFaces`, `EnumerateMeshes`)
- **Builder Pattern**: `JsonWriter` constructs JSON incrementally with global mesh deduplication
- **Factory Selection**: File extension determines which importer to instantiate
- **Separation of Concerns**:
  - Importers: File loading and OCCT integration
  - Utils: Shared geometry/mesh processing utilities
  - XCAF: Extended data support (colors, layers, metadata)
  - Main: CLI interface and JSON output

### Testing Strategy
Currently no formal test suite. Testing is manual:
1. Run parser on sample CAD files
2. Verify `result.json` output structure
3. Visual validation in downstream consumers

**Future testing needs**: Unit tests for importers, integration tests with various CAD files, mesh quality validation.

### Git Workflow
- **Branch**: `main` is the primary development branch
- **Commit Style**: Currently uses simple "feat: add" messages (not very descriptive)
- **Recommended**: Adopt conventional commits format:
  - `feat: add IGES color support`
  - `fix: handle invalid STEP topology`
  - `refactor: extract mesh tessellation logic`
  - `docs: update build instructions`
- **No CI/CD**: Manual builds using scripts in `tools/` directory

## Domain Context

### CAD File Formats
- **STEP (.stp, .step)**: ISO 10303 standard for product data exchange
  - Industry standard for mechanical CAD
  - Contains parametric geometry, topology, assemblies, metadata
- **IGES (.igs, .iges)**: Initial Graphics Exchange Specification
  - Older format, still widely used
  - Less robust than STEP for complex assemblies
- **BREP (.brp, .brep)**: Boundary Representation
  - Native OpenCASCADE format
  - Direct topology and geometry representation

### OCCT Concepts
- **BREP Topology**: Hierarchical structure of Shell → Face → Wire → Edge → Vertex
- **Tessellation**: Converting parametric surfaces into triangle meshes
  - `linearDeflection`: Maximum distance between surface and mesh
  - `angularDeflection`: Maximum angle between surface normals
- **XCAF (Extended CAF)**: OpenCASCADE's application framework for:
  - Assembly structure and part hierarchies
  - Color and material attributes
  - Layers and metadata

### Mesh Generation Parameters
- **Linear Unit**: Input file units (millimeter, centimeter, meter, inch, foot)
- **Linear Deflection Type**:
  - `BoundingBoxRatio`: Deflection as percentage of bounding box diagonal
  - `AbsoluteValue`: Fixed deflection in model units
- **Vertex Precision**: Rounded to 3 decimal places in JSON output

## Important Constraints

### Technical Constraints
- **OCCT Dependency**: Massive library (200+ modules) makes compilation slow
  - Each module explicitly listed in CMakeLists.txt
  - No plugin system (`-DOCCT_NO_PLUGINS` flag)
- **Platform-Specific Paths**:
  - FreeType includes hardcoded to Homebrew on macOS (`/opt/homebrew`)
  - Requires adjustment for other platforms/package managers
- **Memory Usage**: Large CAD files can consume significant memory during:
  - OCCT shape loading
  - Mesh tessellation
  - JSON serialization
- **Unicode**: Built with UNICODE support (`-DUNICODE -D_UNICODE`)

### Performance Constraints
- **Tessellation Cost**: High-quality meshes (low deflection) can be very slow
- **Single-Threaded**: No parallel processing of faces/meshes
- **File I/O**: Entire JSON written to disk at once (not streamed)

### Functional Constraints
- **Output Format**: Fixed JSON schema (not configurable)
- **Mesh-Only**: No support for:
  - Parametric curve data
  - PMI (Product Manufacturing Information)
  - Kinematics or tolerances
- **Color Handling**: Depends on XCAF data availability in source file

## External Dependencies

### Build-Time Dependencies
- **CMake**: Version 3.6 or higher
- **C++ Compiler**: C++11 support required (GCC, Clang, MSVC)
- **FreeType 2**: Font rendering library
  - macOS Homebrew: `/opt/homebrew/opt/freetype`
  - Needs adaptation for other platforms

### Runtime Dependencies
- **None**: Statically linked executable (OcctParser)
- **Embeds**: OCCT library compiled into binary
- **Output**: `result.json` file written to current directory

### Included Libraries
- **OCCT Source**: Full Open CASCADE source in `occt/` directory
- **nlohmann/json**: Header-only JSON library (assumed in source)

### External Tools
- **Build Scripts**: Platform-specific shell scripts in `tools/`
  - `build_native_linux_release.sh` / `build_native_linux_debug.sh`
  - `build_native_win_release.bat` / `build_native_win_debug.bat`
