find_package(SWIG 3.0 REQUIRED)
if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.12.0")
	find_package(Python 3.5 REQUIRED)
	set(PYTHON_EXECUTABLE ${Python_EXECUTABLE})
else()
	find_package(PythonInterp 3.5 REQUIRED)
endif()

set(PYTHON_FILE "constraint_grammar.py")
set(CPP_WRAP_FILE "constraint_grammar_wrap.cpp")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/cg3.py" "from constraint_grammar import *\n")

set(BUILD_DEFS "")
get_directory_property(_defs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} COMPILE_DEFINITIONS)
foreach(d ${_defs})
	set(BUILD_DEFS "${BUILD_DEFS} -D${d}")
endforeach()
configure_file(constraint_grammar.i.in constraint_grammar.i @ONLY)
configure_file(setup.py.in setup.py)

add_custom_command(OUTPUT ${CPP_WRAP_FILE} ${PYTHON_FILE}
	COMMAND swig -python -c++ -I/usr/include -I. -Isrc -Iinclude -Wall -o python/constraint_grammar_wrap.cpp python/constraint_grammar.i
	COMMAND ${PYTHON_EXECUTABLE} -m build
	COMMENT "Building ${PYTHON_FILE}"
)

add_custom_target(wrapper ALL
	DEPENDS ${CPP_WRAP_FILE} ${PYTHON_FILE}
	VERBATIM
)

if(NOT PYTHON_INSTALL_PARAMS)
	set(PYTHON_INSTALL_PARAMS "--prefix=${CMAKE_INSTALL_PREFIX} --no-index --find-links=dist/")  # TODO keep --root=\$ENV{DESTDIR}/ ?
endif()

set(INSTALL_WRAPPER "${PYTHON_EXECUTABLE} -m pip install ${PYTHON_INSTALL_PARAMS} constraint_grammar")
install(CODE "execute_process(COMMAND ${INSTALL_WRAPPER} WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})")
