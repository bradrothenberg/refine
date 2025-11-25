add_executable(closestpointApp main.cpp)

find_package(ntop_core REQUIRED)

list(APPEND closestpoint_sources
  "main.cpp"
)

source_group("closestpointApp" FILES ${closestpoint_sources})

target_link_libraries(closestpointApp PRIVATE ntop_core)

set_target_properties(closestpointApp PROPERTIES OUTPUT_NAME "closestpoint")
