SET(CMAKE_LEGACY_CYGWIN_WIN32 0)
CMAKE_MINIMUM_REQUIRED(VERSION 2.6.4 FATAL_ERROR)
PROJECT(cg3 CXX C)

# Release or Debug
if(NOT CMAKE_BUILD_TYPE)
	SET(CMAKE_BUILD_TYPE "Release")
endif()

#SET(CMAKE_VERBOSE_MAKEFILE 1)
SET(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMake" ${CMAKE_MODULE_PATH})

if(MSVC)
	SET(CMAKE_CXX_FLAGS "/W4 /GR- /EHsc /MP")
	SET(CMAKE_CXX_FLAGS_RELEASE "/MT /Ox /Ot /GL /GS-")
	SET(CMAKE_EXE_LINKER_FLAGS_RELEASE "/LTCG")
	include_directories("${CMAKE_CURRENT_SOURCE_DIR}/win32")
else(MSVC)
	SET(CMAKE_CXX_FLAGS "-Wall -Wextra -Wno-deprecated")
	SET(CMAKE_CXX_FLAGS_DEBUG "-O0 -g3")
	SET(CMAKE_CXX_FLAGS_RELEASE "-O3")
endif(MSVC)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")

enable_testing()

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src)

install(PROGRAMS "${CMAKE_CURRENT_SOURCE_DIR}/scripts/cg3-autobin.pl" DESTINATION bin)

option(OPT_CPACK "Set to ON to generate CPack configuration files" OFF)
if(OPT_CPACK)
	SET(CPACK_PACKAGE_NAME "CG3")
	SET(CPACK_PACKAGE_CONTACT "Tino Didriksen <mail@tinodidriksen.com>")
	SET(CPACK_PACKAGE_VENDOR "http://tinodidriksen.com/")
	SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "CG-3 is a compiler and applicator for the third edition of the Constraint Grammar formalism.")
	SET(CPACK_DEBIAN_PACKAGE_DEPENDS "libicu48, libgoogle-perftools0|libgoogle-perftools4, libstdc++6")
	FILE(READ "${CMAKE_CURRENT_SOURCE_DIR}/src/version.h" _cg3_VERSION_FILE)
	STRING(REGEX REPLACE ".*CG3_VERSION_MAJOR = ([0-9]+).*" "\\1" _cg3_VERSION_MAJOR "${_cg3_VERSION_FILE}")
	STRING(REGEX REPLACE ".*CG3_VERSION_MINOR = ([0-9]+).*" "\\1" _cg3_VERSION_MINOR "${_cg3_VERSION_FILE}")
	STRING(REGEX REPLACE ".*CG3_VERSION_PATCH = ([0-9]+).*" "\\1" _cg3_VERSION_PATCH "${_cg3_VERSION_FILE}")
	STRING(REGEX REPLACE ".*CG3_REVISION = ([0-9]+).*" "\\1" _cg3_REVISION "${_cg3_VERSION_FILE}")
	SET(CPACK_PACKAGE_VERSION "${_cg3_VERSION_MAJOR}.${_cg3_VERSION_MINOR}.${_cg3_VERSION_PATCH}.${_cg3_REVISION}")

	SET(CPACK_DEBIAN_BUILD_DEPENDS debhelper cmake libicu-dev libboost-dev libgoogle-perftools-dev)
	SET(CPACK_DEBIAN_PACKAGE_SOURCE_COPY svn export --force)
	SET(CPACK_DEBIAN_CHANGELOG "  * Changelog is available at http://visl.sdu.dk/cg3/changelog.txt\n\n")
	EXECUTE_PROCESS(COMMAND lsb_release -is
		OUTPUT_VARIABLE _lsb_distribution OUTPUT_STRIP_TRAILING_WHITESPACE
		RESULT_VARIABLE _lsb_release_failed)
	SET(CPACK_DEBIAN_DISTRIBUTION_NAME ${_lsb_distribution})
	STRING(TOLOWER ${CPACK_DEBIAN_DISTRIBUTION_NAME} CPACK_DEBIAN_DISTRIBUTION_NAME)
	if( ${CPACK_DEBIAN_DISTRIBUTION_NAME} STREQUAL "ubuntu" )
		SET(CPACK_DEBIAN_DISTRIBUTION_RELEASES precise quantal raring)
	endif()
	SET(DPUT_HOST "ppa:tinodidriksen/cg3")
	INCLUDE(CPack)
	INCLUDE(DebSourcePPA)
endif()
