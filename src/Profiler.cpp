/*
* Copyright (C) 2007-2023, GrammarSoft ApS
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

#include "Profiler.hpp"
#include "stdafx.hpp"
#include <sqlite3.h>

namespace CG3 {

inline auto sqlite3_exec(sqlite3* db, const char* sql) {
	return ::sqlite3_exec(db, sql, nullptr, nullptr, nullptr);
}

void Profiler::write(const char* fname) {
	if (sqlite3_initialize() != SQLITE_OK) {
		throw std::runtime_error("sqlite3_initialize() errored");
	}

	remove(fname);

	sqlite3* db = nullptr;
	sqlite3_stmt* s = nullptr;
	if (sqlite3_open_v2(fname, &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr) != SQLITE_OK) {
		throw std::runtime_error(concat("sqlite3_open_v2() error: ", sqlite3_errmsg(db)));
	}

	auto inits = {
		"PRAGMA journal_mode = MEMORY",
		"PRAGMA locking_mode = EXCLUSIVE",
		"PRAGMA synchronous = OFF",
		"CREATE TABLE strings (key INTEGER PRIMARY KEY NOT NULL, value TEXT NOT NULL)",
		"CREATE TABLE grammars (fname INTEGER PRIMARY KEY NOT NULL, grammar INTEGER NOT NULL)",
		"CREATE TABLE entries (type INTEGER NOT NULL, id INTEGER NOT NULL, grammar INTEGER NOT NULL, b INTEGER NOT NULL, e INTEGER NOT NULL, num_match INTEGER NOT NULL, num_fail INTEGER NOT NULL, example_window INTEGER NOT NULL, PRIMARY KEY (type, id))",
		"CREATE TABLE rule_contexts (rule INTEGER NOT NULL, context INTEGER NOT NULL, num_match INTEGER NOT NULL, PRIMARY KEY (rule, context))",
		"BEGIN",
	};
	for (auto q : inits) {
		if (sqlite3_exec(db, q) != SQLITE_OK) {
			throw std::runtime_error(concat("sqlite3 error while initializing database: ", sqlite3_errmsg(db)));
		}
	}

	// Strings
	if (sqlite3_prepare_v2(db, "INSERT INTO strings (key, value) VALUES (:key, :value)", -1, &s, nullptr) != SQLITE_OK) {
		throw std::runtime_error(concat("sqlite3 error preparing insert into strings table: ", sqlite3_errmsg(db)));
	}
	for (auto& it : strings) {
		sqlite3_reset(s);
		auto sz = it.second;
		if (sz == grammar_ast) {
			sz = 0;
		}
		if (sqlite3_bind_int64(s, 1, sz) != SQLITE_OK) {
			throw std::runtime_error(concat("sqlite3 error trying to bind int64 for key: ", sqlite3_errmsg(db)));
		}
		if (sqlite3_bind_text(s, 2, it.first.c_str(), SI32(it.first.size()), SQLITE_STATIC) != SQLITE_OK) {
			throw std::runtime_error(concat("sqlite3 error trying to bind text for value: ", sqlite3_errmsg(db)));
		}
		if (sqlite3_step(s) != SQLITE_DONE) {
			throw std::runtime_error(concat("sqlite3 error inserting into strings table: ", sqlite3_errmsg(db)));
		}
	}
	sqlite3_finalize(s);

	// Grammars
	if (sqlite3_prepare_v2(db, "INSERT INTO grammars (fname, grammar) VALUES (:fname, :grammar)", -1, &s, nullptr) != SQLITE_OK) {
		throw std::runtime_error(concat("sqlite3 error preparing insert into grammars table: ", sqlite3_errmsg(db)));
	}
	for (auto& it : grammars) {
		sqlite3_reset(s);
		if (sqlite3_bind_int64(s, 1, it.first) != SQLITE_OK) {
			throw std::runtime_error(concat("sqlite3 error trying to bind int64 for fname: ", sqlite3_errmsg(db)));
		}
		if (sqlite3_bind_int64(s, 2, it.second) != SQLITE_OK) {
			throw std::runtime_error(concat("sqlite3 error trying to bind int64 for grammar: ", sqlite3_errmsg(db)));
		}
		if (sqlite3_step(s) != SQLITE_DONE) {
			throw std::runtime_error(concat("sqlite3 error inserting into grammars table: ", sqlite3_errmsg(db)));
		}
	}
	sqlite3_finalize(s);

	// Entries
	if (sqlite3_prepare_v2(db, "INSERT INTO entries (type, id, grammar, b, e, num_match, num_fail, example_window) VALUES(:type, :id, :grammar, :b, :e, :num_match, :num_fail, :example_window)", -1, &s, nullptr) != SQLITE_OK) {
		throw std::runtime_error(concat("sqlite3 error preparing insert into entries table: ", sqlite3_errmsg(db)));
	}
	for (auto& it : entries) {
		sqlite3_reset(s);
		if (sqlite3_bind_int64(s, 1, it.first.type) != SQLITE_OK) {
			throw std::runtime_error(concat("sqlite3 error trying to bind int64 for type: ", sqlite3_errmsg(db)));
		}
		if (sqlite3_bind_int64(s, 2, it.first.id) != SQLITE_OK) {
			throw std::runtime_error(concat("sqlite3 error trying to bind int64 for id: ", sqlite3_errmsg(db)));
		}
		if (sqlite3_bind_int64(s, 3, it.second.grammar) != SQLITE_OK) {
			throw std::runtime_error(concat("sqlite3 error trying to bind int64 for grammar: ", sqlite3_errmsg(db)));
		}
		if (sqlite3_bind_int64(s, 4, it.second.b) != SQLITE_OK) {
			throw std::runtime_error(concat("sqlite3 error trying to bind int64 for b: ", sqlite3_errmsg(db)));
		}
		if (sqlite3_bind_int64(s, 5, it.second.e) != SQLITE_OK) {
			throw std::runtime_error(concat("sqlite3 error trying to bind int64 for e: ", sqlite3_errmsg(db)));
		}
		if (sqlite3_bind_int64(s, 6, it.second.num_match) != SQLITE_OK) {
			throw std::runtime_error(concat("sqlite3 error trying to bind int64 for num_match: ", sqlite3_errmsg(db)));
		}
		if (sqlite3_bind_int64(s, 7, it.second.num_fail) != SQLITE_OK) {
			throw std::runtime_error(concat("sqlite3 error trying to bind int64 for num_fail: ", sqlite3_errmsg(db)));
		}
		if (sqlite3_bind_int64(s, 8, it.second.example_window) != SQLITE_OK) {
			throw std::runtime_error(concat("sqlite3 error trying to bind int64 for example_window: ", sqlite3_errmsg(db)));
		}
		if (sqlite3_step(s) != SQLITE_DONE) {
			throw std::runtime_error(concat("sqlite3 error inserting into entries table: ", sqlite3_errmsg(db)));
		}
	}
	sqlite3_finalize(s);

	// Rule->Context hits
	if (sqlite3_prepare_v2(db, "INSERT INTO rule_contexts (rule, context, num_match) VALUES (:rule, :context, :num_match)", -1, &s, nullptr) != SQLITE_OK) {
		throw std::runtime_error(concat("sqlite3 error preparing insert into rule_contexts table: ", sqlite3_errmsg(db)));
	}
	for (auto& it : rule_contexts) {
		sqlite3_reset(s);
		if (sqlite3_bind_int64(s, 1, it.first.first) != SQLITE_OK) {
			throw std::runtime_error(concat("sqlite3 error trying to bind int64 for rule: ", sqlite3_errmsg(db)));
		}
		if (sqlite3_bind_int64(s, 2, it.first.second) != SQLITE_OK) {
			throw std::runtime_error(concat("sqlite3 error trying to bind int64 for context: ", sqlite3_errmsg(db)));
		}
		if (sqlite3_bind_int64(s, 3, it.second) != SQLITE_OK) {
			throw std::runtime_error(concat("sqlite3 error trying to bind int64 for num_match: ", sqlite3_errmsg(db)));
		}
		if (sqlite3_step(s) != SQLITE_DONE) {
			throw std::runtime_error(concat("sqlite3 error inserting into rule_contexts table: ", sqlite3_errmsg(db)));
		}
	}
	sqlite3_finalize(s);

	// Erase contexts that only exist as linked part of a larger context
	// Such contexts are currently irrelevant and just get in the way
	for (int i = 0; i < 10; ++i) {
		if (sqlite3_exec(db, "DELETE FROM entries WHERE type = 1 AND id IN (SELECT id FROM entries as et INNER JOIN (SELECT max(b) as b, e FROM entries WHERE type = 1 GROUP BY e HAVING count(b) > 1) as jt ON (et.b = jt.b AND et.e = jt.e))") != SQLITE_OK) {
			throw std::runtime_error(concat("sqlite3 error while deleting overlapping contexts: ", sqlite3_errmsg(db)));
		}
	}

	if (sqlite3_exec(db, "COMMIT") != SQLITE_OK) {
		throw std::runtime_error(concat("sqlite3 error while committing: ", sqlite3_errmsg(db)));
	}
}

void Profiler::read(const char* fname) {
	if (sqlite3_initialize() != SQLITE_OK) {
		throw std::runtime_error("sqlite3_initialize() errored");
	}

	sqlite3* db = nullptr;
	sqlite3_stmt* s = nullptr;
	int r = 0;
	std::string tmp;
	if (sqlite3_open_v2(fname, &db, SQLITE_OPEN_READONLY, nullptr) != SQLITE_OK) {
		throw std::runtime_error(concat("sqlite3_open_v2() error: ", sqlite3_errmsg(db)));
	}

	// Strings
	if (sqlite3_prepare_v2(db, "SELECT * FROM strings", -1, &s, nullptr) != SQLITE_OK) {
		throw std::runtime_error(concat("sqlite3 error preparing select from strings table: ", sqlite3_errmsg(db)));
	}
	while ((r = sqlite3_step(s)) == SQLITE_ROW) {
		auto sz = UIZ(sqlite3_column_int64(s, 0));
		tmp.assign(reinterpret_cast<const char*>(sqlite3_column_text(s, 1)));
		strings[std::move(tmp)] = sz;
	}
	sqlite3_finalize(s);

	// Grammars
	if (sqlite3_prepare_v2(db, "SELECT * FROM grammars", -1, &s, nullptr) != SQLITE_OK) {
		throw std::runtime_error(concat("sqlite3 error preparing select from grammars table: ", sqlite3_errmsg(db)));
	}
	while ((r = sqlite3_step(s)) == SQLITE_ROW) {
		auto f = UIZ(sqlite3_column_int64(s, 0));
		auto g = UIZ(sqlite3_column_int64(s, 1));
		grammars[f] = g;
	}
	sqlite3_finalize(s);

	// Entries
	if (sqlite3_prepare_v2(db, "SELECT * FROM entries", -1, &s, nullptr) != SQLITE_OK) {
		throw std::runtime_error(concat("sqlite3 error preparing select from entries table: ", sqlite3_errmsg(db)));
	}
	while ((r = sqlite3_step(s)) == SQLITE_ROW) {
		auto type = UI8(sqlite3_column_int64(s, 0));
		auto id = UI32(sqlite3_column_int64(s, 1));
		Profiler::Key k{type, id};
		auto& e = entries[k];
		e.type = type;
		e.grammar = UI32(sqlite3_column_int64(s, 2));
		e.b = UIZ(sqlite3_column_int64(s, 3));
		e.e = UIZ(sqlite3_column_int64(s, 4));
		e.num_match = UIZ(sqlite3_column_int64(s, 5));
		e.num_fail = UIZ(sqlite3_column_int64(s, 6));
		e.example_window = UIZ(sqlite3_column_int64(s, 7));
	}
	sqlite3_finalize(s);

	// Rule->Context hits
	if (sqlite3_prepare_v2(db, "SELECT * FROM rule_contexts", -1, &s, nullptr) != SQLITE_OK) {
		throw std::runtime_error(concat("sqlite3 error preparing select from rule_contexts table: ", sqlite3_errmsg(db)));
	}
	while ((r = sqlite3_step(s)) == SQLITE_ROW) {
		auto r = UI32(sqlite3_column_int64(s, 0));
		auto c = UI32(sqlite3_column_int64(s, 1));
		rule_contexts[std::pair(r,c)] = UIZ(sqlite3_column_int64(s, 2));
	}
	sqlite3_finalize(s);
}

}
