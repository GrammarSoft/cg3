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

#ifndef __INLINES_H
#define __INLINES_H

inline uint32_t hash_sdbm_uchar(const UChar *str, uint32_t hash = 0) {
	if (hash == 0) {
		hash = 705577479U;
	}
    UChar c = 0;

	while ((c = *str++) != 0) {
        hash = c + (hash << 6U) + (hash << 16U) - hash;
	}

    return hash;
}

inline uint32_t hash_sdbm_char(const char *str, uint32_t hash = 0) {
	if (hash == 0) {
		hash = 705577479U;
	}
    UChar c = 0;

	while ((c = *str++) != 0) {
        hash = c + (hash << 6U) + (hash << 16U) - hash;
	}

    return hash;
}

inline uint32_t hash_sdbm_uint32_t(const uint32_t c, uint32_t hash = 0) {
	if (hash == 0) {
		hash = 705577479U;
	}
    hash = c + (hash << 6U) + (hash << 16U) - hash;
    return hash;
}

inline bool ISSTRING(UChar *p, uint32_t c) {
	if (*(p-1) == '"' && *(p+c+1) == '"') {
		return true;
	}
	if (*(p-1) == '<' && *(p+c+1) == '>') {
		return true;
	}
	return false;
}

inline bool ISNL(const UChar c) {
	return (
	   c == 0x2028L // Unicode Line Seperator
	|| c == 0x2029L // Unicode Paragraph Seperator
	|| c == 0x0085L // EBCDIC NEL
	|| c == 0x000CL // Form Feed
	|| c == 0x000AL // ASCII \n
	);
}

inline bool ISESC(UChar *p) {
	uint32_t a=1;
	while(*(p-a) && *(p-a) == '\\') {
		a++;
	}
	return (a%2==0);
}

inline bool ISCHR(const UChar p, const UChar a, const UChar b) {
	return ((p) && ((p) == (a) || (p) == (b)));
}

inline void BACKTONL(UChar **p) {
	while (**p && !ISNL(**p) && (**p != ';' || ISESC(*p))) {
		(*p)--;
	}
	(*p)++;
}

inline uint32_t SKIPLN(UChar **p) {
	while (**p && !ISNL(**p)) {
		(*p)++;
	}
	(*p)++;
	return 1;
}

inline uint32_t SKIPWS(UChar **p, const UChar a = 0, const UChar b = 0) {
	uint32_t s = 0;
	while (**p && **p != a && **p != b) {
		if (ISNL(**p)) {
			s++;
		}
		if (**p == '#' && !ISESC(*p)) {
			s += SKIPLN(p);
			(*p)--;
		}
		if (!u_isWhitespace(**p)) {
			break;
		}
		(*p)++;
	}
	return s;
}

inline uint32_t SKIPTOWS(UChar **p, const UChar a = 0, const bool allowhash = false) {
	uint32_t s = 0;
	while (**p && !u_isWhitespace(**p)) {
		if (!allowhash && **p == '#' && !ISESC(*p)) {
			s += SKIPLN(p);
			(*p)--;
		}
		if (ISNL(**p)) {
			s++;
			(*p)++;
		}
		if (**p == ';' && !ISESC(*p)) {
			break;
		}
		if (**p == a && !ISESC(*p)) {
			break;
		}
		(*p)++;
	}
	return s;
}

inline uint32_t SKIPTO(UChar **p, const UChar a) {
	uint32_t s = 0;
	while (**p && (**p != a || ISESC(*p))) {
		if (ISNL(**p)) {
			s++;
		}
		(*p)++;
	}
	return s;
}

inline uint32_t SKIPTO_NOSPAN(UChar **p, const UChar a) {
	uint32_t s = 0;
	while (**p && (**p != a || ISESC(*p))) {
		if (ISNL(**p)) {
			break;
		}
		(*p)++;
	}
	return s;
}

#endif
