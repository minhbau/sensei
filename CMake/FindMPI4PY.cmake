#*****************************************************************************
# FindMpi4Py
#
# Check if mpi4py is installed and configure c-api includes
#
# This module defines
#  MPI4PY_FOUND, set TRUE if mpi4py and c-api are available
#  MPI4PY_INCLUDE_DIR, where to find c-api headers
#  MPI4PY_VERSION, mpi4py release version
set(_TMP_PY_OUTPUT)
set(_TMP_PY_RETURN)
exec_program("${PYTHON_EXECUTABLE}"
  ARGS "-c 'import mpi4py; print(mpi4py.get_include())'"
  OUTPUT_VARIABLE _TMP_PY_OUTPUT
  RETURN_VALUE _TMP_PY_RETURN)
set(MPI4PY_INCLUDE_FOUND FALSE)
if(NOT _TMP_PY_RETURN AND EXISTS "${_TMP_PY_OUTPUT}")
  set(MPI4PY_INCLUDE_FOUND TRUE)
else()
  set(_TMP_PY_OUTPUT)
endif()
set(MPI4PY_INCLUDE_DIR "${_TMP_PY_OUTPUT}" CACHE PATH
  "mpi4py include directories")

set(_TMP_PY_OUTPUT)
set(_TMP_PY_RETURN)
exec_program("${PYTHON_EXECUTABLE}"
  ARGS "-c 'import mpi4py; print(mpi4py.__version__)'"
  OUTPUT_VARIABLE _TMP_PY_OUTPUT
  RETURN_VALUE _TMP_PY_RETURN)
set(MPI4PY_VERSION_FOUND FALSE)
if(NOT _TMP_PY_RETURN)
  set(MPI4PY_VERSION_FOUND TRUE)
else()
  set(_TMP_PY_OUTPUT)
endif()
set(MPI4PY_VERSION "${_TMP_PY_OUTPUT}" CACHE STRING
  "mpi4py version string")

if (NOT ${QUIET})
  message(STATUS "MPI4PY_INCLUDE_DIR=${MPI4PY_INCLUDE_DIR}")
  message(STATUS "MPI4PY_VERSION=${MPI4PY_VERSION}")
endif()

mark_as_advanced(MPI4PY_INCLUDE_DIR MPI4PY_VERSION)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(MPI4PY DEFAULT_MSG
  MPI4PY_INCLUDE_FOUND MPI4PY_VERSION_FOUND)