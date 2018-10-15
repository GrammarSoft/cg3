#ifndef c6d28b7452ec699b_GETOPT_H
#define c6d28b7452ec699b_GETOPT_H

#include <stdafx.hpp>
#include <stdio.h>
#include <string.h>

namespace CG3_GetOpt {

int getopt(int argc, char** argv, const char* opts);

CG3_IMPORTS extern int opterr;
CG3_IMPORTS extern int optind;
CG3_IMPORTS extern int optopt;
CG3_IMPORTS extern char *optarg;

}
using namespace CG3_GetOpt;

#endif
