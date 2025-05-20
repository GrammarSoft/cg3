/*
* Copyright (C) 2024, GrammarSoft ApS
* Developed by Tino Didriksen <mail@tinodidriksen.com>
* Based on contributions from GitHub Copilot
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

#ifndef JSONLAPPLICATOR_HPP
#define JSONLAPPLICATOR_HPP

#include "GrammarApplicator.hpp"
#include "rapidjson/document.h"

namespace CG3 {

class JsonlApplicator : public virtual GrammarApplicator {
public:
	JsonlApplicator(std::ostream& ux_err);
	~JsonlApplicator() override;

	void runGrammarOnText(std::istream& input, std::ostream& output) override;

protected:
	void printCohort(Cohort* cohort, std::ostream& output, bool profiling = false) override;
	void printSingleWindow(SingleWindow* window, std::ostream& output, bool profiling = false) override;
	void printStreamCommand(UStringView cmd, std::ostream& output) override;
	void printPlainTextLine(UStringView line, std::ostream& output) override;

private:
	void parseJsonCohort(const rapidjson::Value& obj, SingleWindow* cSWindow, Cohort*& cCohort);
	Reading* parseJsonReading(const rapidjson::Value& reading_obj, Cohort* parentCohort);
	void buildJsonReading(const Reading* reading, rapidjson::Value& reading_json, rapidjson::Document::AllocatorType& allocator);
	void buildJsonTags(const Reading* reading, rapidjson::Value& tags_json, rapidjson::Document::AllocatorType& allocator);
};

} // namespace CG3

#endif // JSONLAPPLICATOR_HPP
