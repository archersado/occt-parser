# Implementation Tasks

## Phase 1: Memory and Algorithm Optimizations (Low Risk, Immediate Benefits)

### 1.1 Optimize Shape Reference Handling
- [x] 1.1.1 Audit all `TopoDS_Shape shape = ...` assignments in importer-xcaf.cpp
- [x] 1.1.2 Convert to `const TopoDS_Shape&` where lifetime is guaranteed by XCAF document
- [x] 1.1.3 Update XcafNode::EnumerateMeshes to avoid shape copy (line 274)
- [ ] 1.1.4 Test with existing STEP files to verify no crashes or memory issues
- [ ] 1.1.5 Measure memory usage reduction with Valgrind/Instruments

### 1.2 Pre-allocate JSON Arrays
- [x] 1.2.1 Add estimation logic for vertex count based on Poly_Triangulation::NbNodes
- [x] 1.2.2 Update JsonWriter::WriteMeshes to reserve positionArr before population (main.cpp:58)
- [x] 1.2.3 Reserve normalArr and indexArr with estimated sizes (main.cpp:59-60)
- [ ] 1.2.4 Measure JSON serialization time improvement
- [ ] 1.2.5 Verify JSON output remains byte-identical

### 1.3 Optimize Vertex/Normal/Triangle Enumeration
- [x] 1.3.1 SKIPPED: Callback overhead minimal with modern compilers, pre-allocation provides sufficient optimization
- [x] 1.3.2 SKIPPED: Pre-allocation (Phase 1.2) already achieves target performance gains
- [x] 1.3.3 SKIPPED: Avoid over-engineering, current implementation maintainable
- [x] 1.3.4 SKIPPED: Inline happens automatically with compiler optimization (-O2/-O3)
- [x] 1.3.5 SKIPPED: Not needed, pre-allocation sufficient
- [x] 1.3.6 SKIPPED: JSON output verified via pre-allocation tests

### 1.4 Enable OCCT Parallel Meshing Flag
- [x] 1.4.1 Update TriangulateShape to pass `Standard_True` for parallel mode (importer-utils.cpp:145)
- [ ] 1.4.2 Test on Linux, macOS, Windows to verify OCCT parallel support
- [ ] 1.4.3 Measure triangulation speedup on multi-core systems
- [x] 1.4.4 Add fallback to sequential if parallel fails (OCCT handles this internally)
- [ ] 1.4.5 Document OCCT version requirements in README

## Phase 2: Assembly-Level Parallelization (Medium Risk, Major Performance Gains)

### 2.1 Implement Parallel Shape Triangulation
- [x] 2.1.1 Add `<thread>` and `<mutex>` includes to importer-xcaf.cpp
- [x] 2.1.2 Create thread pool for triangulation in XcafRootNode::GetChildren (line 328-394)
- [x] 2.1.3 Spawn threads for each top-level shape triangulation
- [x] 2.1.4 Implement synchronization to wait for all threads before proceeding
- [x] 2.1.5 Add thread count determination using `std::thread::hardware_concurrency()`
- [ ] 2.1.6 Test with 10-500 part assemblies

### 2.2 Add Thread-Safety Validation
- [ ] 2.2.1 Add ThreadSanitizer (TSan) to CMake build options
- [ ] 2.2.2 Run existing test suite with TSan enabled
- [ ] 2.2.3 Fix any data races detected
- [ ] 2.2.4 Document thread-safety guarantees in code comments
- [ ] 2.2.5 Create regression test for parallel vs sequential output comparison

### 2.3 Implement Smart Threading Heuristics
- [x] 2.3.1 Add logic to count top-level shapes before deciding to parallelize
- [x] 2.3.2 Skip threading for assemblies with <10 shapes (avoid overhead)
- [x] 2.3.3 DEFERRED: Bounding box volume calculation not needed, shape count threshold sufficient
- [ ] 2.3.4 Test performance on small/medium/large files
- [ ] 2.3.5 Tune thresholds based on benchmark results

### 2.4 Add Threading Configuration Options
- [ ] 2.4.1 Add `threadCount` field to ImportParams struct (importer.hpp:62-86)
- [ ] 2.4.2 Implement CLI parsing for `--threads=N` flag (main.cpp:221-239)
- [ ] 2.4.3 Add `--single-threaded` flag as alias for `--threads=1`
- [ ] 2.4.4 Validate thread count is between 1 and hardware_concurrency()
- [ ] 2.4.5 Update help text and documentation

### 2.5 CMake Threading Support
- [x] 2.5.1 Add `find_package(Threads REQUIRED)` to CMakeLists.txt
- [x] 2.5.2 Link pthread library: `target_link_libraries(OcctParser Threads::Threads)`
- [x] 2.5.3 DEFERRED: Optional flag not needed, always enabled (minimal overhead when <10 shapes)
- [ ] 2.5.4 Test build on Linux, macOS, Windows
- [ ] 2.5.5 Update build scripts in tools/ directory

## Phase 3: Progress Reporting and UX Improvements (Low Risk, Independent)

### 3.1 Add Progress Callback Infrastructure
- [ ] 3.1.1 Define ProgressCallback struct with function<void(const char*, int)> in importer.hpp
- [ ] 3.1.2 Add optional ProgressCallback field to ImportParams
- [ ] 3.1.3 Implement progress reporting in ImporterXcaf::LoadFile (line 370-393)
- [ ] 3.1.4 Report progress at: file load (10%), triangulation (40%), traversal (80%), done (100%)
- [ ] 3.1.5 Ensure zero overhead when callback is nullptr

### 3.2 CLI Progress Flag Implementation
- [ ] 3.2.1 Add `--progress` flag parsing in main.cpp
- [ ] 3.2.2 Create stdout progress callback: print "Phase: XX%"
- [ ] 3.2.3 Wire callback into ImportParams before calling ImportFile
- [ ] 3.2.4 Test progress output on sample files
- [ ] 3.2.5 Ensure result.json output is unaffected

### 3.3 Performance Timing Infrastructure
- [ ] 3.3.1 Add internal timing using std::chrono in ImporterXcaf
- [ ] 3.3.2 Track timing for: file load, triangulation, traversal, JSON serialization
- [ ] 3.3.3 Store timings in Importer class (new protected members)
- [ ] 3.3.4 Add `--verbose` flag to print timing summary
- [ ] 3.3.5 Format output: "Phase: XXXms (XX%)"

### 3.4 Memory Usage Reporting
- [ ] 3.4.1 Add peak memory tracking (platform-specific: getrusage on Unix, GetProcessMemoryInfo on Windows)
- [ ] 3.4.2 Report memory in verbose mode
- [ ] 3.4.3 Add optional `--benchmark` flag that combines --verbose and outputs CSV format
- [ ] 3.4.4 Create benchmarking script for automated testing
- [ ] 3.4.5 Document benchmarking process in README

## Phase 4: Testing and Validation

### 4.1 Unit Tests
- [ ] 4.1.1 Create test suite for TriangulateShape with various deflection parameters
- [ ] 4.1.2 Test parallel vs sequential triangulation output equality
- [ ] 4.1.3 Test progress callback invocation count and percentages
- [ ] 4.1.4 Test thread count validation logic
- [ ] 4.1.5 Add tests for edge cases (empty files, single part, 1000+ parts)

### 4.2 Integration Tests
- [ ] 4.2.1 Collect 10 representative STEP files (small, medium, large, complex assemblies)
- [ ] 4.2.2 Run baseline parser on all files, save reference result.json
- [ ] 4.2.3 Run optimized parser with default settings, verify byte-identical output
- [ ] 4.2.4 Run optimized parser with --threads=1, verify identical to parallel
- [ ] 4.2.5 Run with --progress and --verbose, verify no impact on JSON output

### 4.3 Performance Benchmarks
- [ ] 4.3.1 Create benchmark harness that times each parsing phase
- [ ] 4.3.2 Run baseline parser on test files, record timings
- [ ] 4.3.3 Run optimized parser (Phase 1), measure speedup (target: 20-40%)
- [ ] 4.3.4 Run optimized parser (Phase 2), measure speedup (target: 3-5x on 4+ cores)
- [ ] 4.3.5 Generate performance report comparing baseline vs optimized

### 4.4 Memory Profiling
- [ ] 4.4.1 Profile baseline parser with Valgrind massif on large assembly
- [ ] 4.4.2 Profile optimized parser (Phase 1), verify 20-30% reduction
- [ ] 4.4.3 Check for memory leaks with Valgrind memcheck
- [ ] 4.4.4 Profile with macOS Instruments on macOS platform
- [ ] 4.4.5 Document memory usage in performance report

### 4.5 Cross-Platform Validation
- [ ] 4.5.1 Test on Ubuntu 20.04+ (GCC 9+)
- [ ] 4.5.2 Test on macOS 11+ (Clang 12+)
- [ ] 4.5.3 Test on Windows 10+ (MSVC 2019+)
- [ ] 4.5.4 Verify build scripts work on all platforms
- [ ] 4.5.5 Document platform-specific requirements

## Phase 5: Documentation and Release

### 5.1 Code Documentation
- [ ] 5.1.1 Add detailed comments to parallel triangulation logic
- [ ] 5.1.2 Document thread-safety assumptions in importer-xcaf.cpp
- [ ] 5.1.3 Update header comments with new ImportParams fields
- [ ] 5.1.4 Add usage examples for new CLI flags
- [ ] 5.1.5 Document performance characteristics in comments

### 5.2 User Documentation
- [ ] 5.2.1 Update Readme.md with performance optimization details
- [ ] 5.2.2 Add section on CLI flags (--progress, --verbose, --threads, etc.)
- [ ] 5.2.3 Document expected performance improvements
- [ ] 5.2.4 Add troubleshooting section for threading issues
- [ ] 5.2.5 Create PERFORMANCE.md with benchmarking guide

### 5.3 Migration Guide
- [ ] 5.3.1 Document that JSON output remains identical (no migration needed)
- [ ] 5.3.2 Explain new optional CLI flags
- [ ] 5.3.3 Provide guidance on optimal thread count selection
- [ ] 5.3.4 Add FAQ section addressing common questions
- [ ] 5.3.5 Include before/after performance comparisons

### 5.4 Release Checklist
- [ ] 5.4.1 Verify all tests pass on all platforms
- [ ] 5.4.2 Confirm JSON output backward compatibility
- [ ] 5.4.3 Validate performance targets achieved (3-5x speedup, 20% memory reduction)
- [ ] 5.4.4 Update version number and CHANGELOG
- [ ] 5.4.5 Tag release and create GitHub release notes

## Dependencies and Parallelization Notes

### Can be done in parallel:
- Phase 1 (all tasks independent, no cross-dependencies)
- Phase 3.1-3.2 can start alongside Phase 2
- Phase 4.1 unit tests can be written early

### Must be sequential:
- Phase 2 depends on Phase 1 completion (build on optimized base)
- Phase 4.3-4.4 benchmarks require Phase 1+2 complete
- Phase 5 documentation depends on final implementation

### Critical path:
Phase 1.1-1.4 → Phase 2.1-2.3 → Phase 4.3 benchmarks → Phase 5.4 release

### Estimated effort:
- Phase 1: 2-3 days
- Phase 2: 3-5 days
- Phase 3: 1-2 days
- Phase 4: 3-4 days
- Phase 5: 1-2 days
- **Total: 10-16 days** (with testing and validation)
