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

int GrammarParser::parseSubstitute(const UChar *line) {
	if (!line) {
		u_fprintf(ux_stderr, "Error: No string provided at line %u - cannot continue!\n", result->curline);
		return -1;
	}
	int length = u_strlen(line);
	if (!length) {
		u_fprintf(ux_stderr, "Error: No string provided at line %u - cannot continue!\n", result->curline);
		return -1;
	}

	uint32_t lname = hash_sdbm_uchar(line, 0);
	UChar *local = gbuffers[1];
	u_strcpy(local, line);
	UChar *space = u_strchr(local, ' ');
	space[0] = 0;
	UChar *wordform = 0;

	UChar *name = local;
	if (u_strcmp(local, keywords[K_SUBSTITUTE]) != 0) {
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
	rule->type = K_SUBSTITUTE;

	if (wordform) {
		Tag *wform = result->allocateTag(wordform);
		rule->wordform = wform->rehash();
		result->addTag(wform);
	}

	readTagList(&space, &rule->sublist);
	readTagList(&space, &rule->maplist);
	rule->target = parseTarget(&space);

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

	return 0;
}
