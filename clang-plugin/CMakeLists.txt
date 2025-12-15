add_llvm_library(LoopBoundPlugin MODULE
  LoopBoundPlugin.cpp

  PLUGIN_TOOL clang
)

target_include_directories(LoopBoundPlugin PRIVATE
  ${LLVM_INCLUDE_DIRS}
  ${CMAKE_SOURCE_DIR}/../clang/include
  ${CMAKE_BINARY_DIR}/../clang/include
  ${CMAKE_BINARY_DIR}/tools/clang/include
)

# # With shared libraries, link against Clang libraries normally
# # With shared libraries, link against Clang libraries normally
# # target_link_libraries(LoopBoundPlugin PRIVATE
# #   clangAST
# #   clangBasic
# #   clangFrontend
# #   clangLex
# # )
# # )
