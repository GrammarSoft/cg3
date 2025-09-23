find_package(RapidJSON REQUIRED)
include_directories(${RapidJSON_INCLUDE_DIRS})

if(EMSCRIPTEN)
	message(STATUS "Detected EMSCRIPTEN: skipping local library discovery and using Emscripten ports")
	# Emscripten ports flags for Boost, ICU, SQLite3:
	set(USE_FLAGS "--use-port=boost_headers --use-port=icu --use-port=sqlite3")

	# Append to CXX flags, exe linker flags, and shared linker flags.
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${USE_FLAGS}")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${USE_FLAGS}")
	set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${USE_FLAGS}")

	# No threading for Emscripten build
	set(CMAKE_THREAD_LIBS_INIT "")
else()
	# Threading, for exec-stream
	# There's some weird bug/interaction with recent Apple toolchains and this, so disable for now.
	if(NOT APPLE)
		find_package(Threads REQUIRED)
	else()
		# See https://trac.macports.org/ticket/71642
		cmake_policy(SET CMP0167 OLD)
	endif()

	# Boost
	find_path(Boost_LOCAL NAMES boost/config.hpp PATHS "../include" NO_DEFAULT_PATH)
	if(Boost_LOCAL)
		set(BOOST_ROOT "../include")
		find_package(Boost 1.62.0)
	endif()
	if(NOT Boost_FOUND)
		unset(BOOST_ROOT)
		message(STATUS "Did not find Boost locally; trying globally...")
		message(STATUS "If this doesn't work, run ./get-boost.sh and try again.")
		find_package(Boost 1.62.0 REQUIRED)
	endif()
	include_directories(${Boost_INCLUDE_DIRS})
	link_directories(${Boost_LIBRARY_DIRS})

	# ICU
	find_package(ICU 59.0 COMPONENTS i18n io uc data REQUIRED)
	if(ICU_FOUND)
		include_directories(${ICU_INCLUDE_DIRS})
		message(STATUS "Found ICU version ${ICU_VERSION}")
	endif()
	link_directories(${ICU_LIBRARY_DIRS})

	if(ENABLE_PROFILING)
		if(VCPKG_TOOLCHAIN)
			find_path(SQLITE3_INCLUDE_DIRS sqlite3.h REQUIRED)
			find_library(SQLITE3_LIBRARIES sqlite3 REQUIRED)
		else()
			find_package(PkgConfig REQUIRED)
			pkg_search_module(SQLITE3 REQUIRED sqlite3)
		endif()
		include_directories(${SQLITE3_INCLUDE_DIRS})
		link_directories(${SQLITE3_LIBRARY_DIRS})
	endif()
endif()

macro(cg3_link target)
	if(EMSCRIPTEN)
		target_link_options(${target} PRIVATE
			-sEXPORTED_RUNTIME_METHODS=ccall,cwrap,FS
			# Generated with: pcregrep -o 'cg3_\S+\(' src/cg3.h | perl -wpne "s/\(//g; s/^/'_/; s/\n',/;"
			-sEXPORTED_FUNCTIONS=['_cg3_init','_cg3_init','_cg3_cleanup','_cg3_cleanup','_cg3_grammar_load','_cg3_grammar_load_buffer','_cg3_grammar_free','_cg3_detect_sformat','_cg3_detect_sformat_buffer','_cg3_sconverter_create','_cg3_sconverter_free','_cg3_sconverter_run_fns','_cg3_applicator_create','_cg3_applicator_setflags','_cg3_applicator_setoption','_cg3_applicator_setoption','_cg3_applicator_setoption','_cg3_applicator_free','_cg3_run_grammar_on_text','_cg3_run_grammar_on_text_fns','_cg3_mwesplitapplicator_create','_cg3_sentence_new','_cg3_sentence_copy','_cg3_sentence_runrules','_cg3_sentence_addcohort','_cg3_sentence_numcohorts','_cg3_sentence_getcohort','_cg3_sentence_free','_cg3_cohort_create','_cg3_cohort_setwordform','_cg3_cohort_getwordform','_cg3_cohort_getid','_cg3_cohort_setdependency','_cg3_cohort_getdependency','_cg3_cohort_addreading','_cg3_cohort_numreadings','_cg3_cohort_getreading','_cg3_cohort_free','_cg3_reading_create','_cg3_reading_addtag','_cg3_reading_numtags','_cg3_reading_gettag','_cg3_reading_numtraces','_cg3_reading_gettrace','_cg3_reading_free','_cg3_subreading_create','_cg3_reading_setsubreading','_cg3_reading_numsubreadings','_cg3_reading_getsubreading','_cg3_subreading_free','_cg3_tag_create_u','_cg3_tag_create_u8','_cg3_tag_create_u16','_cg3_tag_create_u32','_cg3_tag_create_w','_cg3_tag_gettext_u','_cg3_tag_gettext_u8','_cg3_tag_gettext_u16','_cg3_tag_gettext_u32','_cg3_tag_gettext_w','_cg3_cohort_numdelreadings','_cg3_cohort_getdelreading','_cg3_reading_gettrace_ruletype']
			)
	else()
		target_link_libraries(${target}
			${CMAKE_THREAD_LIBS_INIT}
			${STDFS_LIB}
			${Boost_LIBRARIES}
			${ICU_LIBRARIES}
			${SQLITE3_LIBRARIES}
			)
	endif()
endmacro()

set(LIBCG3_SOURCES
	AST.hpp
	ApertiumApplicator.cpp
	ApertiumApplicator.hpp
	BinaryApplicator.cpp
	BinaryApplicator.hpp
	BinaryGrammar.cpp
	BinaryGrammar.hpp
	BinaryGrammar_read.cpp
	BinaryGrammar_read_10043.cpp
	BinaryGrammar_write.cpp
	Cohort.cpp
	Cohort.hpp
	CohortIterator.cpp
	CohortIterator.hpp
	ContextualTest.cpp
	ContextualTest.hpp
	FSTApplicator.cpp
	FSTApplicator.hpp
	FormatConverter.cpp
	FormatConverter.hpp
	Grammar.cpp
	Grammar.hpp
	GrammarApplicator.cpp
	GrammarApplicator.hpp
	GrammarApplicator_context.cpp
	GrammarApplicator_matchSet.cpp
	GrammarApplicator_reflow.cpp
	GrammarApplicator_runContextualTest.cpp
	GrammarApplicator_runGrammar.cpp
	GrammarApplicator_runRules.cpp
	GrammarWriter.cpp
	GrammarWriter.hpp
	IGrammarParser.hpp
	JsonlApplicator.cpp
	JsonlApplicator.hpp
	MathParser.hpp
	MatxinApplicator.cpp
	MatxinApplicator.hpp
	MweSplitApplicator.cpp
	MweSplitApplicator.hpp
	NicelineApplicator.cpp
	NicelineApplicator.hpp
	PlaintextApplicator.cpp
	PlaintextApplicator.hpp
	Profiler.cpp
	Profiler.hpp
	Reading.cpp
	Reading.hpp
	Relabeller.cpp
	Relabeller.hpp
	Rule.cpp
	Rule.hpp
	Set.cpp
	Set.hpp
	SingleWindow.cpp
	SingleWindow.hpp
	Strings.cpp
	Strings.hpp
	Tag.cpp
	Tag.hpp
	TagTrie.hpp
	TextualParser.cpp
	TextualParser.hpp
	Window.cpp
	Window.hpp
	bloomish.hpp
	cg3.h
	flat_unordered_map.hpp
	flat_unordered_set.hpp
	inlines.hpp
	interval_vector.hpp
	options.cpp
	options.hpp
	options_parser.hpp
	parser_helpers.hpp
	pool.hpp
	process.hpp
	scoped_stack.hpp
	sorted_vector.hpp
	stdafx.hpp
	streambuf.hpp
	uextras.cpp
	uextras.hpp
	version.hpp
	${POPEN_PLUS_CPP}
	)
if(WIN32 OR EMSCRIPTEN)
	set(LIBCG3_SOURCES
		"${CMAKE_SOURCE_DIR}/include/wordexp/wordexp.cpp"
		"${CMAKE_SOURCE_DIR}/include/wordexp/wordexp.h"
		${LIBCG3_SOURCES}
		)
endif()
if(WIN32)
	set(LIBCG3_SOURCES
		"${CMAKE_SOURCE_DIR}/include/getopt/getopt.cpp"
		"${CMAKE_SOURCE_DIR}/include/getopt/getopt.h"
		${LIBCG3_SOURCES}
		)
endif()

add_library(cg3 libcg3.cpp ${LIBCG3_SOURCES})
set_target_properties(cg3 PROPERTIES SOVERSION ${_cg3_VERSION_MAJOR})
cg3_link(cg3)
set(LINKLIB cg3)

if(NOT EMSCRIPTEN)
	add_executable(cg-comp cg-comp.cpp)
	cg3_link(cg-comp)
	target_link_libraries(cg-comp ${LINKLIB})

	add_executable(cg-proc cg-proc.cpp)
	cg3_link(cg-proc)
	target_link_libraries(cg-proc ${LINKLIB})

	add_executable(cg-relabel cg-relabel.cpp)
	cg3_link(cg-relabel)
	target_link_libraries(cg-relabel ${LINKLIB})

	add_executable(cg-conv
		cg-conv.cpp
		options_conv.hpp
		)
	cg3_link(cg-conv)
	target_link_libraries(cg-conv ${LINKLIB})

	add_executable(cg-mwesplit cg-mwesplit.cpp)
	cg3_link(cg-mwesplit)
	target_link_libraries(cg-mwesplit ${LINKLIB})

	add_executable(cg-annotate cg-annotate.cpp)
	cg3_link(cg-annotate)
	target_link_libraries(cg-annotate ${LINKLIB})

	add_executable(cg-merge-annotations cg-merge-annotations.cpp)
	cg3_link(cg-merge-annotations)
	target_link_libraries(cg-merge-annotations ${LINKLIB})

	add_executable(vislcg3
		main.cpp
		)
	cg3_link(vislcg3)
	target_link_libraries(vislcg3 ${LINKLIB})

	add_executable(test_libcg3 test_libcg3.c)
	target_link_libraries(test_libcg3 ${LINKLIB})
endif()

if(APPLE AND NOT EMSCRIPTEN)
	foreach(t cg-conv cg-comp cg-proc cg-relabel cg-mwesplit cg-annotate cg-merge-annotations vislcg3)
		set("_file_${t}" $<TARGET_FILE:${t}>)
		add_custom_command(
			TARGET ${t}
			POST_BUILD
			COMMAND install_name_tool -change libcg3.0.dylib @rpath/libcg3.0.dylib ${_file_${t}}
			COMMAND install_name_tool -add_rpath @executable_path/ ${_file_${t}}
			COMMAND install_name_tool -add_rpath @executable_path/../${CMAKE_INSTALL_LIBDIR} ${_file_${t}}
			COMMAND install_name_tool -add_rpath @loader_path/../${CMAKE_INSTALL_LIBDIR} ${_file_${t}}
			COMMENT "Adding @rpath to ${t}"
			VERBATIM
		)
	endforeach()
endif()

if(NOT EMSCRIPTEN)
	add_test(t_libcg3 test_libcg3 "${CMAKE_CURRENT_SOURCE_DIR}/../test/T_Select/grammar.cg3")

	install(TARGETS cg3 ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
	install(FILES cg3.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
	install(TARGETS cg-comp cg-proc cg-relabel cg-mwesplit cg-conv cg-annotate cg-merge-annotations vislcg3 RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

if(EMSCRIPTEN)
	add_executable(libcg3_wasm libcg3.cpp ${LIBCG3_SOURCES})
	cg3_link(libcg3_wasm)
	set_target_properties(libcg3_wasm PROPERTIES OUTPUT_NAME "libcg3")
endif()
