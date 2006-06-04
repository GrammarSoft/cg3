#ifndef __CG3_STRUCTS
#define __CG3_STRUCTS

#include <unicode/ustring.h>
#include "hashtable.h"

namespace CG3 {

	struct Set {
		UChar *name;
	};

	struct Grammar {
		unsigned int last_modified;
		UChar *name;
	};

}

#endif
