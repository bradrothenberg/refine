#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "refine::refine_with_egadslite" for configuration "Release"
set_property(TARGET refine::refine_with_egadslite APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(refine::refine_with_egadslite PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "C"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/refine_with_egadslite.lib"
  )

list(APPEND _cmake_import_check_targets refine::refine_with_egadslite )
list(APPEND _cmake_import_check_files_for_refine::refine_with_egadslite "${_IMPORT_PREFIX}/lib/refine_with_egadslite.lib" )

# Import target "refine::refine_with_egads" for configuration "Release"
set_property(TARGET refine::refine_with_egads APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(refine::refine_with_egads PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "C"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/refine_with_egads.lib"
  )

list(APPEND _cmake_import_check_targets refine::refine_with_egads )
list(APPEND _cmake_import_check_files_for_refine::refine_with_egads "${_IMPORT_PREFIX}/lib/refine_with_egads.lib" )

# Import target "refine::refine_without_mpi" for configuration "Release"
set_property(TARGET refine::refine_without_mpi APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(refine::refine_without_mpi PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "C"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/refine_without_mpi.lib"
  )

list(APPEND _cmake_import_check_targets refine::refine_without_mpi )
list(APPEND _cmake_import_check_files_for_refine::refine_without_mpi "${_IMPORT_PREFIX}/lib/refine_without_mpi.lib" )

# Import target "refine::refine_core" for configuration "Release"
set_property(TARGET refine::refine_core APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(refine::refine_core PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "C"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/refine_core.lib"
  )

list(APPEND _cmake_import_check_targets refine::refine_core )
list(APPEND _cmake_import_check_files_for_refine::refine_core "${_IMPORT_PREFIX}/lib/refine_core.lib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
