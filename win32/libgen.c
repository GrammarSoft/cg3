#include "libgen.h"
#include <string.h>

// http://www.opengroup.org/onlinepubs/007908775/xsh/basename.html

const char *basename(const char *path) {
	if (path != NULL) {
		// Find the last position of the \ in the path name
		const char *pos = strrchr(path, '\\');

		if (pos != NULL) { // If a \ char was found...
			if (pos + 1 != NULL) // If it is not the last character in the string...
				return pos + 1; // then return a pointer to the first character after \.
			else
				return pos; // else return a pointer to \

		}
		else { // If a \ char was NOT found
			return path; // return the pointer passed to basename (this is probably non-conformant)
		}

	}
	else { // If path == NULL, return "."
		return ".";
	}
}
