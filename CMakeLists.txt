cmake_minimum_required(VERSION 2.8)


option(HACK_RETAIN_VIEWER "include opengl-based viewer" FALSE)
if (HACK_RETAIN_VIEWER) 
  message(STATUS "sparing opengl")
else()
  add_definitions(-DNO_VIEWER=1)
endif()

option(HACK_EMSCRIPTEN "be emscripten-friendly" FALSE)

# we replace BOOST_FOREACH with native for
add_definitions(-std=c++0x)
# add_definitions(-std=c++11)

add_definitions(-fno-exceptions)
add_definitions(-fno-rtti)

option(CREATE_SHARED_LIBRARIES "Compile shared libraries rather than static" FALSE)
if (CREATE_SHARED_LIBRARIES)
  set(BUILD_SHARED_LIBS ON)
endif ()

option(USE_SDL "use sdl (emscripten-friendly)" FALSE)

if (USE_SDL)
  add_definitions(-DUSE_SDL)
  link_libraries(SDL)
endif ()

if (HACK_EMSCRIPTEN)
  add_definitions(-DNO_FILE_OUTPUT)
  add_definitions(-m32)
endif ()






SET(WITH_CYCLES_TEST ON)

set(CYCLES_INSTALL_PATH "scripts/addons/cycles")
set(WITH_CYCLES_BLENDER OFF)

macro(delayed_install)
endmacro()

# External Libraries

include(cmake/external_libs.cmake)

# Build Flags

if(WITH_RAYOPTIMIZATION AND SUPPORT_SSE_BUILD)
	set(WITH_CYCLES_OPTIMIZED_KERNEL ON)
endif()

# for OSL, not needed yet
# set(RTTI_DISABLE_FLAGS "-fno-rtti -DBOOST_NO_RTTI -DBOOST_NO_TYPEID")
# set(RTTI_DISABLE_FLAGS "/GR- -DBOOST_NO_RTTI -DBOOST_NO_TYPEID")

# Definitions and Includes

# add_definitions(${BOOST_DEFINITIONS} ${OPENIMAGEIO_DEFINITIONS})

add_definitions(-DCCL_NAMESPACE_BEGIN=namespace\ ccl\ {)
add_definitions(-DCCL_NAMESPACE_END=})

if(WITH_CYCLES_OPTIMIZED_KERNEL)
	add_definitions(-DWITH_OPTIMIZED_KERNEL)
endif()

if(WITH_CYCLES_NETWORK)
	add_definitions(-DWITH_NETWORK)
endif()

if(WITH_CYCLES_OSL)
	add_definitions(-DWITH_OSL)
endif()

if(WITH_CYCLES_PARTIO)
	add_definitions(-DWITH_PARTIO)
endif()

if(WITH_CYCLES_CUDA_BINARIES)
	add_definitions(-DWITH_CUDA_BINARIES)
endif()

# add_definitions(-DWITH_OPENCL)
# add_definitions(-DWITH_CUDA)
# add_definitions(-DWITH_MULTI)

#include_directories(
#	${BOOST_INCLUDE_DIR}
#	${OPENIMAGEIO_INCLUDE_DIRS}
#	${OPENIMAGEIO_INCLUDE_DIRS}/OpenImageIO)

# Subdirectories

if(WITH_CYCLES_BLENDER)
	add_definitions(-DBLENDER_PLUGIN)
	add_subdirectory(blender)
endif(WITH_CYCLES_BLENDER)

add_subdirectory(app)
add_subdirectory(bvh)
add_subdirectory(device)
add_subdirectory(doc)
add_subdirectory(kernel)
add_subdirectory(render)
add_subdirectory(subd)
add_subdirectory(util)

