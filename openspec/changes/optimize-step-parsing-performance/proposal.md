# Optimize STEP File Parsing Performance

## Why

STEP file parsing currently suffers from significant performance bottlenecks, particularly during mesh triangulation and data traversal phases. Large CAD models (assemblies with hundreds of parts or complex surfaces) can take minutes to process, with excessive memory consumption and slow user experience. The primary bottleneck is the single-threaded BRepMesh triangulation and inefficient data structure traversal during geometry extraction.

## What Changes

This proposal introduces a comprehensive performance optimization strategy that maintains full backward compatibility:

1. **Parallel Mesh Triangulation**
   - Enable OCCT's parallel mode for BRepMesh_IncrementalMesh
   - Implement concurrent triangulation of independent shapes in the assembly hierarchy
   - Add multi-threaded face iteration for large solids

2. **Memory and Algorithm Optimization**
   - Reduce redundant shape copying during hierarchy traversal
   - Pre-allocate JSON array buffers based on estimated sizes
   - Cache frequently accessed XCAF labels and shapes
   - Optimize vertex/normal/triangle enumeration to reduce callback overhead

3. **Progressive Processing (Optional Feature)**
   - Add optional `--progress` flag to report parsing progress to stdout
   - Stream JSON output for large models (optional `--stream` mode)
   - Early validation to fail fast on invalid files

4. **Performance Monitoring**
   - Add internal timing instrumentation for each parsing phase
   - Optional `--verbose` flag to output performance metrics
   - No impact on default behavior

All optimizations are implemented as performance improvements to existing code paths or optional CLI flags, ensuring **no breaking changes** to JSON output format or default CLI behavior.

## Impact

### Affected Specs
- `step-parser` (new spec) - Comprehensive STEP parsing requirements

### Affected Code
- `src/importer-xcaf.cpp:332` - TriangulateShape call in XcafRootNode::GetChildren
- `src/importer-utils.cpp:118-146` - TriangulateShape implementation
- `src/importer-xcaf.cpp:268-300` - EnumerateMeshes and shape enumeration
- `src/main.cpp:42-141` - JsonWriter mesh enumeration
- `src/main.cpp:221-259` - CLI argument parsing and main loop
- `CMakeLists.txt` - May need threading library linkage

### Performance Goals
- **Target**: 3-5x speedup for models with >100 parts
- **Memory**: Reduce peak memory usage by 20-30% through better allocation
- **User Experience**: Progress feedback for operations >5 seconds

### Compatibility
- JSON output format: **No changes** (byte-for-byte identical)
- CLI interface: **Backward compatible** (new flags are optional)
- Build requirements: C++11 threading support (already available)
