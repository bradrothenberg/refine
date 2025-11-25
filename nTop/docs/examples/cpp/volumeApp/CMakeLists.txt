add_executable(volumeApp main.cpp)

find_package(ntop_core REQUIRED)

list(APPEND volume_sources
  "main.cpp"
)

source_group("volumeApp" FILES ${volume_sources})

target_link_libraries(volumeApp PRIVATE ntop_core)

set_target_properties(volumeApp PROPERTIES OUTPUT_NAME "volume")
