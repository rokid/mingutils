include(CMakeParseArguments)

# params:
#   HINTS
#   INC_PATH_SUFFIX
#   LIB_PATH_SUFFIX
#   HEADER
#   LIBS
#   RPATH <path>|default
function (findPackage name)

# parse arguments, rfp(rokid find package)
set(options REQUIRED)
set(oneValueArgs HEADER INC_PATH_SUFFIX LIB_PATH_SUFFIX RPATH)
set(multiValueArgs LIBS HINTS)
cmake_parse_arguments(rfp "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

if (rfp_REQUIRED)
set (logprio FATAL_ERROR)
else()
set (logprio STATUS)
endif()
if (NOT rfp_HINTS)
message(${logprio} "findPackage ${name} not specified HINTS")
return()
endif()
if (NOT rfp_LIBS)
message(${logprio} "findPackage ${name} not specified LIBS")
endif()
if (rfp_INC_PATH_SUFFIX)
set(incPathSuffix ${rfp_INC_PATH_SUFFIX})
else()
set(incPathSuffix include)
endif()
if (rfp_LIB_PATH_SUFFIX)
set(libPathSuffix ${rfp_LIB_PATH_SUFFIX})
else()
set(libPathSuffix lib)
endif()


unset(rootDir CACHE)
find_path(rootDir NAMES ${incPathSuffix}/${rfp_HEADER} HINTS ${rfp_HINTS})
if (NOT rootDir)
	if(NOT ${flag} EQUAL QUIETLY)
		message(${logprio} "${name}: Could not find package root dir: header file ${incPathSuffix}/${rfp_HEADER} not found, HINTS ${rfp_HINTS}")
	endif()
	return()
endif()

set(found true)
foreach (lib IN LISTS rfp_LIBS)
	unset(libPathName CACHE)
	find_library(
		libPathName
		NAMES ${lib}
		HINTS ${rootDir}
		PATH_SUFFIXES ${libPathSuffix}
		NO_DEFAULT_PATH
	)

	if (libPathName)
		set(ldflags "${ldflags} -l${lib}")
	else()
		if (NOT ${flag} EQUAL QUIETLY)
		message(${logprio} "Not Found ${name}: ${lib}. HINTS ${rootDir} LIB_PATH_SUFFIX ${libPathSuffix}")
		endif()
		set(found false)
	endif()
endforeach()

if (rfp_RPATH)
if (rfp_RPATH STREQUAL default)
set(rpathFlags "-Wl,-rpath=${rootDir}/${libPathSuffix}")
else()
set(rpathFlags "-Wl,-rpath=${rfp_RPATH}")
endif()
endif()
if (found)
	set(${name}_INCLUDE_DIR ${rootDir}/${incPathSuffix} PARENT_SCOPE)
	if (rpathFlags)
		set(${name}_LIBRARIES "-L${rootDir}/${libPathSuffix} ${ldflags} ${rpathFlags}" PARENT_SCOPE)
	else()
		set(${name}_LIBRARIES "-L${rootDir}/${libPathSuffix} ${ldflags}" PARENT_SCOPE)
	endif()
	message(STATUS "Found ${name}: -L${rootDir}/lib ${ldflags} ${rpathFlags}")
endif()
set (${name}_FOUND ${found} PARENT_SCOPE)
endfunction()
