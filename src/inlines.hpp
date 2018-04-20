/*
* Copyright (C) 2007-2018, GrammarSoft ApS
* Developed by Tino Didriksen <mail@tinodidriksen.com>
* Design by Eckhard Bick <eckhard.bick@mail.dk>, Tino Didriksen <mail@tinodidriksen.com>
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
#ifndef c6d28b7452ec699b_INLINES_H
#define c6d28b7452ec699b_INLINES_H

namespace CG3 {

constexpr double NUMERIC_MIN = static_cast<double>(-(1ll << 48ll));
constexpr double NUMERIC_MAX = static_cast<double>((1ll << 48ll) - 1);
constexpr uint32_t CG3_HASH_SEED = 705577479u;

/*
	Paul Hsieh's SuperFastHash from http://www.azillionmonkeys.com/qed/hash.html
	This version adapted from online on 2008-12-30
//*/
#undef get16bits
#if (defined(__GNUC__) && defined(__i386__)) || defined(__WATCOMC__) \
	|| defined(_MSC_VER) || defined(__BORLANDC__) || defined(__TURBOC__)
#define get16bits(d) (*((const uint16_t*) (d)))
#endif

#if !defined (get16bits)
#define get16bits(d) ((((uint32_t)(((const uint8_t*)(d))[1])) << 8) \
					   +(uint32_t)(((const uint8_t*)(d))[0]) )
#endif

inline uint32_t SuperFastHash(const char* data, size_t len = 0, uint32_t hash = CG3_HASH_SEED) {
	if (hash == 0) {
		hash = static_cast<uint32_t>(len);
	}
	uint32_t tmp;
	uint32_t rem;

	if (len == 0 || data == 0) {
		return 0;
	}

	rem = len & 3;
	len >>= 2;

	/* Main loop */
	for (; len > 0; len--) {
		hash += get16bits(data);
		tmp = (get16bits(data + 2) << 11) ^ hash;
		hash = (hash << 16) ^ tmp;
		data += 2 * sizeof(uint16_t);
		hash += hash >> 11;
	}

	/* Handle end cases */
	switch (rem) {
	case 3:
		hash += get16bits(data);
		hash ^= hash << 16;
		hash ^= data[sizeof(uint16_t)] << 18;
		hash += hash >> 11;
		break;
	case 2:
		hash += get16bits(data);
		hash ^= hash << 11;
		hash += hash >> 17;
		break;
	case 1:
		hash += *data;
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

	if (hash == 0 || hash == std::numeric_limits<uint32_t>::max() || hash == std::numeric_limits<uint32_t>::max() - 1) {
		hash = CG3_HASH_SEED;
	}

	return hash;
}

inline uint32_t SuperFastHash(const UChar* data, size_t len = 0, uint32_t hash = CG3_HASH_SEED) {
	if (hash == 0) {
		hash = static_cast<uint32_t>(len);
	}
	uint32_t tmp;
	uint32_t rem;

	if (len == 0 || data == 0) {
		return 0;
	}

	rem = len & 1;
	len >>= 1;

	/* Main loop */
	for (; len > 0; len--) {
		hash += data[0];
		tmp = (data[1] << 11) ^ hash;
		hash = (hash << 16) ^ tmp;
		data += 2;
		hash += hash >> 11;
	}

	/* Handle end cases */
	switch (rem) {
	case 1:
		hash += data[0];
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

	if (hash == 0 || hash == std::numeric_limits<uint32_t>::max() || hash == std::numeric_limits<uint32_t>::max() - 1) {
		hash = CG3_HASH_SEED;
	}

	return hash;
}

inline uint32_t hash_value(const UChar* str, uint32_t hash = 0, size_t len = 0) {
	if (hash == 0) {
		hash = CG3_HASH_SEED;
	}
	if (len == 0) {
		len = u_strlen(str);
	}
	return SuperFastHash(str, len, hash);
}

inline uint32_t hash_value(const UString& str, uint32_t h = 0) {
	return hash_value(str.c_str(), h, str.length());
}

inline uint32_t hash_value(const char* str, uint32_t hash = 0, size_t len = 0) {
	if (hash == 0) {
		hash = CG3_HASH_SEED;
	}
	if (len == 0) {
		len = strlen(str);
	}
	return SuperFastHash(str, len, hash);
}

inline uint32_t hash_value(uint32_t c, uint32_t h = CG3_HASH_SEED) {
	if (h == 0) {
		h = CG3_HASH_SEED;
	}
	//*
	h = c + (h << 6U) + (h << 16U) - h;
	if (h == 0 || h == std::numeric_limits<uint32_t>::max() || h == std::numeric_limits<uint32_t>::max() - 1) {
		h = CG3_HASH_SEED;
	}
	return h;
	/*/
	uint32_t tmp = SuperFastHash(reinterpret_cast<const char*>(&c), sizeof(c), h);
	return tmp;
	//*/
}

inline uint32_t hash_value(uint64_t c) {
	/*
	uint32_t tmp = hash_value(static_cast<uint32_t>(c & 0xFFFFFFFF));
	tmp = hash_value(static_cast<uint32_t>((c >> 32) & 0xFFFFFFFF), tmp);
	return tmp;
	/*/
	uint32_t tmp = SuperFastHash(reinterpret_cast<const char*>(&c), sizeof(c));
	return tmp;
	//*/
}

struct hash_ustring {
	size_t operator()(const UString& str) const {
		return hash_value(str);
	}
};

inline bool ISSPACE(const UChar c) {
	if (c <= 0xFF && c != 0x09 && c != 0x0A && c != 0x0D && c != 0x20 && c != 0xA0) {
		return false;
	}
	return (c == 0x20 || c == 0x09 || c == 0x0A || c == 0x0D || c == 0xA0 || u_isWhitespace(c));
}

inline bool ISSTRING(const UChar* p, const uint32_t c) {
	if (*(p - 1) == '"' && *(p + c + 1) == '"') {
		return true;
	}
	if (*(p - 1) == '<' && *(p + c + 1) == '>') {
		return true;
	}
	return false;
}

inline bool ISNL(const UChar c) {
	return (
	  c == 0x2028L    // Unicode Line Seperator
	  || c == 0x2029L // Unicode Paragraph Seperator
	  || c == 0x000CL // Form Feed
	  || c == 0x000BL // Vertical Tab
	  || c == 0x000AL // ASCII \n
	);
}

inline bool ISESC(const UChar* p) {
	uint32_t a = 1;
	while (*(p - a) && *(p - a) == '\\') {
		a++;
	}
	return (a % 2 == 0);
}

inline bool ISSPACE(const UChar* p) {
	return ISSPACE(*p) && !ISESC(p);
}

template<typename C, size_t N>
inline bool IS_ICASE(const UChar* p, const C (&uc)[N], const C (&lc)[N]) {
	// N - 1 due to null terminator for string constants
	if (ISSTRING(p, N - 1)) {
		return false;
	}
	for (size_t i = 0; i < N - 1; ++i) {
		if (p[i] != uc[i] && p[i] != lc[i]) {
			return false;
		}
	}
	return true;
}

inline void BACKTONL(UChar*& p) {
	while (*p && !ISNL(*p) && (*p != ';' || ISESC(p))) {
		p--;
	}
	++p;
}

inline uint32_t SKIPLN(UChar*& p) {
	while (*p && !ISNL(*p)) {
		++p;
	}
	++p;
	return 1;
}

inline uint32_t SKIPWS(UChar*& p, const UChar a = 0, const UChar b = 0, const bool allowhash = false) {
	uint32_t s = 0;
	while (*p && *p != a && *p != b) {
		if (ISNL(*p)) {
			++s;
		}
		if (!allowhash && *p == '#' && !ISESC(p)) {
			s += SKIPLN(p);
			p--;
		}
		if (!ISSPACE(*p)) {
			break;
		}
		++p;
	}
	return s;
}

inline uint32_t SKIPTOWS(UChar*& p, const UChar a = 0, const bool allowhash = false, const bool allowscol = false) {
	uint32_t s = 0;
	while (*p && !ISSPACE(p)) {
		if (!allowhash && *p == '#' && !ISESC(p)) {
			s += SKIPLN(p);
			--p;
		}
		if (ISNL(*p)) {
			++s;
			++p;
		}
		if (!allowscol && *p == ';' && !ISESC(p)) {
			break;
		}
		if (*p == a && !ISESC(p)) {
			break;
		}
		++p;
	}
	return s;
}

inline uint32_t SKIPTO(UChar*& p, const UChar a) {
	uint32_t s = 0;
	while (*p && (*p != a || ISESC(p))) {
		if (ISNL(*p)) {
			++s;
		}
		++p;
	}
	return s;
}

inline void SKIPTO_NOSPAN(UChar*& p, const UChar a) {
	while (*p && (*p != a || ISESC(p))) {
		if (ISNL(*p)) {
			break;
		}
		++p;
	}
}

inline void SKIPTO_NOSPAN_RAW(UChar*& p, const UChar a) {
	while (*p && *p != a) {
		if (ISNL(*p)) {
			break;
		}
		++p;
	}
}

inline void CG3Quit(const int32_t c = 0, const char* file = 0, const uint32_t line = 0) {
	if (file && line) {
		std::cerr << std::flush;
		std::cerr << "CG3Quit triggered from " << file << " line " << line << "." << std::endl;
	}
	exit(c);
}

inline constexpr uint64_t make_64(uint32_t hi, uint32_t low) {
	return (static_cast<uint64_t>(hi) << 32) | static_cast<uint64_t>(low);
}

template<typename T, size_t N>
inline constexpr size_t size(T (&)[N]) {
	return N;
}

template<typename Cont, typename VT>
inline bool index_matches(const Cont& index, const VT& entry) {
	return (index.find(entry) != index.end());
}

inline void insert_if_exists(boost::dynamic_bitset<>& cont, const boost::dynamic_bitset<>* other) {
	if (other && !other->empty()) {
		cont.resize(std::max(cont.size(), other->size()));
		cont |= *other;
	}
}

template<typename S, typename T>
inline void writeRaw(S& stream, const T& value) {
	stream.write(reinterpret_cast<const char*>(&value), sizeof(T));
}

template<typename S, typename T>
inline void readRaw(S& stream, T& value) {
	stream.read(reinterpret_cast<char*>(&value), sizeof(T));
}

inline void writeUTF8String(std::ostream& output, const UChar* str, size_t len = 0) {
	if (len == 0) {
		len = u_strlen(str);
	}

	std::vector<char> buffer(len * 4);
	int32_t olen = 0;
	UErrorCode status = U_ZERO_ERROR;
	u_strToUTF8(&buffer[0], static_cast<int32_t>(len * 4 - 1), &olen, str, static_cast<int32_t>(len), &status);

	uint16_t cs = static_cast<uint16_t>(olen);
	writeRaw(output, cs);
	output.write(&buffer[0], cs);
}

inline void writeUTF8String(std::ostream& output, const UString& str) {
	writeUTF8String(output, str.c_str(), str.length());
}

template<typename S>
inline UString readUTF8String(S& input) {
	uint16_t len = 0;
	readRaw(input, len);

	UString rv(len, 0);
	std::vector<char> buffer(len);
	input.read(&buffer[0], len);

	int32_t olen = 0;
	UErrorCode status = U_ZERO_ERROR;
	u_strFromUTF8(&rv[0], len, &olen, &buffer[0], len, &status);

	rv.resize(olen);

	return rv;
}

#ifdef _MSC_VER
	// warning C4127: conditional expression is constant
	#pragma warning (disable: 4127)
#endif

template<typename T>
inline void writeSwapped(std::ostream& stream, const T& value) {
	if (sizeof(T) == 1) {
		stream.write(reinterpret_cast<const char*>(&value), sizeof(T));
	}
	else if (sizeof(T) == 2) {
		uint16_t tmp = static_cast<uint16_t>(htons(static_cast<uint16_t>(value)));
		stream.write(reinterpret_cast<const char*>(&tmp), sizeof(T));
	}
	else if (sizeof(T) == 4) {
		uint32_t tmp = static_cast<uint32_t>(htonl(static_cast<uint32_t>(value)));
		stream.write(reinterpret_cast<const char*>(&tmp), sizeof(T));
	}
	else if (sizeof(T) == 8) {
		uint64_t tmp = value;
#ifndef BIG_ENDIAN
		const uint32_t high = static_cast<uint32_t>(htonl(static_cast<uint32_t>(tmp >> 32)));
		const uint32_t low = static_cast<uint32_t>(htonl(static_cast<uint32_t>(tmp & 0xFFFFFFFFULL)));
		tmp = (static_cast<uint64_t>(low) << 32) | high;
#endif
		stream.write(reinterpret_cast<const char*>(&tmp), sizeof(T));
	}
	else {
		throw std::runtime_error("Unhandled type size in writeSwapped()");
	}
	if (!stream) {
		throw std::runtime_error("Stream was in bad state in writeSwapped()");
	}
}

template<>
inline void writeSwapped(std::ostream& stream, const double& value) {
	int exp = 0;
	uint64_t mant64 = static_cast<uint64_t>(std::numeric_limits<int64_t>::max() * frexp(value, &exp));
	uint32_t exp32 = static_cast<uint32_t>(exp);
	writeSwapped(stream, mant64);
	writeSwapped(stream, exp32);
}

template<typename T>
inline T readSwapped(std::istream& stream) {
	if (!stream) {
		throw std::runtime_error("Stream was in bad state in readSwapped()");
	}
	if (sizeof(T) == 1) {
		uint8_t tmp = 0;
		stream.read(reinterpret_cast<char*>(&tmp), sizeof(T));
		return static_cast<T>(tmp);
	}
	else if (sizeof(T) == 2) {
		uint16_t tmp = 0;
		stream.read(reinterpret_cast<char*>(&tmp), sizeof(T));
		return static_cast<T>(ntohs(tmp));
	}
	else if (sizeof(T) == 4) {
		uint32_t tmp = 0;
		stream.read(reinterpret_cast<char*>(&tmp), sizeof(T));
		return static_cast<T>(ntohl(tmp));
	}
	else if (sizeof(T) == 8) {
		uint64_t tmp = 0;
		stream.read(reinterpret_cast<char*>(&tmp), sizeof(T));
#ifndef BIG_ENDIAN
		const uint32_t high = static_cast<uint32_t>(ntohl(static_cast<uint32_t>(tmp >> 32)));
		const uint32_t low = static_cast<uint32_t>(ntohl(static_cast<uint32_t>(tmp & 0xFFFFFFFFULL)));
		tmp = (static_cast<uint64_t>(low) << 32) | high;
#endif
		return static_cast<T>(tmp);
	}
	throw std::runtime_error("Unhandled type size in readSwapped()");
}

template<>
inline double readSwapped(std::istream& stream) {
	uint64_t mant64 = readSwapped<uint64_t>(stream);
	int exp = static_cast<int>(readSwapped<int32_t>(stream));

	double value = static_cast<double>(static_cast<int64_t>(mant64)) / std::numeric_limits<int64_t>::max();

	return ldexp(value, exp);
}

#ifdef _MSC_VER
	// warning C4127: conditional expression is constant
	#pragma warning (default: 4127)
#endif

template<typename Cont>
inline void GAppSetOpts_ranged(const char* value, Cont& cont, bool fill = true) {
	cont.clear();
	bool had_range = false;

	const char* comma = value;
	do {
		uint32_t low = abs(atoi(comma)), high = low;
		const char* delim = strchr(comma, '-');
		const char* nextc = strchr(comma, ',');
		if (delim && (nextc == 0 || nextc > delim)) {
			had_range = true;
			high = abs(atoi(delim + 1));
		}
		for (; low <= high; ++low) {
			cont.push_back(low);
		}
	} while ((comma = strchr(comma, ',')) != 0 && ++comma && *comma != 0);

	if (cont.size() == 1 && !had_range && fill) {
		uint32_t val = cont.front();
		cont.clear();
		for (uint32_t i = 1; i <= val; ++i) {
			cont.push_back(i);
		}
	}
}

template<typename T>
class swapper {
public:
	swapper(bool cond, T& a, T& b)
	  : cond(cond)
	  , a(a)
	  , b(b)
	{
		if (cond) {
			std::swap(a, b);
		}
	}

	~swapper() {
		if (cond) {
			std::swap(a, b);
		}
	}

private:
	bool cond;
	T& a;
	T& b;
};

class swapper_false {
public:
	swapper_false(bool cond, bool& b)
	  : val(false)
	  , swp(cond, val, b)
	{}

private:
	bool val;
	swapper<bool> swp;
};

template<typename T>
class uncond_swap {
public:
	uncond_swap(T& a, T b)
	  : a_(a)
	  , b_(b)
	{
		std::swap(a_, b_);
	}

	~uncond_swap() {
		std::swap(a_, b_);
	}

private:
	T& a_;
	T b_;
};

template<typename T>
class inc_dec {
public:
	inc_dec()
	  : p(0)
	{}

	~inc_dec() {
		if (p) {
			--(*p);
		}
	}

	void inc(T& pt) {
		p = &pt;
		++(*p);
	}

private:
	T* p;
};

template<typename T>
inline T* reverse(T* head) {
	T* nr = 0;
	while (head) {
		T* next = head->next;
		head->next = nr;
		nr = head;
		head = next;
	}
	return nr;
}

template<typename Cont, typename T>
inline void erase(Cont& cont, const T& val) {
	cont.erase(std::remove(cont.begin(), cont.end(), val), cont.end());
}

inline size_t fread_throw(void* buffer, size_t size, size_t count, FILE* stream) {
	size_t rv = ::fread(buffer, size, count, stream);
	if (rv != count) {
		throw std::runtime_error("fread() did not read all requested objects");
	}
	return rv;
}

inline size_t fread_throw(void* buffer, size_t size, size_t count, std::istream& stream) {
	if (!stream.read(static_cast<char*>(buffer), size * count)) {
		throw std::runtime_error("stream did not read all requested objects");
	}
	return size * count;
}

inline size_t fwrite_throw(const void* buffer, size_t size, size_t count, FILE* stream) {
	size_t rv = ::fwrite(buffer, size, count, stream);
	if (rv != count) {
		throw std::runtime_error("fwrite() did not write all requested objects");
	}
	return rv;
}

template<typename Pool, typename Var>
void pool_get(Pool& pool, Var& var) {
	if (!pool.empty()) {
		var.swap(pool.back());
		var.clear();
		pool.pop_back();
	}
}

template<typename Pool, typename Var>
void pool_put(Pool& pool, Var& var) {
	pool.resize(pool.size() + 1);
	var.swap(pool.back());
}

template<typename Pool, typename Var>
void pool_get(Pool& pool, Var*& var) {
	var = 0;
	if (!pool.empty()) {
		var = pool.back();
		pool.pop_back();
	}
}

template<typename Pool>
typename Pool::value_type pool_get(Pool& pool) {
	typename Pool::value_type var = 0;
	if (!pool.empty()) {
		var = pool.back();
		pool.pop_back();
	}
	return var;
}

template<typename Pool, typename Var>
void pool_put(Pool& pool, Var* var) {
	var->clear();
	pool.push_back(var);
}

template<typename Pool>
struct pool_cleaner {
	Pool& pool;

	pool_cleaner(Pool& pool)
	  : pool(pool)
	{
	}

	~pool_cleaner() {
		for (size_t i = 0; i < pool.size(); ++i) {
			delete pool[i];
		}
	}
};
}

#endif
