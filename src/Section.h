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
#ifndef __SECTION_H
#define __SECTION_H

#include <unicode/ustring.h>

namespace CG3 {

	class Section {
	public:
		UChar *name;
		uint32_t hash;
		uint32_t start, end;
		uint32_t line;

		Section() {
			name = 0;
			hash = 0;
			start = 0;
			end = 0;
		}

		void setName(uint32_t to) {
			if (!to) {
				if (!line) {
					to = (uint32_t)rand();
				} else {
					to = line;
				}
			}
			name = new UChar[24];
			memset(name, 0, 24);
			u_sprintf(name, "Section_%u", to);
		}
		void setName(const UChar *to) {
			if (to) {
				name = new UChar[u_strlen(to)+1];
				u_strcpy(name, to);
			} else {
				setName((uint32_t)rand());
			}
		}
		const UChar *getName() {
			return name;
		}

		void setLine(uint32_t to) {
			line = to;
		}
		uint32_t getLine() {
			return line;
		}
	};
}

#endif
