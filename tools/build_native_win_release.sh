#!/bin/bash

# Save the current directory
pushd "$(dirname "$0")/.." > /dev/null

# Run CMake to configure the project
cmake -B build/native -G "Unix Makefiles" .
if [ $? -ne 0 ]; then
  echo "CMake configuration failed."
  popd > /dev/null
  exit 1
fi

# Build the project with the specified configuration
cmake --build build/native --config RelWithDebInfo
if [ $? -ne 0 ]; then
  echo "Build failed."
  popd > /dev/null
  exit 1
fi

# Restore the original directory
popd > /dev/null
exit 0
