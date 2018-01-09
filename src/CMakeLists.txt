# Threading, for exec-stream
# There's some weird bug/interaction with recent Apple toolchains and this, so disable for now.
if(NOT APPLE)
	find_package(Threads REQUIRED)
endif()

# Boost
find_path(Boost_LOCAL NAMES boost/config.hpp PATHS "${CMAKE_CURRENT_SOURCE_DIR}/../include" NO_DEFAULT_PATH)
if(Boost_LOCAL)
	set(BOOST_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../include")
	find_package(Boost 1.48.0)
endif()
if(NOT Boost_FOUND)
	unset(BOOST_ROOT)
	message(STATUS "Did not find Boost locally; trying globally...")
	message(STATUS "If this doesn't work, run ./get-boost.sh and try again.")
	find_package(Boost 1.48.0 REQUIRED)
endif()
include_directories(${Boost_INCLUDE_DIRS})
link_directories(${Boost_LIBRARY_DIRS})

# ICU
find_package(ICU REQUIRED)
if(ICU_FOUND)
	if(ICU_VERSION VERSION_LESS "4.2")
		message(FATAL_ERROR "ICU is too old - ${ICU_VERSION} was found, but CG-3 requires 4.2 or newer")
	endif()
	include_directories(${ICU_INCLUDE_DIRS})
	message(STATUS "Found ICU version ${ICU_VERSION}")
endif()

if(NOT CMAKE_BUILD_TYPE STREQUAL Debug AND USE_TCMALLOC)
	find_library(GOOGLE_TCMALLOC_LIB tcmalloc)
	if(GOOGLE_TCMALLOC_LIB)
		message(STATUS "Found tcmalloc: ${GOOGLE_TCMALLOC_LIB}")
		set(GOOGLE_TCMALLOC_ENABLED 1)
	else()
		message(STATUS "Can't find tcmalloc - skipping")
	endif()
endif()

macro(cg3_link target)
	if(GOOGLE_TCMALLOC_ENABLED)
		target_link_libraries(${target} ${GOOGLE_TCMALLOC_LIB})
	endif()

	if(WIN32)
		target_link_libraries(${target} "wsock32.lib")
	endif()

	target_link_libraries(${target} ${CMAKE_THREAD_LIBS_INIT})
	target_link_libraries(${target} ${Boost_LIBRARIES})
	target_link_libraries(${target} ${ICU_LIBRARIES} ${ICU_IO_LIBRARIES} ${ICU_I18N_LIBRARIES})
endmacro()

set(LIBCG3_HEADERS
	AST.hpp
	BinaryGrammar.hpp
	Cohort.hpp
	CohortIterator.hpp
	ContextualTest.hpp
	Grammar.hpp
	GrammarApplicator.hpp
	GrammarWriter.hpp
	Relabeller.hpp
	MweSplitApplicator.hpp
	IGrammarParser.hpp
	Reading.hpp
	Rule.hpp
	Set.hpp
	SingleWindow.hpp
	Strings.hpp
	Tag.hpp
	TagTrie.hpp
	TextualParser.hpp
	Window.hpp
	bloomish.hpp
	cg3.h
	flat_unordered_map.hpp
	flat_unordered_set.hpp
	inlines.hpp
	interval_vector.hpp
	parser_helpers.hpp
	process.hpp
	scoped_stack.hpp
	sorted_vector.hpp
	stdafx.hpp
	streambuf.hpp
	uextras.hpp
	version.hpp
	)
set(LIBCG3_SOURCES
	BinaryGrammar.cpp
	BinaryGrammar_read.cpp
	BinaryGrammar_read_10043.cpp
	BinaryGrammar_write.cpp
	Cohort.cpp
	CohortIterator.cpp
	ContextualTest.cpp
	Grammar.cpp
	GrammarApplicator.cpp
	GrammarApplicator_matchSet.cpp
	GrammarApplicator_reflow.cpp
	GrammarApplicator_runContextualTest.cpp
	GrammarApplicator_runGrammar.cpp
	GrammarApplicator_runRules.cpp
	GrammarWriter.cpp
	Relabeller.cpp
	MweSplitApplicator.cpp
	Reading.cpp
	Rule.cpp
	Set.cpp
	SingleWindow.cpp
	Strings.cpp
	Tag.cpp
	TextualParser.cpp
	Window.cpp
	uextras.cpp
	${POPEN_PLUS_C}
	${LIBCG3_HEADERS}
	)
if(MSVC)
	set(LIBCG3_SOURCES
		"${CMAKE_SOURCE_DIR}/include/win32/getopt.c"
		"${CMAKE_SOURCE_DIR}/include/win32/getopt.h"
		${LIBCG3_SOURCES}
		)
endif()

if(MSVC OR INSTALL_STATIC)
	add_library(libcg3 STATIC libcg3.cpp ${LIBCG3_SOURCES})
	set_target_properties(libcg3 PROPERTIES OUTPUT_NAME "cg3")
	set_target_properties(libcg3 PROPERTIES PREFIX "lib")
	if(MSVC AND NOT CMAKE_BUILD_TYPE STREQUAL Debug)
		set_target_properties(libcg3 PROPERTIES STATIC_LIBRARY_FLAGS "/LTCG")
	endif()
	cg3_link(libcg3)
	set(LINKLIB libcg3)
endif()

if(NOT MSVC)
	add_library(libcg3-shared SHARED libcg3.cpp ${LIBCG3_SOURCES})
	set_target_properties(libcg3-shared PROPERTIES SOVERSION ${_cg3_VERSION_MAJOR})
	set_target_properties(libcg3-shared PROPERTIES OUTPUT_NAME "cg3")
	set_target_properties(libcg3-shared PROPERTIES PREFIX "lib")
	cg3_link(libcg3-shared)
	set(LINKLIB libcg3-shared)
endif()

add_executable(cg-comp cg_comp.cpp)
cg3_link(cg-comp)
target_link_libraries(cg-comp ${LINKLIB})

add_executable(cg-proc
	cg_proc.cpp
	ApertiumApplicator.cpp
	ApertiumApplicator.hpp
	MatxinApplicator.cpp
	MatxinApplicator.hpp
	)
cg3_link(cg-proc)
target_link_libraries(cg-proc ${LINKLIB})

add_executable(cg-relabel cg-relabel.cpp)
cg3_link(cg-relabel)
target_link_libraries(cg-relabel ${LINKLIB})

add_executable(cg-conv
	cg_conv.cpp
	ApertiumApplicator.cpp
	ApertiumApplicator.hpp
	MatxinApplicator.cpp
	MatxinApplicator.hpp
	FormatConverter.cpp
	FormatConverter.hpp
	FSTApplicator.cpp
	FSTApplicator.hpp
	NicelineApplicator.cpp
	NicelineApplicator.hpp
	PlaintextApplicator.cpp
	PlaintextApplicator.hpp
	options_conv.hpp
	)
cg3_link(cg-conv)
target_link_libraries(cg-conv ${LINKLIB})

add_executable(cg-mwesplit cg-mwesplit.cpp)
cg3_link(cg-mwesplit)
target_link_libraries(cg-mwesplit ${LINKLIB})

add_executable(vislcg3 main.cpp options.hpp)
cg3_link(vislcg3)
target_link_libraries(vislcg3 ${LINKLIB})

add_executable(test_libcg3 test_libcg3.c)
target_link_libraries(test_libcg3 ${LINKLIB})

if(APPLE)
	foreach(t cg-conv cg-comp cg-proc cg-relabel cg-mwesplit vislcg3)
		set("_file_${t}" $<TARGET_FILE:${t}>)
		add_custom_command(
			TARGET ${t}
			POST_BUILD
			COMMAND install_name_tool -change libcg3.0.dylib @rpath/libcg3.0.dylib ${_file_${t}}
			COMMAND install_name_tool -add_rpath @executable_path/ ${_file_${t}}
			COMMAND install_name_tool -add_rpath @executable_path/../${CG_LIBDIR} ${_file_${t}}
			COMMAND install_name_tool -add_rpath @loader_path/../${CG_LIBDIR} ${_file_${t}}
			COMMENT "Adding @rpath to ${t}"
			VERBATIM
		)
	endforeach()
endif()

add_test(t_libcg3 test_libcg3 "${CMAKE_CURRENT_SOURCE_DIR}/../test/T_Select/grammar.cg3")

if(INSTALL_STATIC)
	install(TARGETS libcg3 ARCHIVE DESTINATION "${CG_LIBDIR}/${CMAKE_LIBRARY_ARCHITECTURE}")
endif()

if(NOT MSVC)
	install(TARGETS libcg3-shared ARCHIVE DESTINATION "${CG_LIBDIR}/${CMAKE_LIBRARY_ARCHITECTURE}" LIBRARY DESTINATION "${CG_LIBDIR}/${CMAKE_LIBRARY_ARCHITECTURE}" RUNTIME DESTINATION bin)
	install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/cg3.h" DESTINATION include)
endif()
install(TARGETS cg-comp cg-proc cg-relabel cg-mwesplit cg-conv vislcg3 RUNTIME DESTINATION bin)
