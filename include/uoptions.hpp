/*
*******************************************************************************
*
*   Copyright (C) 2000, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*   file name:  uoptions.h
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 2000apr17
*   created by: Markus W. Scherer
*
*   This file provides a command line argument parser.
*/

#pragma once
#ifndef c6d28b7452ec699b_UOPTIONS_H__
#define c6d28b7452ec699b_UOPTIONS_H__

#include <string.h>
#include <string>
#include <cstdint>

namespace Options {

/* values of UOption.hasArg */
enum : uint8_t { UOPT_NO_ARG, UOPT_REQUIRES_ARG, UOPT_OPTIONAL_ARG };

/* structure describing a command line option */
struct UOption {
	const char *longName = nullptr; /* "foo" for --foo */
	char shortName = 0;             /* 'f' for -f */
	uint8_t hasArg = UOPT_NO_ARG;   /* enum value: option takes no/requires/may have argument */
	std::string description;        /* description of the option (added by Tino Didriksen <mail@tinodidriksen.com>) */
	bool doesOccur = false;         /* boolean for "this one occured" */
	std::string value;              /* output placeholder, will point to the argument string, if any */
};

/**
 * C Command line argument parser.
 *
 * This function takes the argv[argc] command line and a description of
 * the program's options in form of an array of UOption structures.
 * Each UOption defines a long and a short name (a string and a character)
 * for options like "--foo" and "-f".
 *
 * Each option is marked with whether it does not take an argument,
 * requires one, or optionally takes one. The argument may follow in
 * the same argv[] entry for short options, or it may always follow
 * in the next argv[] entry.
 *
 * An argument is in the next argv[] entry for both long and short name
 * options, except it is taken from directly behind the short name in
 * its own argv[] entry if there are characters following the option letter.
 * An argument in its own argv[] entry must not begin with a '-'
 * unless it is only the '-' itself. There is no restriction of the
 * argument format if it is part of the short name options's argv[] entry.
 *
 * The argument is stored in the value field of the corresponding
 * UOption entry, and the doesOccur field is set to 1 if the option
 * is found at all.
 *
 * Short name options without arguments can be collapsed into a single
 * argv[] entry. After an option letter takes an argument, following
 * letters will be taken as its argument.
 *
 * If the same option is found several times, then the last
 * argument value will be stored in the value field.
 *
 * For each option, a function can be called. This could be used
 * for options that occur multiple times and all arguments are to
 * be collected.
 *
 * All options are removed from the argv[] array itself. If the parser
 * is successful, then it returns the number of remaining non-option
 * strings (including argv[0]).
 * argv[0], the program name, is never read or modified.
 *
 * An option "--" ends option processing; everything after this
 * remains in the argv[] array.
 *
 * An option string "-" alone is treated as a non-option.
 *
 * If an option is not recognized or an argument missing, then
 * the parser returns with the negative index of the argv[] entry
 * where the error was detected.
 *
 * The OS/400 compiler requires that argv either be "char* argv[]",
 * or "const char* const argv[]", and it will not accept, 
 * "const char* argv[]" as a definition for main().
 *
 * @param argv This parameter is modified
 * @param options This parameter is modified
 */
inline int u_parseArgs(int argc, char* argv[], int optionCount, UOption options[]) {
	char *arg;
	int i = 1, remaining = 1;
	char c, stopOptions = 0;

	while (i < argc) {
		arg = argv[i];
		if (!stopOptions && *arg == '-' && (c = arg[1]) != 0) {
			/* process an option */
			UOption *option = NULL;
			arg += 2;
			if (c == '-') {
				/* process a long option */
				if (*arg == 0) {
					/* stop processing options after "--" */
					stopOptions = 1;
				}
				else {
					/* search for the option string */
					int j;
					for (j = 0; j < optionCount; ++j) {
						if (options[j].longName && strcmp(arg, options[j].longName) == 0) {
							option = options + j;
							break;
						}
					}
					if (option == NULL) {
						/* no option matches */
						return -i;
					}
					option->doesOccur = 1;

					if (option->hasArg != UOPT_NO_ARG) {
						/* parse the argument for the option, if any */
						if (i + 1 < argc && !(argv[i + 1][0] == '-' && argv[i + 1][1] != 0)) {
							/* argument in the next argv[], and there is not an option in there */
							option->value = argv[++i];
						}
						else if (option->hasArg == UOPT_REQUIRES_ARG) {
							/* there is no argument, but one is required: return with error */
							return -i;
						}
					}
				}
			}
			else {
				/* process one or more short options */
				do {
					/* search for the option letter */
					int j;
					for (j = 0; j < optionCount; ++j) {
						if (c == options[j].shortName) {
							option = options + j;
							break;
						}
					}
					if (option == NULL) {
						/* no option matches */
						return -i;
					}
					option->doesOccur = 1;

					if (option->hasArg != UOPT_NO_ARG) {
						/* parse the argument for the option, if any */
						if (*arg != 0) {
							/* argument following in the same argv[] */
							option->value = arg;
							/* do not process the rest of this arg as option letters */
							break;
						}
						else if (i + 1 < argc && !(argv[i + 1][0] == '-' && argv[i + 1][1] != 0)) {
							/* argument in the next argv[], and there is not an option in there */
							option->value = argv[++i];
							/* this break is redundant because we know that *arg==0 */
							break;
						}
						else if (option->hasArg == UOPT_REQUIRES_ARG) {
							/* there is no argument, but one is required: return with error */
							return -i;
						}
					}

					/* get the next option letter */
					option = NULL;
					c = *arg++;
				} while (c != 0);
			}

			/* go to next argv[] */
			++i;
		}
		else {
			/* move a non-option up in argv[] */
			argv[remaining++] = arg;
			++i;
		}
	}
	return remaining;
}

}
#endif
