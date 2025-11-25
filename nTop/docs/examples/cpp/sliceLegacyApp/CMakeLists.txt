add_executable(sliceLegacyApp main.cpp)

find_package(ntop_core REQUIRED)

list(APPEND slice_legacy_sources
  "main.cpp"
)

source_group("sliceLegacyApp" FILES ${slice_legacy_sources})

target_link_libraries(sliceLegacyApp PRIVATE ntop_core)
target_link_libraries(sliceLegacyApp PUBLIC matplot)

set_target_properties(sliceLegacyApp PROPERTIES OUTPUT_NAME "sliceLegacy")
