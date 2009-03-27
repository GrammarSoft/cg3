/*
* Copyright (C) 2007, GrammarSoft ApS
* Developed by Tino Didriksen <tino@didriksen.cc>
* Design by Eckhard Bick <eckhard.bick@mail.dk>, Tino Didriksen <tino@didriksen.cc>
*
* This file is part of VISL CG-3
*
* VISL CG-3 is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* VISL CG-3 is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with VISL CG-3.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "stdafx.h"

int main(int argc, char* argv[]) {
	UErrorCode status = U_ZERO_ERROR;

	/* Initialize ICU */
	u_init(&status);
	if (U_FAILURE(status) && status != U_FILE_ACCESS_ERROR) {
		std::cerr << "Error: Cannot initialize ICU. Status = " << u_errorName(status) << std::endl;
		CG3Quit(1);
	}
	status = U_ZERO_ERROR;

	ucnv_setDefaultName("UTF-8");
	uloc_setDefault("en_US_POSIX", &status);

	char *s = new char[512];
	memset(s, 0, sizeof(char)*512);
	sprintf(s, "waffles with pancakes");
	printf("Hash char : %u %u %u %u\n", sizeof(char), s[0], s[1], hash_sdbm_char(s));

	UChar *t = new UChar[512];
	memset(t, 0, sizeof(UChar)*512);
	u_sprintf(t, "waffles with pancakes");
	char *c = reinterpret_cast<char*>(t);
	printf("Hash UChar: %u %u %u %u\n", sizeof(UChar), c[0], c[1], hash_sdbm_uchar(t));

	u_cleanup();

	return status;
}
