#ifndef __CG3_STRUCTS
#define __CG3_STRUCTS

#include <unicode/ustring.h>

namespace CG3 {

	struct Tag {
		UChar *tag;
	};

	struct MultiTag {
		stdext::hash_map<UChar*, bool> *tags;
	};

	struct Set {
		UChar *name;
		unsigned int line;
	};

	struct IndexEntry {
		stdext::hash_map<unsigned int, unsigned int> *rules;
		stdext::hash_map<UChar*, bool> *sets;
		stdext::hash_map<UChar*, bool> *index_simple;
	};

	struct Grammar {
		unsigned int last_modified;
		UChar *name;
		unsigned int lines;
		stdext::hash_map<UChar*, bool> *index_simple;
		stdext::hash_map<UChar*, bool> *sets;
		stdext::hash_map<UChar*, bool> *delimiters;
	};

}

#endif
