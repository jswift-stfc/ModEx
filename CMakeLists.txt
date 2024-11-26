cmake_minimum_required(VERSION 3.10)
project(np)
include(CMakePrintHelpers)


set(DESCRIPTION "NeXuS Processor")
set(AUTHOR "Jared Swift and Tristan Youngs")
set(VERSION_MAJOR "0")
set(VERSION_MINOR "2")
set(VERSION_PATCH "0")

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Include application support
include(ExternalProject)

set(CMAKE_MODULE_PATH "${HDF5_DIR}/share/cmake/;${CMAKE_MODULE_PATH}")

# Include Conan Packages
option(CONAN "Use conan to find dependencies" ON)
if(CONAN)
  include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
  conan_basic_setup(TARGETS)
  include_directories(${CONAN_INCLUDE_DIRS_FMT})
else(CONAN)
  find_package(fmt REQUIRED)
  find_package(CLI11 REQUIRED)
endif(CONAN)

option(
  LOCAL_STATIC_GSL
  "Use local GSL installation (specified with GSL_DIR)"
  OFF)

if(LOCAL_STATIC_GSL)
  list(APPEND LINK_LIBS "${GSL_DIR}/lib/libgsl.a" "${GSL_DIR}/lib/libgslcblas.a")
  if(WIN32)
    list(APPEND LINK_LIBS "${GSL_DIR}/lib/libgsl.dll.a" "${GSL_DIR}/lib/libgslcblas.dll.a")
  endif(WIN32)
  include_directories(${GSL_DIR}/include ${GSL_DIR}/include/gsl)
  message("${GSL_DIR}/include")
else(LOCAL_STATIC_GSL)
  find_package(GSL REQUIRED)
  list(APPEND LINK_LIBS "${GSL_LIBRARIES}")
  include_directories("${GSL_INCLUDE_DIRS}")
endif(LOCAL_STATIC_GSL)

option(
  LOCAL_STATIC_HDF5
  "Use local HDF5 installation (specified with HDF5_DIR) built with static ZLIB and SZIP support (so don't search for them)"
  OFF)

# ZLIB Locate installed library, or build as external version (if BUILD_ZLIB)
option(BUILD_ZLIB "Build ZLib as an external project" OFF)
if(BUILD_ZLIB)
  ExternalProject_Add(
    zlib
    PREFIX 3rdparty
    URL https://github.com/madler/zlib/archive/refs/tags/v1.2.11.zip
    BUILD_ALWAYS ON
    INSTALL_DIR "${CMAKE_BINARY_DIR}/3rdparty/"
    BUILD_BYPRODUCTS "${CMAKE_BINARY_DIR}/3rdparty/lib/libzlibstatic.a"
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>)

  add_library(zliblib STATIC IMPORTED GLOBAL)
  set_property(
    TARGET zliblib PROPERTY IMPORTED_LOCATION
                            "${CMAKE_BINARY_DIR}/3rdparty/lib/libzlibstatic.a")
  list(APPEND LINK_LIBS "zliblib")
else(BUILD_ZLIB)
  if(NOT LOCAL_STATIC_HDF5)
    find_package(ZLIB REQUIRED)
    list(APPEND LINK_LIBS "${ZLIB_LIBRARIES}")
  endif(NOT LOCAL_STATIC_HDF5)
endif(BUILD_ZLIB)

# Build HDF5 as an external project
option(BUILD_HDF "Build HDF as an external project" OFF)
if(BUILD_HDF)
  if(WIN32)
    set(HDF_FORTRAN_LINK_FLAGS "-Wl,--allow-multiple-definition")
  endif(WIN32)
  ExternalProject_Add(
    HDF5
    PREFIX 3rdparty
    URL https://github.com/HDFGroup/hdf5/archive/refs/tags/hdf5-1_10_7.zip
    BUILD_ALWAYS ON
    INSTALL_DIR "${CMAKE_BINARY_DIR}/3rdparty/"
    BUILD_BYPRODUCTS
      "${CMAKE_BINARY_DIR}/3rdparty/lib/libhdf5.a"
    CMAKE_ARGS -DHDF5_BUILD_FORTRAN:bool=True
                -DHDF5_INSTALL_MOD_FORTRAN:string=STATIC
                -DHDF5_ENABLE_HDF_SUPPORT:bool=True
                -DHDF5_ENABLE_Z_LIB_SUPPORT:bool=True
                -DBUILD_TESTING:bool=False
                -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>)
  add_library(hdf5main STATIC IMPORTED GLOBAL)
  set_property(
    TARGET hdf5main PROPERTY IMPORTED_LOCATION
                              "${CMAKE_BINARY_DIR}/3rdparty/lib/libhdf5.a")
  set_property(
    TARGET hdf5cpp
    PROPERTY IMPORTED_LOCATION
              "{CMAKE_BINARY_DIR}/3rdparty/lib/libhdf5_cpp.a"
  )
  set_property(
    TARGET hdf5f90cstub
    PROPERTY IMPORTED_LOCATION
              "${CMAKE_BINARY_DIR}/3rdparty/lib/libhdf5_f90cstub.a")
  list(APPEND LINK_LIBS "hdf5f90cstub" "hdf5main" "hdf5cpp")

  # Build SZIP as an external project
  option(BUILD_SZIP "Build SZIP as an external project" OFF)
  if(BUILD_SZIP)
    ExternalProject_Add(
      szip
      PREFIX 3rdparty
      URL https://support.hdfgroup.org/ftp/lib-external/szip/2.1.1/src/szip-2.1.1.tar.gz
      BUILD_ALWAYS ON
      INSTALL_DIR "${CMAKE_BINARY_DIR}/3rdparty/"
      BUILD_BYPRODUCTS "${CMAKE_BINARY_DIR}/3rdparty/lib/libszip-static.a"
      CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
      INSTALL_COMMAND ${CMAKE_COMMAND} --install .
      COMMAND ${CMAKE_COMMAND} -E remove -f
              ${CMAKE_BINARY_DIR}/3rdparty/src/szip-build/szip-config.cmake
      COMMAND ${CMAKE_COMMAND} -E remove -f
              ${CMAKE_BINARY_DIR}/3rdparty/src/szip-build/szip-targets.cmake)
    add_library(sziplib STATIC IMPORTED GLOBAL)
    set_property(
      TARGET sziplib PROPERTY IMPORTED_LOCATION
                              "${CMAKE_BINARY_DIR}/3rdparty/lib/libszip-static.a")
    list(APPEND LINK_LIBS "sziplib")
    add_dependencies(hdf5main HDF5 szip)
  else(BUILD_SZIP)
    if(NOT LOCAL_STATIC_HDF5)
      find_package(SZIP REQUIRED)
      list(APPEND LINK_LIBS "${SZIP_LIBRARIES}")
    endif(NOT LOCAL_STATIC_HDF5)
  endif(BUILD_SZIP)
else(BUILD_HDF)
  if(LOCAL_STATIC_HDF5)
    list(APPEND LINK_LIBS "${HDF5_DIR}/lib/libhdf5_cpp.a"
          "${HDF5_DIR}/lib/libhdf5.a"
          "${HDF5_DIR}/lib/libszip-static.a")
    if(WIN32)
      list(APPEND LINK_LIBS "${HDF5_DIR}/lib/libzlibstatic.a")
    else(WIN32)
      list(APPEND LINK_LIBS "${HDF5_DIR}/lib/libz.a")
    endif(WIN32)
    include_directories(${HDF5_DIR}/include ${HDF5_DIR}/include/static)
  else(LOCAL_STATIC_HDF5)
    find_package(HDF5 REQUIRED COMPONENTS CXX)
    list(APPEND LINK_LIBS "${HDF5_CXX_LIBRARIES}" "${HDF5_LIBRARIES}")
    include_directories(${HDF5_INCLUDE_DIRS})
  endif(LOCAL_STATIC_HDF5)
endif(BUILD_HDF)


# Set necessary include directories
if(BUILD_HDF)
  include_directories(${CMAKE_BINARY_DIR}/3rdparty/include)
endif(BUILD_HDF)

include_directories(
    ${CMAKE_BINARY_DIR}/src
)

cmake_print_variables(LINK_LIBS)

add_subdirectory(src)

add_executable(np np.cpp)
target_include_directories(np PRIVATE ${PROJECT_SOURCE_DIR}/src ${CONAN_INCLUDE_DIRS})
target_link_libraries(np PRIVATE nexusProcess ${LINK_LIBS})

IF(NOT WIN32)
  target_link_libraries(np PRIVATE dl)
ENDIF(NOT WIN32)

# Install targets
install(TARGETS np RUNTIME DESTINATION bin)
