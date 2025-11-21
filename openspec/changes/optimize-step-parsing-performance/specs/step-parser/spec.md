# STEP Parser Performance Optimization

## ADDED Requirements

### Requirement: Parallel Mesh Triangulation
The parser SHALL support parallel triangulation of CAD models to improve performance on multi-core systems.

#### Scenario: Enable OCCT parallel meshing
- **WHEN** TriangulateShape is called on a TopoDS_Shape
- **THEN** BRepMesh_IncrementalMesh SHALL be created with parallel mode enabled (Standard_True)
- **AND** the triangulation SHALL complete successfully

#### Scenario: Assembly-level parallelization
- **WHEN** multiple independent shapes exist in the XCAF assembly hierarchy
- **THEN** the parser SHALL triangulate top-level shapes concurrently using std::thread
- **AND** each thread SHALL operate on independent TopoDS_Shape instances
- **AND** the results SHALL be thread-safe with no data races

#### Scenario: Automatic thread count selection
- **WHEN** parallel triangulation is enabled
- **THEN** the parser SHALL use std::thread::hardware_concurrency() to determine optimal thread count
- **AND** SHALL limit threads to avoid over-subscription

### Requirement: Memory Optimization
The parser SHALL minimize memory allocations and copies during geometry extraction to reduce peak memory usage.

#### Scenario: Use reference semantics for shapes
- **WHEN** accessing TopoDS_Shape from XCAF labels
- **THEN** the code SHALL prefer const references over value copies where lifetime permits
- **AND** SHALL avoid unnecessary Handle reference count increments

#### Scenario: Pre-allocate JSON arrays
- **WHEN** building JSON arrays for vertices, normals, or indices
- **THEN** the parser SHALL estimate array sizes and call reserve() before population
- **AND** SHALL reduce reallocation overhead during JSON construction

#### Scenario: Cache XCAF label lookups
- **WHEN** the same TDF_Label is accessed multiple times (e.g., for shape and color queries)
- **THEN** the parser SHALL cache the lookup results
- **AND** SHALL use an LRU cache with configurable maximum size
- **AND** SHALL clear cache after processing each top-level shape to bound memory usage

### Requirement: Batch Geometry Enumeration
The parser SHALL optimize vertex, normal, and triangle extraction to minimize callback overhead.

#### Scenario: Direct vertex enumeration
- **WHEN** extracting vertices from a triangulated face
- **THEN** the parser SHALL use direct iteration over Poly_Triangulation nodes instead of per-vertex callbacks
- **AND** SHALL batch-transform vertices using the face's TopLoc_Location
- **AND** SHALL achieve lower per-vertex processing cost than callback-based approach

#### Scenario: Inlined triangle extraction
- **WHEN** extracting triangle indices from a face
- **THEN** the parser SHALL iterate Poly_Triangulation triangles directly
- **AND** SHALL apply orientation correction inline without callback overhead

### Requirement: Progress Reporting
The parser SHALL support optional progress reporting for long-running operations without affecting default behavior.

#### Scenario: Progress callback mechanism
- **WHEN** ImportParams includes a ProgressCallback
- **THEN** the parser SHALL invoke the callback at key milestones (file loading, triangulation, traversal, serialization)
- **AND** SHALL report phase name and percentage complete
- **AND** SHALL have zero overhead when callback is not provided (nullptr check)

#### Scenario: CLI progress flag
- **WHEN** the executable is invoked with `--progress` flag
- **THEN** progress updates SHALL be printed to stdout
- **AND** SHALL show current phase and percentage
- **AND** SHALL not affect JSON output to result.json

#### Scenario: Quiet mode preservation
- **WHEN** no `--progress` flag is provided (default)
- **THEN** the parser SHALL not print any progress information
- **AND** behavior SHALL be identical to pre-optimization version

### Requirement: Performance Metrics
The parser SHALL optionally collect and report detailed performance timing for each parsing phase.

#### Scenario: Verbose timing output
- **WHEN** the executable is invoked with `--verbose` flag
- **THEN** detailed timing for each phase SHALL be printed to stdout after completion
- **AND** SHALL include: file load time, triangulation time, traversal time, JSON serialization time
- **AND** SHALL report memory usage statistics if available

#### Scenario: Default silent operation
- **WHEN** `--verbose` flag is not provided
- **THEN** no timing information SHALL be printed
- **AND** behavior SHALL match original parser output

### Requirement: Backward Compatibility
The parser SHALL maintain full backward compatibility in output format and default CLI behavior.

#### Scenario: Identical JSON output
- **WHEN** parsing the same STEP file before and after optimization
- **THEN** the generated result.json SHALL be byte-for-byte identical
- **AND** all vertex coordinates SHALL round to 3 decimal places as before
- **AND** mesh structure, hierarchy, and colors SHALL match exactly

#### Scenario: CLI interface compatibility
- **WHEN** the parser is invoked with existing command-line arguments (filepath and params JSON)
- **THEN** it SHALL parse arguments identically to the original version
- **AND** SHALL produce the same result.json output
- **AND** new optional flags SHALL not interfere with existing usage patterns

#### Scenario: Build compatibility
- **WHEN** building the project with CMake
- **THEN** it SHALL compile with C++11 standard (existing requirement)
- **AND** SHALL link against the same OCCT modules
- **AND** MAY optionally link threading library (pthread, std::thread)

### Requirement: Thread Safety
The parser SHALL ensure thread-safe operation when parallel processing is enabled.

#### Scenario: Independent shape processing
- **WHEN** multiple threads triangulate different shapes concurrently
- **THEN** each thread SHALL operate on independent TopoDS_Shape instances
- **AND** SHALL not share mutable state with other threads
- **AND** SHALL not cause data races or undefined behavior

#### Scenario: Thread-safe XCAF access
- **WHEN** accessing XCAF document tools (ShapeTool, ColorTool) from multiple threads
- **THEN** read-only operations SHALL be thread-safe
- **AND** each top-level shape SHALL be fully triangulated before parallel data extraction begins
- **AND** no concurrent modifications to the same XCAF label SHALL occur

#### Scenario: Fallback to single-threaded mode
- **WHEN** thread-safety issues are detected or parallelization is disabled via configuration
- **THEN** the parser SHALL fall back to sequential processing
- **AND** SHALL produce correct results identical to parallel mode

### Requirement: Performance Scalability
The parser SHALL demonstrate measurable performance improvements for large CAD models.

#### Scenario: Large assembly speedup
- **WHEN** parsing a STEP file with >100 top-level parts
- **THEN** the optimized parser SHALL complete 3-5x faster than the baseline on a 4+ core system
- **AND** SHALL show linear scaling up to the number of available CPU cores

#### Scenario: Memory reduction
- **WHEN** parsing any STEP file
- **THEN** the optimized parser SHALL use 20-30% less peak memory than baseline
- **AND** SHALL not introduce memory leaks or excessive allocations

#### Scenario: Small file overhead
- **WHEN** parsing a small STEP file (<10 parts)
- **THEN** the threading overhead SHALL not slow down processing
- **AND** performance SHALL be within 5% of single-threaded baseline
- **AND** MAY automatically disable parallelization for small files

### Requirement: Optional Parallel Mode Configuration
The parser SHALL allow users to control parallelization behavior via CLI flags.

#### Scenario: Thread count control
- **WHEN** the executable is invoked with `--threads=N` flag
- **THEN** the parser SHALL use exactly N threads for parallel processing
- **AND** SHALL validate N is between 1 and hardware_concurrency()
- **AND** SHALL error if N is invalid

#### Scenario: Disable parallelization
- **WHEN** the executable is invoked with `--threads=1` or `--single-threaded` flag
- **THEN** the parser SHALL use sequential processing only
- **AND** SHALL produce identical results to parallel mode

#### Scenario: Default threading behavior
- **WHEN** no threading flags are provided
- **THEN** the parser SHALL use std::thread::hardware_concurrency() threads
- **AND** SHALL automatically tune for optimal performance
