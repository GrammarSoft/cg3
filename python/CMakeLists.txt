find_package(SWIG 3.0 REQUIRED)

set(Python_ADDITIONAL_VERSIONS 3.5)
find_package(PythonInterp REQUIRED)

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

set(INSTALL_WRAPPER "${PYTHON_EXECUTABLE} setup.py install --prefix=${CMAKE_INSTALL_PREFIX}")
install(CODE "execute_process(COMMAND ${INSTALL_WRAPPER} WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR})")
