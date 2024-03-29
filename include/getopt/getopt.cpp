/*
Newsgroups: mod.std.unix
Subject: public domain AT&T getopt source
Date: 3 Nov 85 19:34:15 GMT

Here's something you've all been waiting for:  the AT&T public domain
source for getopt(3).  It is the code which was given out at the 1985
UNIFORUM conference in Dallas.  I obtained it by electronic mail
directly from AT&T.  The people there assure me that it is indeed
in the public domain.
*/

#include "getopt.h"

/*LINTLIBRARY*/
namespace CG3_GetOpt {

#define EOF	(-1)
#define ERR(s, c)	if (opterr){\
	char errbuf[2]{c, '\n'};\
	(void) fwrite(argv[0], (unsigned)strlen(argv[0]), 1, stderr);\
	(void) fwrite(s, (unsigned)strlen(s), 1, stderr);\
	(void) fwrite(errbuf, 2, 1, stderr);}

int	opterr = 1;
int	optind = 1;
int	optopt;
char	*optarg;

int getopt(int argc, char **argv, const char *opts) {
	static thread_local int sp = 1;
	int c;
	const char *cp;

	if (sp == 1)
		if (optind >= argc ||
		   argv[optind][0] != '-' || argv[optind][1] == '\0')
			return(EOF);
		else if (strcmp(argv[optind], "--") == 0) {
			optind++;
			return(EOF);
		}
	optopt = c = argv[optind][sp];
	if (c == ':' || (cp=strchr(opts, c)) == 0) {
		ERR(": illegal option -- ", (char)c);
		if (argv[optind][++sp] == '\0') {
			optind++;
			sp = 1;
		}
		return('?');
	}
	if (*++cp == ':') {
		if (argv[optind][sp+1] != '\0')
			optarg = &argv[optind++][sp+1];
		else if (++optind >= argc) {
			ERR(": option requires an argument -- ", (char)c);
			sp = 1;
			return('?');
		}
		else {
			optarg = argv[optind++];
		}
		sp = 1;
	}
	else {
		if (argv[optind][++sp] == '\0') {
			sp = 1;
			optind++;
		}
		optarg = nullptr;
	}
	return(c);
}

}
