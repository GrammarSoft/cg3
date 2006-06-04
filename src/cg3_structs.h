#ifndef __CG3_STRUCTS
#define __CG3_STRUCTS

#include <unicode/ustring.h>
#include "hashtable.h"

namespace CG3 {

	struct Tag {
		UChar *tag;
	};

	struct MultiTag {
		hashtable *tags;
	};

	struct Set {
		UChar *name;
		unsigned int line;
	};

	struct Grammar {
		unsigned int last_modified;
		UChar *name;
		unsigned int lines;
	};

}

#endif
