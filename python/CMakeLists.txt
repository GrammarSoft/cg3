find_package(SWIG 3.0 REQUIRED)
find_package(PythonInterp 3.5 REQUIRED)

set(PYTHON_FILE "constraint_grammar.py")
set(CPP_WRAP_FILE "constraint_grammar_wrap.cpp")

configure_file(constraint_grammar.i.in constraint_grammar.i @ONLY)
configure_file(setup.py.in setup.py)

add_custom_command(OUTPUT ${CPP_WRAP_FILE} ${PYTHON_FILE}
	COMMAND ${PYTHON_EXECUTABLE} setup.py build
	COMMENT "Building ${PYTHON_FILE}"
)

add_custom_target(wrapper ALL
	DEPENDS ${CPP_WRAP_FILE} ${PYTHON_FILE}
	VERBATIM
)

if(NOT PYTHON_INSTALL_PARAMS)
	set(PYTHON_INSTALL_PARAMS "--prefix=${CMAKE_INSTALL_PREFIX} --root=\$ENV{DESTDIR}/")
endif()

set(INSTALL_WRAPPER "${PYTHON_EXECUTABLE} setup.py install ${PYTHON_INSTALL_PARAMS}")
install(CODE "execute_process(COMMAND ${INSTALL_WRAPPER} WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR})")
