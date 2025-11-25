add_executable(boundingboxApp main.cpp)

find_package(ntop_core REQUIRED)

list(APPEND boundingbox_sources
  "main.cpp"
)

source_group("boundingboxApp" FILES ${boundingbox_sources})

target_link_libraries(boundingboxApp PRIVATE ntop_core)

set_target_properties(boundingboxApp PROPERTIES OUTPUT_NAME "boundingbox")
