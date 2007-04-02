/*
 * Copyright (C) 2006, GrammarSoft Aps
 * and the VISL project at the University of Southern Denmark.
 * All Rights Reserved.
 *
 * The contents of this file are subject to the GrammarSoft Public
 * License Version 1.0 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.grammarsoft.com/GSPL or
 * http://visl.sdu.dk/GSPL.txt
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 */

#include "GrammarParser.h"

using namespace CG3;
using namespace CG3::Strings;

int GrammarParser::parsePreferredTargets(const UChar *line) {
	if (!line) {
		u_fprintf(ux_stderr, "Error: No string provided at line %u - cannot continue!\n", result->curline);
		return -1;
	}
	int length = u_strlen(line);
	if (!length) {
		u_fprintf(ux_stderr, "Error: No string provided at line %u - cannot continue!\n", result->curline);
		return -1;
	}
	UChar *local = new UChar[length+1];
	u_strcpy(local, line+u_strlen(keywords[K_PREFERRED_TARGETS])+1);

	// Allocate temp vars and skips over "PREFERRED-TARGETS = "
	UChar *space = u_strchr(local, ' ');
	space[0] = 0;
	space++;
	UChar *base = space;

	while (space && (space = u_strchr(space, ' ')) != 0) {
		space[0] = 0;
		space++;
		if (u_strlen(base)) {
			result->addPreferredTarget(base);
		}
		base = space;
	}
	if (u_strlen(base)) {
		result->addPreferredTarget(base);
	}

	delete local;
	return 0;
}
