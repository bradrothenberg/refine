# Install script for directory: C:/cplusplus/ntop/refine/src

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "C:/Program Files (x86)/refine")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "C:/cplusplus/ntop/refine/build/src/Debug/refine_with_egadslite.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "C:/cplusplus/ntop/refine/build/src/Release/refine_with_egadslite.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "C:/cplusplus/ntop/refine/build/src/MinSizeRel/refine_with_egadslite.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "C:/cplusplus/ntop/refine/build/src/RelWithDebInfo/refine_with_egadslite.lib")
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "C:/cplusplus/ntop/refine/build/src/Debug/refine_with_egads.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "C:/cplusplus/ntop/refine/build/src/Release/refine_with_egads.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "C:/cplusplus/ntop/refine/build/src/MinSizeRel/refine_with_egads.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "C:/cplusplus/ntop/refine/build/src/RelWithDebInfo/refine_with_egads.lib")
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "C:/cplusplus/ntop/refine/build/src/Debug/refine_without_mpi.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "C:/cplusplus/ntop/refine/build/src/Release/refine_without_mpi.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "C:/cplusplus/ntop/refine/build/src/MinSizeRel/refine_without_mpi.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "C:/cplusplus/ntop/refine/build/src/RelWithDebInfo/refine_without_mpi.lib")
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "C:/cplusplus/ntop/refine/build/src/Debug/refine_core.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "C:/cplusplus/ntop/refine/build/src/Release/refine_core.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "C:/cplusplus/ntop/refine/build/src/MinSizeRel/refine_core.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "C:/cplusplus/ntop/refine/build/src/RelWithDebInfo/refine_core.lib")
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE EXECUTABLE FILES "C:/cplusplus/ntop/refine/build/src/Debug/ref.exe")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE EXECUTABLE FILES "C:/cplusplus/ntop/refine/build/src/Release/ref.exe")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE EXECUTABLE FILES "C:/cplusplus/ntop/refine/build/src/MinSizeRel/ref.exe")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE EXECUTABLE FILES "C:/cplusplus/ntop/refine/build/src/RelWithDebInfo/ref.exe")
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    include("C:/cplusplus/ntop/refine/build/src/CMakeFiles/ref.dir/install-cxx-module-bmi-Debug.cmake" OPTIONAL)
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    include("C:/cplusplus/ntop/refine/build/src/CMakeFiles/ref.dir/install-cxx-module-bmi-Release.cmake" OPTIONAL)
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
    include("C:/cplusplus/ntop/refine/build/src/CMakeFiles/ref.dir/install-cxx-module-bmi-MinSizeRel.cmake" OPTIONAL)
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
    include("C:/cplusplus/ntop/refine/build/src/CMakeFiles/ref.dir/install-cxx-module-bmi-RelWithDebInfo.cmake" OPTIONAL)
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/cmake/refine.cmake")
    file(DIFFERENT _cmake_export_file_changed FILES
         "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/cmake/refine.cmake"
         "C:/cplusplus/ntop/refine/build/src/CMakeFiles/Export/272ceadb8458515b2ae4b5630a6029cc/refine.cmake")
    if(_cmake_export_file_changed)
      file(GLOB _cmake_old_config_files "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/cmake/refine-*.cmake")
      if(_cmake_old_config_files)
        string(REPLACE ";" ", " _cmake_old_config_files_text "${_cmake_old_config_files}")
        message(STATUS "Old export file \"$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/cmake/refine.cmake\" will be replaced.  Removing files [${_cmake_old_config_files_text}].")
        unset(_cmake_old_config_files_text)
        file(REMOVE ${_cmake_old_config_files})
      endif()
      unset(_cmake_old_config_files)
    endif()
    unset(_cmake_export_file_changed)
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/cmake" TYPE FILE FILES "C:/cplusplus/ntop/refine/build/src/CMakeFiles/Export/272ceadb8458515b2ae4b5630a6029cc/refine.cmake")
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/cmake" TYPE FILE FILES "C:/cplusplus/ntop/refine/build/src/CMakeFiles/Export/272ceadb8458515b2ae4b5630a6029cc/refine-debug.cmake")
  endif()
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/cmake" TYPE FILE FILES "C:/cplusplus/ntop/refine/build/src/CMakeFiles/Export/272ceadb8458515b2ae4b5630a6029cc/refine-minsizerel.cmake")
  endif()
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/cmake" TYPE FILE FILES "C:/cplusplus/ntop/refine/build/src/CMakeFiles/Export/272ceadb8458515b2ae4b5630a6029cc/refine-relwithdebinfo.cmake")
  endif()
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/cmake" TYPE FILE FILES "C:/cplusplus/ntop/refine/build/src/CMakeFiles/Export/272ceadb8458515b2ae4b5630a6029cc/refine-release.cmake")
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/refine" TYPE FILE FILES
    "C:/cplusplus/ntop/refine/src/ref_adapt.h"
    "C:/cplusplus/ntop/refine/src/ref_adj.h"
    "C:/cplusplus/ntop/refine/src/ref_agents.h"
    "C:/cplusplus/ntop/refine/src/ref_args.h"
    "C:/cplusplus/ntop/refine/src/ref_axi.h"
    "C:/cplusplus/ntop/refine/src/ref_cavity.h"
    "C:/cplusplus/ntop/refine/src/ref_cell.h"
    "C:/cplusplus/ntop/refine/src/ref_cloud.h"
    "C:/cplusplus/ntop/refine/src/ref_clump.h"
    "C:/cplusplus/ntop/refine/src/ref_collapse.h"
    "C:/cplusplus/ntop/refine/src/ref_comprow.h"
    "C:/cplusplus/ntop/refine/src/ref_defs.h"
    "C:/cplusplus/ntop/refine/src/ref_dict.h"
    "C:/cplusplus/ntop/refine/src/ref_dist.h"
    "C:/cplusplus/ntop/refine/src/ref_edge.h"
    "C:/cplusplus/ntop/refine/src/ref_egads.h"
    "C:/cplusplus/ntop/refine/src/ref_elast.h"
    "C:/cplusplus/ntop/refine/src/ref_endian.h"
    "C:/cplusplus/ntop/refine/src/ref_export.h"
    "C:/cplusplus/ntop/refine/src/ref_face.h"
    "C:/cplusplus/ntop/refine/src/ref_facelift.h"
    "C:/cplusplus/ntop/refine/src/ref_fixture.h"
    "C:/cplusplus/ntop/refine/src/ref_fortran.h"
    "C:/cplusplus/ntop/refine/src/ref_gather.h"
    "C:/cplusplus/ntop/refine/src/ref_geom.h"
    "C:/cplusplus/ntop/refine/src/ref_grid.h"
    "C:/cplusplus/ntop/refine/src/ref_histogram.h"
    "C:/cplusplus/ntop/refine/src/ref_html.h"
    "C:/cplusplus/ntop/refine/src/ref_import.h"
    "C:/cplusplus/ntop/refine/src/ref_inflate.h"
    "C:/cplusplus/ntop/refine/src/ref_interp.h"
    "C:/cplusplus/ntop/refine/src/ref_iso.h"
    "C:/cplusplus/ntop/refine/src/ref_layer.h"
    "C:/cplusplus/ntop/refine/src/ref_list.h"
    "C:/cplusplus/ntop/refine/src/ref_malloc.h"
    "C:/cplusplus/ntop/refine/src/ref_math.h"
    "C:/cplusplus/ntop/refine/src/ref_matrix.h"
    "C:/cplusplus/ntop/refine/src/ref_meshlink.h"
    "C:/cplusplus/ntop/refine/src/ref_metric.h"
    "C:/cplusplus/ntop/refine/src/ref_migrate.h"
    "C:/cplusplus/ntop/refine/src/ref_ntop.h"
    "C:/cplusplus/ntop/refine/src/ref_mpi.h"
    "C:/cplusplus/ntop/refine/src/ref_node.h"
    "C:/cplusplus/ntop/refine/src/ref_oct.h"
    "C:/cplusplus/ntop/refine/src/ref_part.h"
    "C:/cplusplus/ntop/refine/src/ref_phys.h"
    "C:/cplusplus/ntop/refine/src/ref_recon.h"
    "C:/cplusplus/ntop/refine/src/ref_search.h"
    "C:/cplusplus/ntop/refine/src/ref_shard.h"
    "C:/cplusplus/ntop/refine/src/ref_smooth.h"
    "C:/cplusplus/ntop/refine/src/ref_sort.h"
    "C:/cplusplus/ntop/refine/src/ref_split.h"
    "C:/cplusplus/ntop/refine/src/ref_subdiv.h"
    "C:/cplusplus/ntop/refine/src/ref_swap.h"
    "C:/cplusplus/ntop/refine/src/ref_validation.h"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/cmake" TYPE FILE FILES "C:/cplusplus/ntop/refine/src/refineConfig.cmake")
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
if(CMAKE_INSTALL_LOCAL_ONLY)
  file(WRITE "C:/cplusplus/ntop/refine/build/src/install_local_manifest.txt"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
