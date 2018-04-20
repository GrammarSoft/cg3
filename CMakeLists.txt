set(CMAKE_LEGACY_CYGWIN_WIN32 0)
cmake_minimum_required(VERSION 2.8.9 FATAL_ERROR)
project(cg3 CXX C)

# Release or Debug
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Release")
endif()

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_MACOSX_RPATH ON)
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMake" ${CMAKE_MODULE_PATH})

file(READ "${CMAKE_CURRENT_SOURCE_DIR}/src/version.hpp" _cg3_VERSION_FILE)
string(REGEX REPLACE ".*CG3_VERSION_MAJOR = ([0-9]+).*" "\\1" _cg3_VERSION_MAJOR ${_cg3_VERSION_FILE})
string(REGEX REPLACE ".*CG3_VERSION_MINOR = ([0-9]+).*" "\\1" _cg3_VERSION_MINOR ${_cg3_VERSION_FILE})
string(REGEX REPLACE ".*CG3_VERSION_PATCH = ([0-9]+).*" "\\1" _cg3_VERSION_PATCH ${_cg3_VERSION_FILE})
string(REGEX REPLACE ".*CG3_REVISION = ([0-9]+).*" "\\1" REVISION ${_cg3_VERSION_FILE})
set(VERSION "${_cg3_VERSION_MAJOR}.${_cg3_VERSION_MINOR}.${_cg3_VERSION_PATCH}.${REVISION}")

# Because Fedora / CentOS / RHEL ...
set(CG_LIBDIR "lib")
if(LIB_INSTALL_DIR)
	string(REGEX REPLACE "^${CMAKE_INSTALL_PREFIX}/" "" CG_LIBDIR ${LIB_INSTALL_DIR})
elseif(LIB_SUFFIX)
	set(CG_LIBDIR "${CG_LIBDIR}${LIB_SUFFIX}")
endif()

option(INSTALL_STATIC "Set to ON to install the static library" OFF)
option(USE_TCMALLOC "Set to OFF to disable linking against TCMalloc" ON)
if(APPLE)
	message(STATUS "Disabling TCMalloc for OS X")
	set(USE_TCMALLOC OFF)
endif()

if(MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /utf-8 /std:c++latest /W4 /EHsc /MP")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT /Ox /Ot /GL /GS-")
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

	# Enable latest possible C++ standard
	foreach(flag "-std=c++20" "-std=c++2a" "-std=c++17" "-std=c++1z" "-std=c++14" "-std=c++1y" "-std=c++11" "-std=c++0x")
		string(REGEX REPLACE "[^a-z0-9]" "-" _flag ${flag})
		CHECK_CXX_COMPILER_FLAG(${flag} COMPILER_SUPPORTS_${_flag})
		if(COMPILER_SUPPORTS_${_flag})
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${flag}")
			break()
		endif()
	endforeach()

	# Generate pkg-config file
	set(prefix      ${CMAKE_INSTALL_PREFIX})
	set(exec_prefix ${CMAKE_INSTALL_PREFIX})
	set(libdir      ${CMAKE_INSTALL_PREFIX}/${CG_LIBDIR}/${CMAKE_LIBRARY_ARCHITECTURE})
	set(includedir  ${CMAKE_INSTALL_PREFIX}/include)
	configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cg3.pc.in
		${CMAKE_CURRENT_BINARY_DIR}/cg3.pc @ONLY)
	install(FILES "${CMAKE_CURRENT_BINARY_DIR}/cg3.pc" DESTINATION "${CG_LIBDIR}/${CMAKE_LIBRARY_ARCHITECTURE}/pkgconfig")

	install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/manual/man/"
		DESTINATION share/man/man1
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
	include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include/win32")
else()
	add_definitions(-D_POSIX_C_SOURCE=200112)
	set(POPEN_PLUS_C "${CMAKE_CURRENT_SOURCE_DIR}/include/posix/popen_plus.c" "${CMAKE_CURRENT_SOURCE_DIR}/include/posix/popen_plus.h")
	include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include/posix")
endif()

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")

enable_testing()

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/scripts/cg3-autobin.pl.in ${CMAKE_CURRENT_BINARY_DIR}/scripts/cg3-autobin.pl @ONLY)
install(PROGRAMS "${CMAKE_CURRENT_BINARY_DIR}/scripts/cg3-autobin.pl" DESTINATION bin)
install(PROGRAMS "${CMAKE_CURRENT_SOURCE_DIR}/scripts/cg-strictify" DESTINATION bin)
install(PROGRAMS "${CMAKE_CURRENT_SOURCE_DIR}/scripts/cg-sort" DESTINATION bin)

set(ELISPDIR "share/emacs/site-lisp" CACHE PATH "Where to install Emacs Lisp files")
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/emacs/cg.el" DESTINATION ${ELISPDIR})
