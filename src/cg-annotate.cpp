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
#include "sorted_vector.hpp"
#include "stdafx.hpp"
#include "filesystem.hpp"
namespace fs = ::std::filesystem;
using namespace CG3;

inline auto xml_encode(std::string_view in) {
	std::string buf;
	buf.reserve(in.size());
	for (auto c : in) {
		if (c == '&') {
			buf.append("&amp;");
		}
		else if (c == '"') {
			buf.append("&quot;");
		}
		else if (c == '\'') {
			buf.append("&apos;");
		}
		else if (c == '<') {
			buf.append("&lt;");
		}
		else if (c == '>') {
			buf.append("&gt;");
		}
		else {
			buf += c;
		}
	}
	return buf;
}

inline auto xml_encode(std::string str) {
	return xml_encode(std::string_view(str));
}

inline auto xml_encode(fs::path p) {
	return xml_encode(p.string());
}

inline void file_save(fs::path fn, std::string_view data) {
	std::ofstream file(fn.string(), std::ios::binary);
	file.exceptions(std::ios::badbit | std::ios::failbit);
	file.write(data.data(), data.size());
}

int main(int argc, char* argv[]) {
	using namespace ::std::string_literals;
	(void)argc;

	UErrorCode status = U_ZERO_ERROR;
	u_init(&status);
	if (U_FAILURE(status) && status != U_FILE_ACCESS_ERROR) {
		std::cerr << "Error: Cannot initialize ICU. Status = " << u_errorName(status) << std::endl;
		CG3Quit(1);
	}
	status = U_ZERO_ERROR;

	ucnv_setDefaultName("UTF-8");
	uloc_setDefault("en_US_POSIX", &status);

	Profiler profiler;
	profiler.read(argv[1]);

	fs::path folder(argv[2]);
	if (!fs::exists(folder) && !fs::create_directories(folder)) {
		throw std::runtime_error("Output folder did not exist and could not be created!");
	}
	fs::current_path(folder);

	fs::create_directories("rs");
	fs::create_directories("cs");

	size_t sz = 0;
	std::string html, buf;
	UnicodeString ubuf;

	// Re-map strings to enable lookup by ID
	std::map<size_t, std::string_view> strings;
	for (auto& it : profiler.strings) {
		strings[it.second] = it.first;
	}

	// Store per-grammar ASTs separately
	std::map<size_t, std::string> asts;
	std::string ast{strings[0]};
	while ((sz = ast.rfind("<Grammar ")) != std::string::npos) {
		auto e = ast.find("</Grammar>", sz);
		auto g = ast.substr(sz, (e - sz) + 11);
		ast.erase(sz, (e - sz) + 11);
		sz = g.find(" u=\"") + 4;
		e = std::stoi(g.substr(sz, g.find("\"", sz) - sz));
		asts[e] = g;
	}

	// Extract each rule and context's start offset, per grammar
	std::map<size_t, std::map<std::pair<size_t,size_t>, Profiler::Key>> offsets;
	auto todos_ast = {
		std::tuple("<Rule ", ET_RULE),
		std::tuple("<Context ", ET_CONTEXT),
	};
	for (auto& it : asts) {
		auto& ast = it.second;
		for (auto todo : todos_ast) {
			size_t last = 0;
			while ((last = ast.find(std::get<0>(todo), last)) != std::string::npos) {
				auto b = ast.find(" b=\"", last) + 4;
				b = std::stoul(ast.substr(b, ast.find("\"", b) - b));

				auto e = ast.find(" e=\"", last) + 4;
				e = std::stoul(ast.substr(e, ast.find("\"", e) - e));

				auto u = UI32(ast.find(" u=\"", last) + 4);
				u = std::stoul(ast.substr(u, ast.find("\"", u) - u));

				std::pair k{b, e};
				offsets[it.first][k] = { std::get<1>(todo), u };

				++last;
			}
		}
	}

	auto write_grammar = [&](size_t g, UnicodeString& grammar) {
		html.clear();
		auto gn = strings[g];
		html.resize(256 + gn.size());
		sz = sprintf(&html[0], R"X(<!DOCTYPE html>
<html>
<head>
	<meta charset="UTF-8">
	<title>%s annotated</title>
	<link rel="stylesheet" href="style.css">
</head>
<body>
<div id="grammar" class="p-2 pre-wrap">
)X", xml_encode(fs::path(gn).filename()).c_str());
		html.resize(sz);

		auto gz = profiler.grammars[g];
		buf.clear();
		auto& offs = offsets[gz];

		sorted_vector<size_t, std::greater<>> close;
		size_t e = 0;
		std::pair<size_t, uint32_t> rid;
		for (auto& it : offs) {
			auto eit = profiler.entries.find(it.second);
			if (eit == profiler.entries.end()) {
				continue;
			}
			auto& entry = eit->second;

			while (!close.empty() && close.back() <= it.first.first) {
				if (rid.first == it.first.first) {
					rid = {};
				}
				buf.clear();
				auto tmp = grammar.tempSubStringBetween(SI32(e), SI32(close.back()));
				tmp.toUTF8String(buf);
				html += xml_encode(buf);
				html += "</a>";
				e = close.back();
				close.pop_back();
			}

			buf.clear();
			auto tmp = grammar.tempSubStringBetween(SI32(e), SI32(it.first.first));
			tmp.toUTF8String(buf);
			html += xml_encode(buf);
			e = it.first.first;

			html += "<a href=\"";
			if (it.second.type == ET_RULE) {
				html += "rs/";
			}
			else {
				html += "cs/";
			}
			html += std::to_string(it.second.id);
			html += ".html\"";
			if (entry.type == ET_RULE || !rid.second) {
				if (entry.num_match != 0) {
					html += R"X( class="entry good"><span class="stats">M:)X";
				}
				else {
					html += R"X( class="entry bad"><span class="stats">M:)X";
				}
				html += std::to_string(entry.num_match);
				html += ", F:";
				html += std::to_string(entry.num_fail);
			}
			else {
				std::pair k{rid.second, it.second.id};
				auto rct = profiler.rule_contexts.find(k);
				if (rct != profiler.rule_contexts.end() && rct->second) {
					html += R"X( class="entry context good"><span class="stats">M:)X";
					html += std::to_string(rct->second);
				}
				else {
					html += R"X( class="entry context bad"><span class="stats">M:0)X";
				}
			}
			html += "</span>";

			if (entry.type == ET_RULE) {
				rid = std::pair(it.first.first, it.second.id);
			}
			close.push_back(it.first.second);
		}

		buf.clear();
		auto tmp = grammar.tempSubStringBetween(SI32(e));
		tmp.toUTF8String(buf);
		html += xml_encode(buf);

		html += R"X(</div>
</body>
</html>
)X";

		fs::path fn{"g"s.append(std::to_string(profiler.grammars[g])).append(".html")};
		file_save(fn, html);
	};

	// UTF-16 copies of the grammars, to enable extracting snippets from offsets
	std::map<size_t, UnicodeString> grammars;
	for (auto& it : profiler.grammars) {
		// ToDo: Pass string_view directly when ICU 65 is oldest supported
		grammars[it.second] = UnicodeString::fromUTF8({ strings[it.second].data(), SI32(strings[it.second].size()) });
		write_grammar(it.first, grammars[it.second]);
	}

	// Helper to write out usage examples
	auto write_entry = [&](uint32_t id, Profiler::Entry& e) {
		html.clear();
		auto& g = grammars[e.grammar];

		html += R"X(<!DOCTYPE html>
<html>
<head>
	<meta charset="UTF-8">
	<title>Usage example</title>
	<link rel="stylesheet" href="../style.css">
</head>
<body>
<div id="what" class="p-2 pre-wrap">
)X";

		buf.clear();
		auto snip = g.tempSubString(SI32(e.b), SI32(e.e - e.b));
		snip.toUTF8String(buf);
		html += xml_encode(buf);
		html += R"X(
</div>
<div id="context" class="p-2 pre-wrap">
)X";
		html += xml_encode(strings[e.example_window]);
		html += R"X(
</div>
</body>
</html>
)X";

		fs::path fn{"rs"};
		if (e.type == ET_CONTEXT) {
			fn = "cs";
		}
		fn /= std::to_string(id).append(".html");
		file_save(fn, html);
	};

	for (auto& it : profiler.entries) {
		if (it.second.example_window) {
			write_entry(it.first.id, it.second);
		}
	}

	html.clear();
	for (auto& it : profiler.grammars) {
		auto s_f = xml_encode(fs::path(strings[it.first]).filename());
		buf.clear();
		buf.resize(100 + s_f.size());
		sz = sprintf(&buf[0], R"X(<li><a href="g%zu.html">%s</a></li>)X", it.second, s_f.c_str());
		buf.resize(sz);
		html.append(buf);
	}
	std::swap(html, buf);

	auto gn = strings[profiler.grammars.begin()->first];
	html.resize(512 + gn.size() + buf.size());
	sz = sprintf(&html[0], R"X(<!DOCTYPE html>
<html>
<head>
	<meta charset="UTF-8">
	<title>%s &laquo; CG-3 Grammar Annotation</title>
	<link rel="stylesheet" href="style.css">
</head>
<body>
<ul>
%s
</ul>
</body>
</html>
)X", xml_encode(fs::path(gn).filename()).c_str(), buf.c_str());
	html.resize(sz);

	file_save("index.html", html);

	file_save("style.css", R"X(
html, body {
	background-color: #fff;
	font-family: sans-serif;
}

a {
	text-decoration: none;
}

.entry {
	display: inline-block;
	position: relative;
	padding-top: 3ex;
}

#what, .good {
	background-color: #cfc;
}

#context, .context {
	background-color: #ccf;
}

.pre-wrap {
	white-space: pre-wrap;
	font-family: monospace;
}

.bad {
	background-color: #fcc;
}

.stats {
	font-family: sans-serif;
	position: absolute;
	top: 0;
	left: 0;
	white-space: nowrap;
	background-color: #eee;
}

.p-2 {
	padding: 1ex;
}
)X");
}
