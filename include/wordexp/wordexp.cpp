#include "wordexp.h"

namespace CG3_WordExp {

int wordexp(const char* s, wordexp_t* p, int flags) {
	(void)s;
	(void)p;
	(void)flags;
	throw std::runtime_error("NOT IMPLEMENTED");
}

void wordfree(wordexp_t* p) {
	(void)p;
	throw std::runtime_error("NOT IMPLEMENTED");
}

}
