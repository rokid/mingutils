include(CMakeParseArguments)

function (findPackage name flag)

# parse arguments, rfp(rokid find package)
set(options OPTIONAL)
set(oneValueArgs HEADER INC_SUFFIX LIB_PATH_SUFFIX)
set(multiValueArgs LIBS HINTS)
cmake_parse_arguments(rfp "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

if (${flag} STREQUAL REQUIRED)
	set (logprio FATAL_ERROR)
else()
	set (logprio STATUS)
endif()
if (rfp_LIB_PATH_SUFFIX)
	set(libPathSuffix ${rfp_LIB_PATH_SUFFIX})
else()
	set(libPathSuffix lib)
endif()


unset(rootDir CACHE)
find_path(rootDir NAMES ${rfp_HEADER} HINTS ${rfp_HINTS})
if (NOT rootDir)
	if(NOT ${flag} EQUAL QUIETLY)
		message(${logprio} "${name}: Could not find package root dir. NAMES ${rfp_HEADER}, HINTS ${rfp_HINTS}")
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

if (found)
	if (rfp_INC_SUFFIX)
		set(${name}_INCLUDE_DIR ${rootDir}/${rfp_INC_SUFFIX} PARENT_SCOPE)
	else()
		set(${name}_INCLUDE_DIR ${rootDir} PARENT_SCOPE)
	endif()
	set (${name}_LIBRARIES "-L${rootDir}/lib ${ldflags}" PARENT_SCOPE)
	message(STATUS "Found ${name}: -L${rootDir}/lib ${ldflags}")
endif()
set (${name}_FOUND ${found} PARENT_SCOPE)
endfunction()
