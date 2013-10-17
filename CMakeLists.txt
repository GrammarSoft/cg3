set(CMAKE_LEGACY_CYGWIN_WIN32 0)
cmake_minimum_required(VERSION 2.8.0 FATAL_ERROR)
project(cg3 CXX C)

# Release or Debug
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Release")
endif()

#set(CMAKE_VERBOSE_MAKEFILE 1)
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMake" ${CMAKE_MODULE_PATH})

file(READ "${CMAKE_CURRENT_SOURCE_DIR}/src/version.hpp" _cg3_VERSION_FILE)
string(REGEX REPLACE ".*CG3_VERSION_MAJOR = ([0-9]+).*" "\\1" _cg3_VERSION_MAJOR "${_cg3_VERSION_FILE}")
string(REGEX REPLACE ".*CG3_VERSION_MINOR = ([0-9]+).*" "\\1" _cg3_VERSION_MINOR "${_cg3_VERSION_FILE}")
string(REGEX REPLACE ".*CG3_VERSION_PATCH = ([0-9]+).*" "\\1" _cg3_VERSION_PATCH "${_cg3_VERSION_FILE}")
string(REGEX REPLACE ".*CG3_REVISION = ([0-9]+).*" "\\1" _cg3_REVISION "${_cg3_VERSION_FILE}")
set(VERSION "${_cg3_VERSION_MAJOR}.${_cg3_VERSION_MINOR}.${_cg3_VERSION_PATCH}.${_cg3_REVISION}")

if(MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /GR- /EHsc /MP")
	set(CMAKE_CXX_FLAGS_RELEASE "/MT /Ox /Ot /GL /GS-")
	set(CMAKE_EXE_LINKER_FLAGS_RELEASE "/LTCG")
	set(CMAKE_C_FLAGS ${CMAKE_CXX_FLAGS})
	set(CMAKE_C_FLAGS_RELEASE ${CMAKE_CXX_FLAGS_RELEASE})
	add_definitions(-DUNICODE -D_UNICODE -D_SECURE_SCL=0 -D_ITERATOR_DEBUG_LEVEL=0 -D_CRT_SECURE_NO_DEPRECATE -DWIN32_LEAN_AND_MEAN -DVC_EXTRALEAN -DNOMINMAX)
	include_directories("${CMAKE_CURRENT_SOURCE_DIR}/win32")
else()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-deprecated")
	set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g3")
	set(CMAKE_CXX_FLAGS_RELEASE "-O3")
	set(CMAKE_C_FLAGS ${CMAKE_CXX_FLAGS})
	set(CMAKE_C_FLAGS_DEBUG ${CMAKE_CXX_FLAGS_DEBUG})
	set(CMAKE_C_FLAGS_RELEASE ${CMAKE_CXX_FLAGS_RELEASE})

	# Enable C++11 if possible
	option(OPT_CXX11 "Set to OFF to disable use of C++11" ON)
	if(OPT_CXX11 AND
	((CMAKE_COMPILER_IS_GNUCXX AND NOT ${CMAKE_CXX_COMPILER_VERSION} VERSION_LESS 4.6) OR (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND NOT ${CMAKE_CXX_COMPILER_VERSION} VERSION_LESS 3.1)))
		message(STATUS "Enabling C++11 for ${CMAKE_CXX_COMPILER_ID} v${CMAKE_CXX_COMPILER_VERSION}")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
	endif()

	# Generate pkg-config file
	if(NOT DEFINED CMAKE_INSTALL_LIBDIR)
		set(CMAKE_INSTALL_LIBDIR lib)
	endif()
	set(prefix      ${CMAKE_INSTALL_PREFIX})
	set(exec_prefix ${CMAKE_INSTALL_PREFIX})
	set(libdir      ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR})
	set(includedir  ${CMAKE_INSTALL_PREFIX}/include)
	set(CG3_NAME cg3-${_cg3_VERSION_MAJOR}.${_cg3_VERSION_MINOR})
	configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cg3.pc.in
		${CMAKE_CURRENT_BINARY_DIR}/${CG3_NAME}.pc @ONLY)
	install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${CG3_NAME}.pc" DESTINATION lib/pkgconfig)
endif()

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include/exec-stream")

enable_testing()

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src)

install(PROGRAMS "${CMAKE_CURRENT_SOURCE_DIR}/scripts/cg3-autobin.pl" DESTINATION bin)

option(OPT_CPACK "Set to ON to generate CPack configuration files" OFF)
if(OPT_CPACK)
	set(CPACK_PACKAGE_NAME "CG3")
	set(CPACK_PACKAGE_CONTACT "Tino Didriksen <mail@tinodidriksen.com>")
	set(CPACK_PACKAGE_VENDOR "http://tinodidriksen.com/")
	set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "CG-3 is a compiler and applicator for the third edition of the Constraint Grammar formalism.")
	set(CPACK_DEBIAN_PACKAGE_DEPENDS "libicu48, libgoogle-perftools0|libgoogle-perftools4, libstdc++6")
	set(CPACK_PACKAGE_VERSION "${_cg3_VERSION_MAJOR}.${_cg3_VERSION_MINOR}.${_cg3_VERSION_PATCH}.${_cg3_REVISION}")

	set(CPACK_DEBIAN_BUILD_DEPENDS debhelper cmake libicu-dev libboost-dev libgoogle-perftools-dev)
	set(CPACK_DEBIAN_PACKAGE_SOURCE_COPY svn export --force)
	set(CPACK_DEBIAN_CHANGELOG "  * Changelog is available at http://visl.sdu.dk/cg3/changelog.txt\n\n")
	execute_process(COMMAND lsb_release -is
		OUTPUT_VARIABLE _lsb_distribution OUTPUT_STRIP_TRAILING_WHITESPACE
		RESULT_VARIABLE _lsb_release_failed)
	set(CPACK_DEBIAN_DISTRIBUTION_NAME ${_lsb_distribution})
	string(TOLOWER ${CPACK_DEBIAN_DISTRIBUTION_NAME} CPACK_DEBIAN_DISTRIBUTION_NAME)
	if( ${CPACK_DEBIAN_DISTRIBUTION_NAME} STREQUAL "ubuntu" )
		set(CPACK_DEBIAN_DISTRIBUTION_RELEASES precise quantal raring saucy)
	endif()
	set(DPUT_HOST "ppa:tinodidriksen/cg3")
	include(CPack)
	include(DebSourcePPA)
endif()
