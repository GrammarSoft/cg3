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

using namespace CG3;
using namespace CG3::Strings;

int GrammarParser::readSetOperator(UChar **paren) {
	UChar *space = 0;
	int set_op = 0;
	space = u_strchr(*paren, ' ');
	if (space) {
		space[0] = 0;
		set_op = ux_isSetOp(*paren);
		if (!set_op) {
			space[0] = ' ';
			return 0;
		}
	}
	else {
		set_op = ux_isSetOp(*paren);
		if (!set_op) {
			return 0;
		}
	}
	*paren = *paren+u_strlen(*paren)+1;
	return set_op;
}

uint32_t GrammarParser::readSingleSet(UChar **paren) {
	UChar *space = u_strchr(*paren, ' ');
	uint32_t retval = 0;

	if ((*paren)[0] == '(') {
		space = (*paren);
		int matching = 0;
		if (!ux_findMatchingParenthesis(space, 0, &matching)) {
			u_fprintf(ux_stderr, "Error: Unmatched parentheses on or after line %u!\n", result->curline);
		}
		else {
			space[matching] = 0;
			UChar *composite = space+1;
			ux_trim(composite);

			CG3::Set *set_c = result->allocateSet();
			set_c->setLine(result->curline);
			set_c->setName(hash_sdbm_uchar(composite, 0));
			retval = hash_sdbm_uchar(set_c->getName(), 0);

			CG3::CompositeTag *ctag = result->allocateCompositeTag();
			UChar *temp = composite;
			while((temp = u_strchr(temp, ' ')) != 0) {
				if (temp[-1] == '\\') {
					temp++;
					continue;
				}
				temp[0] = 0;
				CG3::Tag *tag = result->allocateTag(composite);
				result->addTagToCompositeTag(tag, ctag);

				temp++;
				composite = temp;
			}
			CG3::Tag *tag = result->allocateTag(composite);
			result->addTagToCompositeTag(tag, ctag);

			result->addCompositeTagToSet(set_c, ctag);
			result->addSet(set_c);
			result->addUniqSet(set_c);

			*paren = space+matching+1;
			space = space+matching;
			if (u_isWhitespace((*paren)[0])) {
				(*paren)++;
			}
		}
	}
	else if (space && space[0] == ' ') {
		space[0] = 0;
		if (u_strlen(*paren)) {
			retval = hash_sdbm_uchar(*paren, 0);
		}
		*paren = space+1;
	}
	else if (u_strlen(*paren)) {
		retval = hash_sdbm_uchar(*paren, 0);
		*paren = *paren+u_strlen(*paren);
	}
	return retval;
}

uint32_t GrammarParser::readTagList(UChar **paren, std::list<uint32_t> *taglist) {
	UChar *space = u_strchr(*paren, ' ');
	uint32_t retval = 0;

	if ((*paren)[0] == '(') {
		space = (*paren);
		int matching = 0;
		if (!ux_findMatchingParenthesis(space, 0, &matching)) {
			u_fprintf(ux_stderr, "Error: Unmatched parentheses on or after line %u!\n", result->curline);
		}
		else {
			space[matching] = 0;
			UChar *composite = space+1;
			ux_trim(composite);

			UChar *temp = composite;
			while((temp = u_strchr(temp, ' ')) != 0) {
				if (temp[-1] == '\\') {
					temp++;
					continue;
				}
				temp[0] = 0;
				CG3::Tag *tag = result->allocateTag(composite);
				tag->parseTag(composite);
				tag->rehash();
				result->addTag(tag);
				taglist->push_back(tag->hash);

				temp++;
				composite = temp;
				retval++;
			}
			CG3::Tag *tag = result->allocateTag(composite);
			tag->parseTag(composite);
			tag->rehash();
			result->addTag(tag);
			taglist->push_back(tag->hash);
			retval++;

			*paren = space+matching+1;
			space = space+matching;
			if (u_isWhitespace((*paren)[0])) {
				(*paren)++;
			}
		}
	}
	return retval;
}

int GrammarParser::parseSet(const UChar *line) {
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
	//memset(local, 0, length+1);
	u_strcpy(local, line+u_strlen(keywords[K_SET])+1);

	// Allocate temp vars and skips over "SET X = "
	UChar *space = u_strchr(local, ' ');
	space[0] = 0;
	space+=3;

	CG3::Set *curset = result->allocateSet();
	curset->setName(local);
	curset->setLine(result->curline);
	result->addSet(curset);

	uint32_t set_a = 0;
	uint32_t set_b = 0;
	uint32_t res = hash_sdbm_uchar(curset->getName(), 0);
	int set_op = S_IGNORE;
	while(space[0]) {
		if (!set_a) {
			set_a = readSingleSet(&space);
			if (!set_a) {
				u_fprintf(ux_stderr, "Error: Could not read in left hand set on line %u for set %S - cannot continue!\n", result->curline, local);
				break;
			}
		}
		if (!set_op) {
			set_op = readSetOperator(&space);
			if (!set_op) {
				u_fprintf(ux_stderr, "Warning: Could not read in operator on line %u for set %S - assuming set alias.\n", result->curline, curset->name);
				result->manipulateSet(res, S_OR, set_a, res);
				break;
			}
		}
		if (!set_b) {
			set_b = readSingleSet(&space);
			if (!set_b) {
				u_fprintf(ux_stderr, "Error: Could not read in right hand set on line %u for set %S - cannot continue!\n", result->curline, curset->name);
				break;
			}
		}
		if (set_a && set_b && set_op) {
			if (option_vislcg_compat && set_op == S_MINUS) {
				u_fprintf(ux_stderr, "Warning: Set %S on line %u - difference operator converted to fail-fast as per --vislcg-compat.\n", curset->name, result->curline);
				set_op = S_FAILFAST;
			}
			result->manipulateSet(set_a, set_op, set_b, res);
			set_op = 0;
			set_b = 0;
			set_a = res;
		}
	}

	result->addUniqSet(curset);

	delete local;
	return 0;
}
