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

int GrammarParser::parseAddMapReplaceAppend(const UChar *line, KEYWORDS key) {
	if (!line) {
		u_fprintf(ux_stderr, "Error: No string provided at line %u - cannot continue!\n", result->curline);
		return -1;
	}
	int length = u_strlen(line);
	if (!length) {
		u_fprintf(ux_stderr, "Error: No string provided at line %u - cannot continue!\n", result->curline);
		return -1;
	}
	if (key != K_ADD && key != K_MAP && key != K_REPLACE && key != K_APPEND) {
		u_fprintf(ux_stderr, "Error: Invalid keyword %u for line %u - cannot continue!\n", key, result->curline);
		return -1;
	}

	uint32_t lname = hash_sdbm_uchar(line, 0);
	UChar *local = new UChar[length+1];
	u_strcpy(local, line);
	UChar *space = u_strchr(local, ' ');
	space[0] = 0;
	UChar *wordform = 0;

	UChar *name = local;
	if (u_strcmp(local, keywords[key]) != 0) {
		wordform = local;
		space++;
		name = space;
		space = u_strchr(space, ' ');
		space[0] = 0;
		space++;
	}
	else {
		space++;
	}

	name = u_strchr(name, ':');
	if (name) {
		name[0] = 0;
		name++;
	}

	CG3::Rule *rule = result->allocateRule();
	rule->line = result->curline;
	rule->type = key;

	if (wordform) {
		Tag *wform = result->allocateTag(wordform);
		rule->wordform = wform->rehash();
		result->addTag(wform);
	}

	readTagList(&space, &rule->maplist);
	uint32_t res = parseTarget(&space);

	rule->target = res;
	addRuleToGrammar(rule);

	if (name && name[0] && u_strlen(name)) {
		result->addAnchor(name, (uint32_t)(result->rules.size()-1));
		rule->setName(name);
	}
	else {
		rule->setName(lname);
	}

	if (space && space[0] && space[0] == '(') {
		parseContextualTests(&space, rule);
	}

	delete[] local;
	return 0;
}

int GrammarParser::parseMappingPrefix(const UChar *line) {
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
	u_strcpy(local, line+u_strlen(keywords[K_MAPPING_PREFIX])+1);

	// Allocate temp vars and skips over "MAPPING-PREFIX = "
	UChar *space = u_strchr(local, ' ');
	space[0] = 0;
	space++;
	UChar *base = space;

	if (u_strlen(base)) {
		result->mapping_prefix = base[0];
	}
	else {
		u_fprintf(ux_stderr, "Warning: No mapping prefix provided at line %u - using @ instead.\n", result->curline);
		result->mapping_prefix = '@';
	}

	delete[] local;
	return 0;
}
