/*
* Copyright (C) 2007-2024, GrammarSoft ApS
* Developed by Tino Didriksen <mail@tinodidriksen.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this progam.  If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once
#ifndef c6d28b7452ec699b_FLAT_UNORDERED_SET_HPP
#define c6d28b7452ec699b_FLAT_UNORDERED_SET_HPP
#include <cassert>
#include <iostream>
#include <vector>
#include <algorithm>
#include <functional>
#include <cstdint>

namespace CG3 {

template<typename T, T res_empty = T(-1), T res_del = T(-1) - 1>
class flat_unordered_set {
public:
	class const_iterator {
	private:
		friend class flat_unordered_set;
		const flat_unordered_set* fus;
		size_t i;

	public:
		using iterator_category = std::bidirectional_iterator_tag;
		using value_type = T;
		using difference_type = ptrdiff_t;
		using pointer = T*;
		using reference = value_type;

		const_iterator()
		  : fus(nullptr)
		  , i(0)
		{
		}

		const_iterator(const const_iterator& o)
		  : fus(o.fus)
		  , i(o.i)
		{
		}

		const_iterator(const flat_unordered_set& fus, size_t i = 0)
		  : fus(&fus)
		  , i(i)
		{
		}

		const_iterator& operator=(const const_iterator& o) {
			fus = o.fus;
			i = o.i;
			return *this;
		}

		const_iterator& operator++() {
			for (++i; i < fus->capacity(); ++i) {
				if (fus->elements[i] != res_empty && fus->elements[i] != res_del) {
					break;
				}
			}
			if (i >= fus->capacity()) {
				fus = nullptr;
				i = 0;
			}
			return *this;
		}

		const_iterator operator++(int) {
			const_iterator tmp(*this);
			operator++();
			return tmp;
		}

		const_iterator& operator--() {
			if (i == 0) {
				fus = nullptr;
				i = 0;
			}
			else {
				for (--i; i > 0; --i) {
					if (fus->elements[i] != res_empty && fus->elements[i] != res_del) {
						break;
					}
				}
			}
			return *this;
		}

		bool operator==(const const_iterator& o) const {
			return fus == o.fus && i == o.i;
		}

		bool operator!=(const const_iterator& o) const {
			return !(*this == o);
		}

		T operator*() const {
			return fus->elements[i];
		}
	};

	using iterator = const_iterator;
	using container = typename std::vector<T>;
	using size_type = typename container::size_type;
	using value_type = T;
	using key_type = T;
	enum {
		DEFAULT_CAP = static_cast<size_type>(16u),
	};

	void insert(T t) {
		assert(t != res_empty && t != res_del && "Value cannot be res_empty or res_del!");

		if (deleted && size_ + deleted == capacity()) {
			reserve(capacity());
		}

		if ((size_ + 1) * 3 / 2 >= capacity() / 2) {
			reserve(std::max(static_cast<size_type>(DEFAULT_CAP), capacity() * 2));
		}
		size_t max = capacity() - 1;
		size_t spot = hash_value(t) & max;
		while (elements[spot] != res_empty && elements[spot] != t) {
			spot = hash_value_sz(spot) & max;
		}
		if (elements[spot] != t) {
			elements[spot] = t;
			++size_;
		}
	}

	template<typename It>
	void insert(It b, It e) {
		size_t d = std::distance(b, e);
		size_t c = capacity();
		while ((size_ + d) * 3 / 2 >= c / 2) {
			c = std::max(static_cast<size_type>(DEFAULT_CAP), c * 2);
		}
		if (c != capacity()) {
			reserve(c);
		}

		for (; b != e; ++b) {
			insert(*b);
		}
	}

	void erase(T t) {
		assert(t != res_empty && t != res_del && "Value cannot be res_empty or res_del!");

		if (size_ == 0) {
			return;
		}
		size_t max = capacity() - 1;
		size_t spot = hash_value(t) & max;
		while (elements[spot] != res_empty && elements[spot] != t) {
			spot = hash_value_sz(spot) & max;
		}
		if (elements[spot] == t) {
			elements[spot] = res_del;
			--size_;
			if (size_ == 0 && deleted != 0) {
				clear();
			}
			else {
				++deleted;
			}
		}
	}

	const_iterator erase(const_iterator it) {
		elements[it.i] = res_del;
		++it;
		--size_;
		if (size_ == 0 && deleted != 0) {
			clear();
		}
		else {
			++deleted;
		}
		return it;
	}

	const_iterator find(T t) const {
		assert(t != res_empty && t != res_del && "Value cannot be res_empty or res_del!");

		const_iterator it;

		if (size_) {
			size_t max = capacity() - 1;
			size_t spot = hash_value(t) & max;
			for (size_t i = 0; i < capacity() * 4 && elements[spot] != res_empty && elements[spot] != t; ++i) {
				spot = hash_value_sz(spot) & max;
			}
			if (elements[spot] == t) {
				it.fus = this;
				it.i = spot;
			}
		}

		return it;
	}

	const_iterator find(T t) {
		if (deleted && size_ + deleted == capacity()) {
			reserve(capacity());
		}

		return const_cast<const flat_unordered_set*>(this)->find(t);
	}

	size_t count(T t) const {
		return (find(t) != end());
	}

	bool contains(T t) const {
		return (find(t) != end());
	}

	const_iterator begin() const {
		if (size_ == 0) {
			return end();
		}
		for (size_t i = 0, ie = capacity(); i < ie; ++i) {
			if (elements[i] != res_empty && elements[i] != res_del) {
				return const_iterator(*this, i);
			}
		}
		return end();
	}

	const_iterator end() const {
		return const_iterator();
	}

	size_type size() const {
		return size_;
	}

	size_type capacity() const {
		return elements.size();
	}

	void reserve(size_type n) {
		if (size_ == 0) {
			elements.resize(n, res_empty);
			deleted = 0;
			return;
		}

		static thread_local container vals;
		vals.resize(0);
		vals.reserve(size_);
		for (auto& elem : elements) {
			if (elem != res_empty && elem != res_del) {
				vals.push_back(elem);
			}
		}

		clear(n);
		size_ = vals.size();
		size_t max = capacity() - 1;
		for (auto& val : vals) {
			size_t spot = hash_value(val) & max;
			while (elements[spot] != res_empty && elements[spot] != val) {
				spot = hash_value_sz(spot) & max;
			}
			elements[spot] = val;
		}
	}

	bool empty() const {
		return (size_ == 0);
	}

	template<typename It>
	void assign(It b, It e) {
		clear();
		insert(b, e);
	}

	void swap(flat_unordered_set& other) {
		std::swap(size_, other.size_);
		std::swap(deleted, other.deleted);
		elements.swap(other.elements);
	}

	void clear(size_type n = 0) {
		size_ = elements.size();
		elements.resize(0);
		elements.resize(std::max(size_, n), res_empty);
		size_ = 0;
		deleted = 0;
	}

	container& get() {
		return elements;
	}

private:
	size_type size_ = 0;
	size_type deleted = 0;
	container elements;

	size_type hash_value_sz(size_type t) const {
		return t * 3663850746527583589ull + 11210403176660999867ull;
	}

	size_type hash_value(T t) const {
		return hash_value_sz(static_cast<size_type>(t));
	}

	friend class const_iterator;
};

using uint32FlatHashSet = flat_unordered_set<uint32_t>;
using uint64FlatHashSet = flat_unordered_set<uint64_t>;
}

#endif
