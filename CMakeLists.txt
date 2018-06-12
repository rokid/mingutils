CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
project(rokid_utils)
set(VERSION 0.1)

# CMake policies: enable MACOSX_RPATH by default
if(POLICY CMP0042)
	cmake_policy(SET CMP0042 NEW)
endif()

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" )
  set(COMPILER_IS_CLANG true)
else()
	set(COMPILER_IS_CLANG false)
endif()

option(BUILD_DEBUG "debug or release" OFF)
option(BUILD_DEMO "build demo" OFF)

if (BUILD_DEBUG)
	set (common_cflags "-g -O0")
	set (common_cxxflags "-g -O0")
else()
	set (common_cflags -O3)
	set (common_cxxflags -O3)
	set (common_ldflags -s)
	set (common_arflags -s)
endif()
if (NOT ${COMPILER_IS_CLANG})
	set (common_ldflags "${common_ldflags} -pthread")
endif()
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${common_cflags}")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 ${common_cxxflags}")
set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${common_ldflags}")
set (CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${common_ldflags}")
set (CMAKE_STATIC_LINKER_FLAGS "${CMAKE_STATIC_LINKER_FLAGS} ${common_arflags}")

# rlog src files
file (GLOB_RECURSE rlog_src_files
	src/log/*.h
	src/log/*.c
	include/log/*.h
)

add_library (rlog SHARED
	${rlog_src_files}
)
target_include_directories (rlog PRIVATE
	include/log
)
target_link_libraries (rlog
)

install (TARGETS rlog
	LIBRARY DESTINATION lib
)
install (DIRECTORY include/log
	DESTINATION include)

# target 'misc'
file(GLOB misc_src_files
	include/misc/*.h
	src/misc/*.cc
)
add_library(misc STATIC
	${misc_src_files}
)
target_include_directories(misc PRIVATE
	include/misc
)
target_compile_options(misc PRIVATE
	-fPIC
)
install(TARGETS misc
	ARCHIVE DESTINATION lib
)
install(DIRECTORY include/misc
	DESTINATION include)


# build demo
if (BUILD_DEMO)
set(clargs_demo_src_files
	demo/misc/clargs_demo.c
)
add_executable (clargs_demo ${clargs_demo_src_files})
target_include_directories (clargs_demo PRIVATE
	include/misc
)
target_link_libraries (clargs_demo
	misc
)

set(vq_demo_src_files
	demo/misc/vq_demo.cc
)
add_executable (vq_demo ${vq_demo_src_files})
target_include_directories (vq_demo PRIVATE
	include/misc
)
target_link_libraries (vq_demo
	misc
)
endif(BUILD_DEMO)
