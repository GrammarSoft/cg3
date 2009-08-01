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

#pragma once
#ifndef __INLINES_H
#define __INLINES_H

namespace CG3 {

#define CG3_HASH_SEED 705577479

inline uint32_t hash_sdbm_uint32_t(const uint32_t c, uint32_t hash = 0) {
	if (hash == 0) {
		hash = CG3_HASH_SEED;
	}
    hash = c + (hash << 6U) + (hash << 16U) - hash;
    return hash;
}

/*
	Paul Hsieh's SuperFastHash from http://www.azillionmonkeys.com/qed/hash.html
	This version adapted from online on 2008-12-30
//*/
#undef get16bits
#if (defined(__GNUC__) && defined(__i386__)) || defined(__WATCOMC__) \
	|| defined(_MSC_VER) || defined(__BORLANDC__) || defined(__TURBOC__)
#define get16bits(d) (*((const uint16_t *) (d)))
#endif

#if !defined (get16bits)
#define get16bits(d) ((((uint32_t)(((const uint8_t *)(d))[1])) << 8) \
                       +(uint32_t)(((const uint8_t *)(d))[0]) )
#endif

inline uint32_t SuperFastHash_char(const char *data, uint32_t hash = CG3_HASH_SEED, uint32_t len = 0) {
	uint32_t tmp;
	uint32_t rem;

	if (len == 0 || data == 0) {
		return 0;
	}

	rem = len & 3;
	len >>= 2;

	/* Main loop */
	for (;len > 0; len--) {
		hash  += get16bits (data);
		tmp    = (get16bits (data+2) << 11) ^ hash;
		hash   = (hash << 16) ^ tmp;
		data  += 2*sizeof (uint16_t);
		hash  += hash >> 11;
	}

	/* Handle end cases */
	switch (rem) {
		case 3:	hash += get16bits (data);
				hash ^= hash << 16;
				hash ^= data[sizeof (uint16_t)] << 18;
				hash += hash >> 11;
				break;
		case 2:	hash += get16bits (data);
				hash ^= hash << 11;
				hash += hash >> 17;
				break;
		case 1: hash += *data;
				hash ^= hash << 10;
				hash += hash >> 1;
	}

	/* Force "avalanching" of final 127 bits */
	hash ^= hash << 3;
	hash += hash >> 5;
	hash ^= hash << 4;
	hash += hash >> 17;
	hash ^= hash << 25;
	hash += hash >> 6;

	return hash;
}

inline uint32_t SuperFastHash_uchar(const UChar *data, uint32_t hash = CG3_HASH_SEED, uint32_t len = 0) {
	uint32_t tmp;
	uint32_t rem;

	if (len == 0 || data == 0) {
		return 0;
	}

	rem = len & 1;
	len >>= 1;

	/* Main loop */
	for (;len > 0; len--) {
		hash  += data[0];
		tmp    = (data[1] << 11) ^ hash;
		hash   = (hash << 16) ^ tmp;
		data  += 2;
		hash  += hash >> 11;
	}

	/* Handle end cases */
	switch (rem) {
		case 1:	hash += data[0];
				hash ^= hash << 11;
				hash += hash >> 17;
				break;
	}

	/* Force "avalanching" of final 127 bits */
	hash ^= hash << 3;
	hash += hash >> 5;
	hash ^= hash << 4;
	hash += hash >> 17;
	hash ^= hash << 25;
	hash += hash >> 6;

	return hash;
}

inline uint32_t hash_sdbm_uchar(const UChar *str, uint32_t hash = 0, size_t len = 0) {
	if (hash == 0) {
		hash = CG3_HASH_SEED;
	}
	if (len == 0) {
		len = u_strlen(str);
	}
	return SuperFastHash_uchar(str, hash, len);
}

inline uint32_t hash_sdbm_char(const char *str, uint32_t hash = 0, size_t len = 0) {
	if (hash == 0) {
		hash = CG3_HASH_SEED;
	}
	if (len == 0) {
		len = strlen(str);
	}
	return SuperFastHash_char(str, hash, len);
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

inline bool ISESC(const UChar *p) {
	uint32_t a=1;
	while (*(p-a) && *(p-a) == '\\') {
		a++;
	}
	return (a%2==0);
}

inline bool ISCHR(const UChar p, const UChar a, const UChar b) {
	return ((p) && ((p) == (a) || (p) == (b)));
}

inline void BACKTONL(UChar *& p) {
	while (*p && !ISNL(*p) && (*p != ';' || ISESC(p))) {
		p--;
	}
	p++;
}

inline uint32_t SKIPLN(UChar *& p) {
	while (*p && !ISNL(*p)) {
		p++;
	}
	p++;
	return 1;
}

inline uint32_t SKIPWS(UChar *& p, const UChar a = 0, const UChar b = 0) {
	uint32_t s = 0;
	while (*p && *p != a && *p != b) {
		if (ISNL(*p)) {
			s++;
		}
		if (*p == '#' && !ISESC(p)) {
			s += SKIPLN(p);
			p--;
		}
		if (!u_isWhitespace(*p)) {
			break;
		}
		p++;
	}
	return s;
}

inline uint32_t SKIPTOWS(UChar *& p, const UChar a = 0, const bool allowhash = false) {
	uint32_t s = 0;
	while (*p && !u_isWhitespace(*p)) {
		if (!allowhash && *p == '#' && !ISESC(p)) {
			s += SKIPLN(p);
			p--;
		}
		if (ISNL(*p)) {
			s++;
			p++;
		}
		if (*p == ';' && !ISESC(p)) {
			break;
		}
		if (*p == a && !ISESC(p)) {
			break;
		}
		p++;
	}
	return s;
}

inline uint32_t SKIPTO(UChar *& p, const UChar a) {
	uint32_t s = 0;
	while (*p && (*p != a || ISESC(p))) {
		if (ISNL(*p)) {
			s++;
		}
		p++;
	}
	return s;
}

inline uint32_t SKIPTO_NOSPAN(UChar *& p, const UChar a) {
	uint32_t s = 0;
	while (*p && (*p != a || ISESC(p))) {
		if (ISNL(*p)) {
			break;
		}
		p++;
	}
	return s;
}

inline void CG3Quit(const int32_t c = 0, const char* file = 0, const uint32_t line = 0) {
	if (file && line) {
		std::cerr << std::flush;
		std::cerr << "CG3Quit triggered from " << file << " line " << line << "." << std::endl;
	}
	exit(c);
}
// #define CG3Quit(a) CG3Quit((a), __FILE__, __LINE__)

}

#endif
