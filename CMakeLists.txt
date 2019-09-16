cmake_minimum_required(VERSION 3.0 FATAL_ERROR)
cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
project(cg3 CXX C)

set(MASTER_PROJECT OFF)
if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
	set(MASTER_PROJECT ON)
endif ()

# Release or Debug
if(MASTER_PROJECT AND NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Release")
endif()

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_MACOSX_RPATH ON)
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMake" ${CMAKE_MODULE_PATH})

file(READ src/version.hpp _cg3_VERSION_FILE)
string(REGEX REPLACE ".*CG3_VERSION_MAJOR = ([0-9]+).*" "\\1" _cg3_VERSION_MAJOR ${_cg3_VERSION_FILE})
string(REGEX REPLACE ".*CG3_VERSION_MINOR = ([0-9]+).*" "\\1" _cg3_VERSION_MINOR ${_cg3_VERSION_FILE})
string(REGEX REPLACE ".*CG3_VERSION_PATCH = ([0-9]+).*" "\\1" _cg3_VERSION_PATCH ${_cg3_VERSION_FILE})
string(REGEX REPLACE ".*CG3_REVISION = ([0-9]+).*" "\\1" REVISION ${_cg3_VERSION_FILE})
set(CPACK_PACKAGE_VERSION_MAJOR ${_cg3_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${_cg3_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${_cg3_VERSION_PATCH})
set(VERSION "${_cg3_VERSION_MAJOR}.${_cg3_VERSION_MINOR}.${_cg3_VERSION_PATCH}.${REVISION}")

include(GNUInstallDirs)

option(BUILD_SHARED_LIBS "Set to OFF to use static library" ON)
option(USE_TCMALLOC "Set to OFF to disable linking against TCMalloc" ON)
option(ENABLE_PYTHON_BINDINGS "Set to ON to build the Python wrapper" OFF)

if(APPLE)
	message(STATUS "Disabling TCMalloc for OS X")
	set(USE_TCMALLOC OFF)
endif()

if(MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /utf-8 /std:c++latest /permissive- /W4 /MP")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2")
	set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /LTCG")
	set(CMAKE_C_FLAGS ${CMAKE_CXX_FLAGS})
	set(CMAKE_C_FLAGS_RELEASE ${CMAKE_CXX_FLAGS_RELEASE})
else()
	set(_FLAGS_COMMON "-Wall -Wextra -Wno-missing-field-initializers -Wno-deprecated -Wno-unused-parameter -fPIC")

	include(CheckCCompilerFlag)
	include(CheckCXXCompilerFlag)

	foreach(flag "-Wno-unused-result" "-flto")
		string(REGEX REPLACE "[^A-Za-z0-9]" "-" _flag ${flag})
		CHECK_CXX_COMPILER_FLAG(${flag} COMPILER_SUPPORTS_${_flag})
		if(COMPILER_SUPPORTS_${_flag})
			set(_FLAGS_COMMON "${_FLAGS_COMMON} ${flag}")
		endif()
	endforeach()
	if(COMPILER_SUPPORTS_flto)
		set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -flto")
	endif()

	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${_FLAGS_COMMON} -fvisibility-inlines-hidden")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -g3")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${_FLAGS_COMMON}")
	set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -O0 -g3")
	set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3")

	# Enable latest possible C standard
	foreach(flag "-std=c2x" "-std=c11" "-std=c1x" "-std=c99")
		string(REGEX REPLACE "[^a-z0-9]" "-" _flag ${flag})
		CHECK_C_COMPILER_FLAG(${flag} COMPILER_SUPPORTS_${_flag})
		if(COMPILER_SUPPORTS_${_flag})
			set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${flag}")
			break()
		endif()
	endforeach()

	# Require latest possible C++ standard
	foreach(flag "-std=c++20" "-std=c++2a" "-std=c++17" "-std=c++1z" "-std=c++14" "-std=c++1y")
		string(REGEX REPLACE "[^a-z0-9]" "-" _flag ${flag})
		CHECK_CXX_COMPILER_FLAG(${flag} COMPILER_SUPPORTS_${_flag})
		if(COMPILER_SUPPORTS_${_flag})
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${flag}")
			set(_ENABLED_CXX ${flag})
			break()
		endif()
	endforeach()
	if(NOT _ENABLED_CXX)
		message(FATAL_ERROR "Could not enable at least C++14 (C++1y) - upgrade your compiler")
	endif()

	# Generate pkg-config file
	set(prefix      ${CMAKE_INSTALL_PREFIX})
	set(exec_prefix ${CMAKE_INSTALL_PREFIX})
	set(libdir      ${CMAKE_INSTALL_LIBDIR})
	set(includedir  ${CMAKE_INSTALL_INCLUDEDIR})
	configure_file(cg3.pc.in cg3.pc @ONLY)
	install(FILES "${CMAKE_CURRENT_BINARY_DIR}/cg3.pc" DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig/")

	install(DIRECTORY manual/man/
		DESTINATION ${CMAKE_INSTALL_MANDIR}/man1
		FILES_MATCHING
		PATTERN "*.1"
		PATTERN ".svn" EXCLUDE)
endif()

include(TestBigEndian)
TEST_BIG_ENDIAN(BIG_ENDIAN)
if(BIG_ENDIAN)
	add_definitions(-DBIG_ENDIAN)
endif()

if(WIN32)
	add_definitions(-DUNICODE -D_UNICODE -D_SECURE_SCL=0 -D_ITERATOR_DEBUG_LEVEL=0 -D_CRT_SECURE_NO_DEPRECATE -DWIN32_LEAN_AND_MEAN -DVC_EXTRALEAN -DNOMINMAX)
	include_directories("include/win32")
else()
	add_definitions(-D_POSIX_C_SOURCE=200112)
	set(POPEN_PLUS_CPP "../include/posix/popen_plus.cpp" "../include/posix/popen_plus.hpp")
	include_directories("include/posix")
endif()

include_directories("include")
include_directories("src")

enable_testing()

add_subdirectory(src)

if(ENABLE_PYTHON_BINDINGS)
	add_subdirectory(python)
endif()

configure_file(scripts/cg3-autobin.pl.in scripts/cg3-autobin.pl @ONLY)
install(PROGRAMS
	"${CMAKE_CURRENT_BINARY_DIR}/scripts/cg3-autobin.pl"
	"${CMAKE_CURRENT_SOURCE_DIR}/scripts/cg-sort"
	"${CMAKE_CURRENT_SOURCE_DIR}/scripts/cg-strictify"
	"${CMAKE_CURRENT_SOURCE_DIR}/scripts/cg-untrace"
	DESTINATION ${CMAKE_INSTALL_BINDIR})

set(ELISPDIR "${CMAKE_INSTALL_DATAROOTDIR}/emacs/site-lisp" CACHE PATH "Where to install Emacs Lisp files")
install(FILES "emacs/cg.el" DESTINATION ${ELISPDIR})
