# Boost
SET(Boost_ADDITIONAL_VERSIONS "1.42" "1.42.0" "1.43" "1.43.0" "1.44" "1.44.0" "1.45" "1.45.0" "1.46" "1.46.0" "1.46.1")
find_package(Boost 1.36.0)
if(Boost_FOUND)
	include_directories(${Boost_INCLUDE_DIRS})
	link_directories(${Boost_LIBRARY_DIRS})
	add_definitions(-DHAVE_BOOST=1)
	message(STATUS "Found Boost version ${Boost_VERSION}")
endif()

# ICU
find_package(ICU REQUIRED)
if (ICU_FOUND)
	if (ICU_VERSION VERSION_LESS "3.6")
		unset(ICU_FOUND CACHE)
		unset(ICU_INCLUDE_DIRS CACHE)
		unset(ICU_LIBRARIES CACHE)
		message(FATAL_ERROR "ICU is too old - ${ICU_VERSION} was found, but CG-3 requires 3.6 or newer")
	endif ()
	include_directories(${ICU_INCLUDE_DIRS})
	message(STATUS "Found ICU version ${ICU_VERSION}")
endif(ICU_FOUND)

if (NOT CMAKE_BUILD_TYPE STREQUAL Debug)
	FIND_LIBRARY(GOOGLE_TCMALLOC_LIB tcmalloc)
	if (GOOGLE_TCMALLOC_LIB)
		message(STATUS "Found tcmalloc: ${GOOGLE_TCMALLOC_LIB}")
		set(GOOGLE_TCMALLOC_ENABLED 1)
	else()
		message(STATUS "Can't find tcmalloc - skipping")
	endif()
endif()

macro(cg3_link target)
	if (GOOGLE_TCMALLOC_ENABLED)
		target_link_libraries(${target} ${GOOGLE_TCMALLOC_LIB})
	endif()

	if(MSVC)
		target_link_libraries(${target} "wsock32.lib")
	endif(MSVC)

	target_link_libraries(${target} ${Boost_LIBRARIES})
	target_link_libraries(${target} ${ICU_LIBRARIES} ${ICU_IO_LIBRARIES} ${ICU_I18N_LIBRARIES})
endmacro()

SET(CXX_SHARED_SOURCES Anchor.cpp BinaryGrammar.cpp CompositeTag.cpp ContextualTest.cpp Grammar.cpp Rule.cpp Set.cpp Strings.cpp Tag.cpp TextualParser.cpp uextras.cpp BinaryGrammar_read.cpp BinaryGrammar_write.cpp)
if(MSVC)
	SET(CXX_SHARED_SOURCES ../win32/libgen.c ../win32/getopt.c ${CXX_SHARED_SOURCES})
endif(MSVC)

SET(CXX_SOURCES GrammarApplicator_runGrammar.cpp GrammarApplicator.cpp GrammarApplicator_matchSet.cpp GrammarApplicator_reflow.cpp GrammarApplicator_runContextualTest.cpp GrammarApplicator_runRules.cpp GrammarWriter.cpp icu_uoptions.cpp SingleWindow.cpp CohortIterator.cpp Window.cpp Cohort.cpp Reading.cpp ${CXX_SHARED_SOURCES})

ADD_LIBRARY(libcg3 STATIC libcg3.cpp ${CXX_SOURCES})
SET_TARGET_PROPERTIES(libcg3 PROPERTIES OUTPUT_NAME "cg3")
SET_TARGET_PROPERTIES(libcg3 PROPERTIES PREFIX "lib")
cg3_link(libcg3)

ADD_EXECUTABLE(cg-comp cg_comp.cpp)
cg3_link(cg-comp)
target_link_libraries(cg-comp libcg3)

ADD_EXECUTABLE(cg-proc cg_proc.cpp ApertiumApplicator.cpp MatxinApplicator.cpp)
cg3_link(cg-proc)
target_link_libraries(cg-proc libcg3)

ADD_EXECUTABLE(cg-conv cg_conv.cpp FormatConverter.cpp ApertiumApplicator.cpp MatxinApplicator.cpp)
cg3_link(cg-conv)
target_link_libraries(cg-conv libcg3)

ADD_EXECUTABLE(vislcg3 main.cpp)
cg3_link(vislcg3)
target_link_libraries(vislcg3 libcg3)

install(TARGETS libcg3 ARCHIVE DESTINATION lib)
install(TARGETS cg-comp cg-proc cg-conv vislcg3 RUNTIME DESTINATION bin)
