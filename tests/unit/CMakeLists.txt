# Standalone unit tests for the modular cache analysis (no GoogleTest).
add_llvm_executable(LLTACacheModuleTests
  CacheModuleTests.cpp
)
# Header-only modules pull in llvm/ADT/StringRef.h; link Support for its symbols.
target_link_libraries(LLTACacheModuleTests PRIVATE LLVMSupport)

# Register with CTest...
add_test(NAME LLTACacheModuleTests COMMAND LLTACacheModuleTests)

# ...and provide a convenience target that builds and runs them directly:
#   cmake --build <build> --target check-llta-cache
add_custom_target(check-llta-cache
  COMMAND LLTACacheModuleTests
  DEPENDS LLTACacheModuleTests
  COMMENT "Running LLTA cache module unit tests"
)
