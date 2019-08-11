%module constraint_grammar

%include <std_map.i>
%include <std_string.i>
%include <std_vector.i>
%include <Grammar.hpp>

%typemap(in) char ** {
  if (PyList_Check($input)) {
    int size = PyList_Size($input);
    int i = 0;
    $1 = (char **) malloc((size+1)*sizeof(char *));
    for (i = 0; i < size; i++) {
      PyObject *py_obj = PyList_GetItem($input, i);
      if (PyUnicode_Check(py_obj)) {
        $1[i] = strdup(PyUnicode_AsUTF8(py_obj));
      }
      else {
        PyErr_SetString(PyExc_TypeError, "list must contain strings");
        free($1);
        return NULL;
      }
    }
    $1[i] = 0;
  } else {
    PyErr_SetString(PyExc_TypeError, "not a list");
    return NULL;
  }
}

%inline%{
#define SWIG_FILE_WITH_INIT
#include "stdafx.hpp"
#include "Grammar.hpp"
#include "TextualParser.hpp"
#include "BinaryGrammar.hpp"
#include "ApertiumApplicator.hpp"
#include "MatxinApplicator.hpp"
#include "GrammarApplicator.hpp"

#include <getopt.h>

class CGProc: public CG3::Grammar
{
private:
	CG3::Grammar grammar;

public:
	CGProc(char *dictionary_path);
	void cg_proc(char argc, char **argv, char *input_path, char *output_path);
};

CGProc::CGProc(char *dictionary_path)
{
	std::unique_ptr<CG3::IGrammarParser> parser;
	FILE* dictionary = fopen(dictionary_path, "rb");
	fread(&CG3::cbuffers[0][0], 1, 4, dictionary);
	fclose(dictionary);
	if (CG3::cbuffers[0][0] == 'C' && CG3::cbuffers[0][1] == 'G' && CG3::cbuffers[0][2] == '3' && CG3::cbuffers[0][3] == 'B') {
		parser.reset(new CG3::BinaryGrammar(grammar, std::cerr));
	}
	else {
		parser.reset(new CG3::TextualParser(grammar, std::cerr));
	}
	parser->parse_grammar(dictionary_path);
}

void CGProc::cg_proc(char argc, char **argv, char *input_path, char *output_path)
{
	bool trace = false;
	bool wordform_case = false;
	bool print_word_forms = true;
	bool only_first = false;
	int cmd = 0;
	int sections = 0;
	int stream_format = 1;
	bool null_flush = false;
	std::string single_rule;

	UErrorCode status = U_ZERO_ERROR;
	std::ifstream input;
	input.open(input_path, std::ios::binary);
	std::istream& ux_stdin = input;
	std::ofstream output;
	output.open(output_path, std::ios::binary);
	std::ostream& ux_stdout = output;

	int c = 0;
	optind = 1;
	while (true) {
		c = getopt(argc, argv, "s:f:tn1wz");
		if (c == -1) {
			break;
		}

		switch (c) {
			case 'f':
				stream_format = atoi(optarg);
				break;

			case 't':
				trace = true;
				break;
			case 's':
				sections = atoi(optarg);
				break;

			case 'n':
				print_word_forms = false;
				break;

			case '1':
				only_first = true;
				break;

			case 'w':
				wordform_case = true;
				break;

			case 'z':
				null_flush = true;
				break;
		}
	}
	grammar.reindex();
	std::unique_ptr<CG3::GrammarApplicator> applicator;

	CG3::ApertiumApplicator* apertiumApplicator = new CG3::ApertiumApplicator(std::cerr);
	apertiumApplicator->setNullFlush(null_flush);
	apertiumApplicator->wordform_case = wordform_case;
	apertiumApplicator->print_word_forms = print_word_forms;
	apertiumApplicator->print_only_first = only_first;
	applicator.reset(apertiumApplicator);

	applicator->setGrammar(&grammar);
	for (int32_t i = 1; i <= sections; i++) {
		applicator->sections.push_back(i);
	}

	applicator->trace = trace;
	applicator->unicode_tags = true;
	applicator->unique_tags = false;

	applicator->runGrammarOnText(ux_stdin, ux_stdout);
	u_cleanup();
}

%}
