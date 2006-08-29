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

#include "stdafx.h"
#include "Strings.h"
#include <unicode/uregex.h>
#include "GrammarParser.h"
#include "Grammar.h"
#include "uextras.h"

using namespace CG3::Strings;

namespace CG3 {
	namespace GrammarParser {
		int parseSelectRemoveIffDelimit(const UChar *line, uint32_t key, CG3::Grammar *result) {
			if (!line) {
				u_fprintf(ux_stderr, "Error: No string provided at line %u - cannot continue!\n", result->curline);
				return -1;
			}
			int length = u_strlen(line);
			if (!length) {
				u_fprintf(ux_stderr, "Error: No string provided at line %u - cannot continue!\n", result->curline);
				return -1;
			}
			if (key != K_SELECT && key != K_REMOVE && key != K_IFF && key != K_DELIMIT) {
				u_fprintf(ux_stderr, "Error: Invalid keyword %u - cannot continue!\n", key);
				return -1;
			}

			UChar *local = new UChar[length+1];
			u_strcpy(local, line+u_strlen(keywords[K_SET])+1);

			UChar *space = u_strchr(local, ' ');
			space[0] = 0;

			if (u_strcmp(local, keywords[key]) != 0) {
			}

			delete local;
			return 0;
		}
	}
}
