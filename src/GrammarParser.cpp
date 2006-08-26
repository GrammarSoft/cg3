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
		bool findMatchingParenthesis(const UChar *structure, int pos, int *result) {
			int len = u_strlen(structure);
			while (pos < len) {
				pos++;
				if (structure[pos] == ')' && structure[pos-1] != '\\') {
					*result = pos;
					return true;
				}
				if (structure[pos] == '(' && structure[pos-1] != '\\') {
					int tmp;
					findMatchingParenthesis(structure, pos, &tmp);
					pos = tmp;
				}
			}
			return false;
		}

		int parseSetList(UChar *paren, CG3::Set *curset, CG3::Grammar *result) {
			if (!curset) {
				std::cerr << "Error: No preallocated set provided - cannot continue!" << std::endl;
				return -1;
			}
			if (!paren) {
				std::cerr << "Error: No string provided - cannot continue!" << std::endl;
				return -1;
			}
			UChar *space = paren;
			while(paren[0]) {
				if (space[0] == 0) {
					if (u_strlen(paren)) {
						CG3::CompositeTag *ctag = result->allocateCompositeTag();
						CG3::Tag *tag = ctag->allocateTag(paren);
						tag->parseTag(paren);
						ctag->addTag(tag);
						result->addCompositeTagToSet(curset, ctag);
					}
					paren = space;
				}
				else if (space[0] == ' ') {
					if (space[-1] != '\\') {
						space[0] = 0;
						if (u_strlen(paren)) {
							CG3::CompositeTag *ctag = result->allocateCompositeTag();
							CG3::Tag *tag = ctag->allocateTag(paren);
							tag->parseTag(paren);
							ctag->addTag(tag);
							result->addCompositeTagToSet(curset, ctag);
						}
						paren = space+1;
					}
				}
				else if (space[0] == '(') {
					if (space[-1] != '\\') {
						int matching = 0;
						if (!findMatchingParenthesis(space, 0, &matching)) {
							std::cerr << "Error: Unmatched parentheses on or after line " << curset->getLine() << std::endl;
						} else {
							space[matching] = 0;
							UChar *composite = space+1;
							ux_trimUChar(composite);

							CG3::CompositeTag *ctag = result->allocateCompositeTag();
							UChar *temp = composite;
							while(temp = u_strchr(temp, ' ')) {
								if (temp[-1] == '\\') {
									temp++;
									continue;
								}
								temp[0] = 0;
								CG3::Tag *tag = ctag->allocateTag(composite);
								tag->parseTag(composite);
								ctag->addTag(tag);

								temp++;
								composite = temp;
							}
							CG3::Tag *tag = ctag->allocateTag(composite);
							tag->parseTag(composite);
							ctag->addTag(tag);

							result->addCompositeTagToSet(curset, ctag);

							paren = space+matching+1;
							space = space+matching;
							ux_trimUChar(paren);
						}
					}
				}
				space++;
			}
			return 0;
		}

		int readSetOperator(UChar **paren, CG3::Grammar *result) {
			UChar *space = 0;
			int set_op = 0;
			ux_trimUChar(*paren);
			space = u_strchr(*paren, ' ');
			if (space) {
				space[0] = 0;
				set_op = ux_isSetOp(*paren);
				if (!set_op) {
					space[0] = ' ';
					return 0;
				}
			} else {
				set_op = ux_isSetOp(*paren);
				if (!set_op) {
					return 0;
				}
			}
			*paren = *paren+u_strlen(*paren)+1;
			return set_op;
		}

		unsigned long readSingleSet(UChar **paren, CG3::Grammar *result) {
			ux_trimUChar(*paren);
			UChar *space = u_strchr(*paren, ' ');
			unsigned long retval = 0;

			if ((*paren)[0] == '(') {
				space = (*paren);
				int matching = 0;
				if (!findMatchingParenthesis(space, 0, &matching)) {
					std::cerr << "Error: Unmatched parentheses on or after line " << result->lines << std::endl;
				} else {
					space[matching] = 0;
					UChar *composite = space+1;
					ux_trimUChar(composite);

					CG3::Set *set_c = result->allocateSet();
					set_c->setName(hash_sdbm_uchar(composite));
					retval = hash_sdbm_uchar(set_c->getName());

					CG3::CompositeTag *ctag = result->allocateCompositeTag();
					UChar *temp = composite;
					while(temp = u_strchr(temp, ' ')) {
						if (temp[-1] == '\\') {
							temp++;
							continue;
						}
						temp[0] = 0;
						CG3::Tag *tag = ctag->allocateTag(composite);
						tag->parseTag(composite);
						ctag->addTag(tag);

						temp++;
						composite = temp;
					}
					CG3::Tag *tag = ctag->allocateTag(composite);
					tag->parseTag(composite);
					ctag->addTag(tag);

					result->addCompositeTagToSet(set_c, ctag);
					set_c->setLine(result->lines);
					result->addSet(set_c);

					*paren = space+matching+1;
					space = space+matching;
					ux_trimUChar(*paren);
				}
			}
			else if (space && space[0] == ' ') {
				space[0] = 0;
				if (u_strlen(*paren)) {
					retval = hash_sdbm_uchar(*paren);
				}
				*paren = space+1;
			} else if (u_strlen(*paren)) {
				retval = hash_sdbm_uchar(*paren);
				*paren = *paren+u_strlen(*paren);
			}
			return retval;
		}

		int parseSet(const UChar *line, const unsigned int which, CG3::Grammar *result) {
			if (!which) {
				std::cerr << "Error: No line number provided - cannot continue!" << std::endl;
				return -1;
			}
			if (!line) {
				std::cerr << "Error: No string provided at line " << which << " - cannot continue!" << std::endl;
				return -1;
			}
			int length = u_strlen(line);
			if (!length) {
				std::cerr << "Error: No string provided at line " << which << " - cannot continue!" << std::endl;
				return -1;
			}
			UChar *local = new UChar[length+1];
			memset(local, 0, length+1);
			u_strcpy(local, line+u_strlen(keywords[K_SET])+1);

			// Allocate temp vars and skips over "SET X = "
			UChar *space = u_strchr(local, ' ');
			space[0] = 0;
			space+=3;

			CG3::Set *curset = result->allocateSet();
			curset->setName(local);
			curset->setLine(which);
			result->addSet(curset);

			unsigned long set_a = 0;
			unsigned long set_b = 0;
			unsigned long res = hash_sdbm_uchar(curset->getName());
			int set_op = S_IGNORE;
			while(space[0]) {
				if (!set_a) {
					set_a = readSingleSet(&space, result);
					if (!set_a) {
						std::cerr << "Error: Could not read in left hand set on line " << which << " - cannot continue!" << std::endl;
						return -1;
					}
				}
				if (!set_op) {
					set_op = readSetOperator(&space, result);
					if (!set_op) {
						std::cerr << "Warning: Could not read in set operator on line " << which << " - assuming set alias." << std::endl;
						result->manipulateSet(res, S_OR, set_a, res);
						return -1;
					}
				}
				if (!set_b) {
					set_b = readSingleSet(&space, result);
					if (!set_b) {
						std::cerr << "Error: Could not read in right hand set on line " << which << " - cannot continue!" << std::endl;
						return -1;
					}
				}
				if (set_a && set_b && set_op) {
					result->manipulateSet(set_a, set_op, set_b, res);
					set_op = 0;
					set_b = 0;
					set_a = res;
				}
			}

			delete local;
			return 0;
		}

		int parseList(const UChar *line, const unsigned int which, CG3::Grammar *result) {
			if (!which) {
				std::cerr << "Error: No line number provided - cannot continue!" << std::endl;
				return -1;
			}
			if (!line) {
				std::cerr << "Error: No string provided at line " << which << " - cannot continue!" << std::endl;
				return -1;
			}
			int length = u_strlen(line);
			if (!length) {
				std::cerr << "Error: No string provided at line " << which << " - cannot continue!" << std::endl;
				return -1;
			}
			UChar *local = new UChar[length+1];
			memset(local, 0, length+1);
			u_strcpy(local, line+u_strlen(keywords[K_LIST])+1);

			// Allocate temp vars and skips over "LIST X = "
			UChar *space = u_strchr(local, ' ');
			space[0] = 0;
			space+=3;

			CG3::Set *curset = result->allocateSet();
			curset->setName(local);
			curset->setLine(which);

			parseSetList(space, curset, result);

			result->addSet(curset);

			delete local;
			return 0;
		}

		int parseSingleLine(const int key, const UChar *line, const unsigned int which, CG3::Grammar *result) {
			if (key <= K_IGNORE || key >= KEYWORD_COUNT) {
				std::cerr << "Error: Invalid keyword " << key << " - skipping." << std::endl;
				return -1;
			}

			UErrorCode status = U_ZERO_ERROR;
			int length = u_strlen(line);
			int locallength = int(length*1.5);
			UChar *local = new UChar[locallength];
			//memset(local, 0, locallength);

			uregex_setText(regexps[R_PACKSPACE], line, length, &status);
			if (status != U_ZERO_ERROR) {
				std::cerr << "Error: uregex_setText returned " << u_errorName(status) << " - cannot continue." << std::endl;
				return -1;
			}
			status = U_ZERO_ERROR;
			uregex_replaceAll(regexps[R_PACKSPACE], stringbits[S_SPACE], u_strlen(stringbits[S_SPACE]), local, locallength, &status);
			if (status != U_ZERO_ERROR) {
				std::cerr << "Error: uregex_replaceAll returned " << u_errorName(status) << " - cannot continue." << std::endl;
				return -1;
			}

			switch(key) {
				case K_LIST:
					parseList(local, which, result);
					break;
				case K_SET:
					parseSet(local, which, result);
					break;
				default:
					break;
			}

			delete local;
			return 0;
		}
		
		int parse_grammar_from_ufile(UFILE *input, CG3::Grammar *result) {
			u_frewind(input);
			if (u_feof(input)) {
				std::cerr << "Error: Input is null - nothing to parse!" << std::endl;
				return -1;
			}
			if (!result) {
				std::cerr << "Error: No preallocated grammar provided - cannot continue!" << std::endl;
				return -1;
			}
			
			int error = init_keywords();
			if (error) {
				std::cerr << "Error: init_keywords returned " << error << std::endl;
				return error;
			}

			error = init_regexps();
			if (error) {
				std::cerr << "Error: init_regexps returned " << error << std::endl;
				return error;
			}

			error = init_strings();
			if (error) {
				std::cerr << "Error: init_strings returned " << error << std::endl;
				return error;
			}

			// ToDo: Make this dynamic.
			std::map<unsigned int, UChar*> lines;
			std::map<unsigned int, unsigned int> keys;
			unsigned int lastcmd = 0;

			while (!u_feof(input)) {
				result->lines++;

				#define BUFFER_SIZE (65536)
				UChar *line = new UChar[BUFFER_SIZE];
				//memset(line, 0, sizeof(UChar)*BUFFER_SIZE);
				u_fgets(line, BUFFER_SIZE-1, input);
				ux_cutComments(line, '#');
				ux_cutComments(line, ';');

				int length = u_strlen(line);
				bool notnull = false;
				for (int i=0;i<length;i++) {
					if (!u_isWhitespace(line[i])) {
						notnull = true;
						break;
					}
				}
				if (notnull) {
					ux_trimUChar(line);
					int keyword = 0;
					for (int i=1;i<KEYWORD_COUNT;i++) {
						UChar *pos = 0;
						int length = 0;
						if (pos = u_strstr(line, keywords[i])) {
							length = u_strlen(keywords[i]);
							if (
								((pos == line) || (pos > line && u_isWhitespace(pos[-1])))
								&& (pos[length] == 0 || pos[length] == ':' || u_isWhitespace(pos[length]))
								) {
								lastcmd = result->lines;
								keyword = i;
								break;
							}
						}
					}
					if (keyword || !lines[lastcmd]) {
						while (!lines.empty()) {
							unsigned int which = (*lines.begin()).first;
							UChar *line = (*lines.begin()).second;
							parseSingleLine(keys[which], line, which, result);
							delete line;
							lines.erase(lines.begin());
							keys.erase(which);
						}
						lines[result->lines] = line;
						keys[result->lines] = keyword;
					} else {
						length = u_strlen(lines[lastcmd]);
						lines[lastcmd][length] = ' ';
						lines[lastcmd][length+1] = 0;
						u_strcat(lines[lastcmd], line);
					}
				} else {
					delete line;
				}
			}

			while (!lines.empty()) {
				unsigned int which = (*lines.begin()).first;
				UChar *line = (*lines.begin()).second;
				parseSingleLine(keys[which], line, which, result);
				delete line;
				lines.erase(lines.begin());
				keys.erase(which);
			}

			free_keywords();
			free_regexps();
			free_strings();
			lines.clear();
			keys.clear();

			return 0; // No errors.
		}
		
		int parse_grammar_from_file(const char *filename, const char *codepage, CG3::Grammar *result) {
			UFILE *grammar = u_fopen(filename, "r", NULL, codepage);
			if (!grammar) {
				return -1; // File could not be opened for reading.
			}
			
			int error = parse_grammar_from_ufile(grammar, result);
			if (error || !result) {
				return error;
			}
			result->name = new UChar[strlen(filename)+1];
			u_uastrcpy(result->name, filename);
			return 0;
		}

		int parse_grammar_from_buffer(const UChar *buffer, const char *codepage, CG3::Grammar *result) {
			return -1; // Not implemented.
		}
	}
}
