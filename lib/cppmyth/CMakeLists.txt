cmake_minimum_required (VERSION 2.6 FATAL_ERROR)
cmake_policy (VERSION 2.6)
#
# Every module has a CMakeLists.txt in it's root. As we saw, those files
# provide the necessary directions to build each module.
set (MODULES
  ${CMAKE_SOURCE_DIR}/jansson
  ${CMAKE_SOURCE_DIR}/cppmyth

#
# Include each CMakeLists.txt for each module.
# add_subdirectory searches for a CMakeLists.txt file in the given
# directory, so that's what we use to include each module's build script.
foreach (MODULE ${MODULES})
  add_subdirectory (${MODULE})
endforeach ()

