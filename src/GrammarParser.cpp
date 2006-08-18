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
#include "GrammarParser.h"
#include "Grammar.h"

namespace CG3 {
	namespace GrammarParser {
		enum KEYWORDS {
			K_IGNORE,
			K_SETS,
			K_LIST,
			K_SET,
			K_DELIMITERS,
			K_PREFERRED_TARGETS,
			K_MAPPINGS,
			K_CONSTRAINTS,
			K_CORRECTIONS,
			K_SECTION,
			K_ADD,
			K_MAP,
			K_REPLACE,
			K_SELECT,
			K_REMOVE,
			K_IFF,
			K_APPEND,
			K_SUBSTITUTE,
			K_END,
			K_ANCHOR,
			K_EXECUTE,
			K_JUMP,
			K_DEFINEGLOBAL,
			K_REMGLOBAL,
			K_SETGLOBAL,
			K_MINVALUE,
			K_MAXVALUE,
			K_DELIMIT,
			K_MATCH,
			KEYWORD_COUNT
		};
		UChar *keywords[KEYWORD_COUNT];

/* These are not really needed.
		U_STRING_DECL(S_ADD, "ADD", 3);      U_STRING_INIT(S_ADD, "ADD", 3);
		U_STRING_DECL(S_OR, "OR", 2);        U_STRING_INIT(S_OR, "OR", 2);
		U_STRING_DECL(S_PLUS, "+", 1);       U_STRING_INIT(S_PLUS, "+", 1);
		U_STRING_DECL(S_MINUS, "-", 1);      U_STRING_INIT(S_MINUS, "-", 1);
		U_STRING_DECL(S_MULTIPLY, "*", 1);   U_STRING_INIT(S_MULTIPLY, "*", 1);
		U_STRING_DECL(S_DENY, "^", 1);       U_STRING_INIT(S_DENY, "^", 1);
		U_STRING_DECL(S_BACKSLASH, "\\", 1); U_STRING_INIT(S_BACKSLASH, "\\", 1);
		U_STRING_DECL(S_HASH, "#", 1);       U_STRING_INIT(S_HASH, "#", 1);
//*/

		inline int init_keyword_single(const char *keyword, const unsigned int entry) {
			if (entry >= KEYWORD_COUNT) {
				return -1; // Out of bounds
			}
			UChar buffer[1024];
			u_memset(buffer, 0, 1024);
			u_uastrcpy(buffer, keyword);
			keywords[entry] = new UChar[u_strlen(buffer)+1];
			u_strcpy(keywords[entry], buffer);
			return 0;
		}
		
		int init_keywords() {
			init_keyword_single("IGNORE",            K_IGNORE);
			init_keyword_single("SETS",              K_SETS);
			init_keyword_single("LIST",              K_LIST);
			init_keyword_single("SET",               K_SET);
			init_keyword_single("DELIMITERS",        K_DELIMITERS);
			init_keyword_single("PREFERRED-TARGETS", K_PREFERRED_TARGETS);
			init_keyword_single("MAPPINGS",          K_MAPPINGS);
			init_keyword_single("CONSTRAINTS",       K_CONSTRAINTS);
			init_keyword_single("CORRECTIONS",       K_CORRECTIONS);
			init_keyword_single("SECTION",           K_SECTION);
			init_keyword_single("ADD",               K_ADD);
			init_keyword_single("MAP",               K_MAP);
			init_keyword_single("REPLACE",           K_REPLACE);
			init_keyword_single("SELECT",            K_SELECT);
			init_keyword_single("REMOVE",            K_REMOVE);
			init_keyword_single("IFF",               K_IFF);
			init_keyword_single("APPEND",            K_APPEND);
			init_keyword_single("SUBSTITUTE",        K_SUBSTITUTE);
			init_keyword_single("END",               K_END);
			init_keyword_single("ANCHOR",            K_ANCHOR);
			init_keyword_single("EXECUTE",           K_EXECUTE);
			init_keyword_single("JUMP",              K_JUMP);
			init_keyword_single("DEFINEGLOBAL",      K_DEFINEGLOBAL);
			init_keyword_single("REMGLOBAL",         K_REMGLOBAL);
			init_keyword_single("SETGLOBAL",         K_SETGLOBAL);
			init_keyword_single("MINVALUE",          K_MINVALUE);
			init_keyword_single("MAXVALUE",          K_MAXVALUE);
			init_keyword_single("DELIMIT",           K_DELIMIT);
			init_keyword_single("MATCH",             K_MATCH);

			for (int i=0;i<KEYWORD_COUNT;i++) {
				if (!keywords[i]) {
					return -(i+1); // One did not get set properly. Returns -i to pinpoint which.
				}
			}
			return 0;
		}

		int free_keywords() {
			for (int i=0;i<KEYWORD_COUNT;i++) {
				if (keywords[i]) {
					delete keywords[i];
				}
				keywords[i] = 0;
			}
			return 0;
		}

		bool ux_isNewline(const UChar32 current, const UChar32 previous) {
			return (current == 0x0D0A // ASCII \r\n
			|| current == 0x2028 // Unicode Line Seperator
			|| current == 0x2029 // Unicode Paragraph Seperator
			|| current == 0x0085 // EBCDIC NEL
			|| current == 0x000C // Form Feed
			|| current == 0x000A // ASCII \n
			|| previous == 0x000D); // ASCII \r
		}
		
		int parse_grammar_from_ufile(UFILE *input, CG3::Grammar *result) {
			u_frewind(input);
			if (u_feof(input)) {
				std::cerr << "Input is null - nothing to parse!" << std::endl;
				return -1;
			}
			if (!result) {
				std::cerr << "No preallocated grammar provided - cannot continue!" << std::endl;
				return -1;
			}
			
			int error = init_keywords();
			if (error) {
				std::cerr << "init_keywords returned " << error << std::endl;
				return error;
			}

			UErrorCode *uerror = new UErrorCode;
			UChar32 previous = 0;
			UChar32 current = 0;

			#define BUFFER_SIZE 1024
			#define NUM_BUFFERS 10
			unsigned int buffer_pos = 0;
			UChar32 buffer32[BUFFER_SIZE];
			memset(buffer32, 0, sizeof(UChar32)*BUFFER_SIZE);

			UChar *buffers16[NUM_BUFFERS];
			for (int i=0;i<NUM_BUFFERS;i++) {
				buffers16[i] = new UChar[BUFFER_SIZE];
				memset(buffers16[i], 0, sizeof(UChar)*BUFFER_SIZE);
			}
			int which16 = 1;
			UChar *current16 = buffers16[which16%NUM_BUFFERS];
			UChar *previous16 = buffers16[(NUM_BUFFERS+which16-1)%NUM_BUFFERS];

			bool MODE_IGNORELINE = false;
			bool MODE_ESCAPE = false;
			bool MODE_STRING = false;

			int MODE_COMMAND = K_IGNORE;
			int TOKENS_SINCE_CMD = 0;
			int MODE_PARENTHESES = 0;
			int PREV_PARENTHESES = 0;
			int CUR_SECTION = 0;
			CG3::Set *CUR_SET = 0;
			CG3::CompositeTag *CUR_COMPTAG = 0;
			CG3::Tag *CUR_TAG = 0;
			
			result->lines = 1;

			while (!u_feof(input)) {
				previous = current;
				current = u_fgetcx(input);

				if (ux_isNewline(current, previous)) {
					if (MODE_PARENTHESES != 0) {
						std::cerr << "Warning: Mismatched number of parentheses " << MODE_PARENTHESES << " on line " << result->lines << std::endl;
						MODE_PARENTHESES = 0;
					}
					result->lines++;
					MODE_IGNORELINE = false;
					MODE_ESCAPE = false;
				}
				
				if ((current == ';' || current == '#') && !MODE_ESCAPE && !MODE_STRING) {
					if (current == ';') {
						MODE_COMMAND = K_IGNORE;
					} else {
						MODE_IGNORELINE = true;
					}
				}
				if (MODE_IGNORELINE || current == 0x000D) {
					continue;
				}
				if (!MODE_ESCAPE && u_isWhitespace(current)) {
					MODE_STRING = false;
					which16 = (which16+1)%NUM_BUFFERS;
					current16 = buffers16[which16];
					previous16 = buffers16[(NUM_BUFFERS+which16-1)%NUM_BUFFERS];
					u_strFromUTF32(current16, BUFFER_SIZE, NULL, buffer32, -1, uerror);
					if (current16[0] != '=' && u_strlen(current16)) {
						if (u_strcmp(current16, keywords[K_DELIMITERS]) == 0) {
							MODE_COMMAND = K_DELIMITERS;
						} else if (u_strcmp(current16, keywords[K_LIST]) == 0) {
							MODE_COMMAND = K_LIST;
							TOKENS_SINCE_CMD = 0;
							PREV_PARENTHESES = MODE_PARENTHESES;
						} else if (u_strcmp(current16, keywords[K_SET]) == 0) {
							MODE_COMMAND = K_SET;
						} else if (u_strcmp(current16, keywords[K_MAPPINGS]) == 0 || u_strcmp(current16, keywords[K_CORRECTIONS]) == 0) {
							CUR_SECTION = 0;
							MODE_COMMAND = K_IGNORE;
						} else if (u_strcmp(current16, keywords[K_CONSTRAINTS]) == 0 || u_strcmp(current16, keywords[K_SECTION]) == 0) {
							result->sections++;
							CUR_SECTION = result->sections;
							MODE_COMMAND = K_IGNORE;
						} else if (u_strcmp(current16, keywords[K_SETS]) == 0) {
							MODE_COMMAND = K_IGNORE;
						} else if (u_strcmp(current16, keywords[K_ANCHOR]) == 0) {
							MODE_COMMAND = K_ANCHOR;
						} else if (u_strcmp(current16, keywords[K_END]) == 0) {
							MODE_COMMAND = K_IGNORE;
							break;
						} else if (u_strcmp(current16, keywords[K_PREFERRED_TARGETS]) == 0) {
							MODE_COMMAND = K_PREFERRED_TARGETS;
						} else if (MODE_COMMAND == K_DELIMITERS) {
							result->addDelimiter(current16);
						} else if (MODE_COMMAND == K_PREFERRED_TARGETS) {
							result->addPreferredTarget(current16);
						} else if (MODE_COMMAND == K_LIST) {
							TOKENS_SINCE_CMD++;
							if (TOKENS_SINCE_CMD == 1) {
								if (CUR_SET) {
									result->addSet(CUR_SET);
									CUR_SET = 0;
								}
								CUR_SET = result->allocateSet();
								CUR_SET->setName(current16);
								CUR_SET->setLine(result->lines);
							} else {
								if (CUR_COMPTAG && MODE_PARENTHESES < PREV_PARENTHESES) {
									CUR_SET->addCompositeTag(CUR_COMPTAG);
									CUR_COMPTAG = 0;
									PREV_PARENTHESES = MODE_PARENTHESES;
								}
								if (!CUR_COMPTAG) {
									CUR_COMPTAG = CUR_SET->allocateCompositeTag();
								}
								CUR_TAG = CUR_COMPTAG->allocateTag(current16);
								CUR_COMPTAG->addTag(CUR_TAG);
								CUR_TAG = 0;
							}
						}
					} else {
						which16 = (NUM_BUFFERS+which16-1)%NUM_BUFFERS;
					}
					buffer_pos = 0;
					memset(buffer32, 0, sizeof(UChar32)*BUFFER_SIZE);
				} else if (!MODE_ESCAPE && current == '(') {
					if (MODE_COMMAND == K_IGNORE) {
						//std::cerr << "Warning: Invalid opening parenthesis on line " << (NUM_LINES+1) << " - ignoring it." << std::endl;
					} else {
						PREV_PARENTHESES = MODE_PARENTHESES;
						MODE_PARENTHESES++;
					}
				} else if (!MODE_ESCAPE && current == ')') {
					if (MODE_COMMAND == K_IGNORE) {
						//std::cerr << "Warning: Invalid closing parenthesis on line " << (NUM_LINES+1) << " - ignoring it." << std::endl;
					} else {
						PREV_PARENTHESES = MODE_PARENTHESES;
						MODE_PARENTHESES--;
					}
				} else {
					if (!MODE_ESCAPE && current == '\\') {
						MODE_ESCAPE = true;
					} else {
						buffer32[buffer_pos] = current;
						buffer_pos++;
						MODE_STRING = true;
						MODE_ESCAPE = false;
					}
				}
			}
			free_keywords();
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
