add_executable(sliceApp main.cpp)

find_package(ntop_core REQUIRED)

list(APPEND slice_sources
  "main.cpp"
)

source_group("sliceApp" FILES ${slice_sources})

target_link_libraries(sliceApp PRIVATE ntop_core)
target_link_libraries(sliceApp PUBLIC matplot)

set_target_properties(sliceApp PROPERTIES OUTPUT_NAME "slice")
