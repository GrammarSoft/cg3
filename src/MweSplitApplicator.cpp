/*
* Copyright (C) 2007-2023, GrammarSoft ApS
* Developed by Tino Didriksen <mail@tinodidriksen.com>
* Design by Eckhard Bick <eckhard.bick@mail.dk>, Tino Didriksen <mail@tinodidriksen.com>
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

#include "MweSplitApplicator.hpp"

namespace CG3 {

MweSplitApplicator::MweSplitApplicator(std::ostream& ux_err)
  : GrammarApplicator(ux_err)
{
	Grammar* grammar = new Grammar;
	grammar->ux_stderr = ux_stderr;
	grammar->allocateDummySet();
	grammar->delimiters = grammar->allocateSet();
	grammar->addTagToSet(grammar->allocateTag(STR_DUMMY), grammar->delimiters);
	grammar->reindex();
	setGrammar(grammar);
	owns_grammar = true;
	is_conv = true;
}

void MweSplitApplicator::runGrammarOnText(std::istream& input, std::ostream& output) {
	GrammarApplicator::runGrammarOnText(input, output);
}

const Tag* MweSplitApplicator::maybeWfTag(const Reading* r) {
	for (auto tter : r->tags_list) {
		if ((!show_end_tags && tter == endtag) || tter == begintag) {
			continue;
		}
		if (tter == r->baseform || tter == r->parent->wordform->hash) {
			continue;
		}
		const Tag* tag = grammar->single_tags[tter];
		// If we are to split, there has to be at least one wordform on a head (not-sub) reading
		if (tag->type & T_WORDFORM) {
			return tag;
		}
	}
	return nullptr;
}

std::vector<Cohort*> MweSplitApplicator::splitMwe(Cohort* cohort) {
	constexpr UChar rtrimblank[] = { ' ', '\n', '\r', '\t', 0 };
	constexpr UChar textprefix[] = { ':', 0 };
	std::vector<Cohort*> cos;
	size_t n_wftags = 0;
	size_t n_goodreadings = 0;
	for (auto rter1 : cohort->readings) {
		if (maybeWfTag(rter1) != nullptr) {
			++n_wftags;
		}
		++n_goodreadings;
	}

	if (n_wftags < n_goodreadings) {
		if (n_wftags > 0) {
			u_fprintf(ux_stderr, "WARNING: Line %u: Some but not all main-readings of %S had wordform-tags (not completely mwe-disambiguated?), not splitting.\n", cohort->line_number, cohort->wordform->tag.data());
			// We also don't split if wordform-tags were only on sub-readings, but should we warn on such faulty input?
		}
		cos.push_back(cohort);
		return cos;
	}
	UString pretext;
	for (auto r : cohort->readings) {
		size_t pos = std::numeric_limits<size_t>::max();
		Reading* prev = nullptr; // prev == NULL || prev->next == rNew (or a ->next of rNew)
		for (auto sub = r; sub; sub = sub->next) {
			const Tag* wfTag = maybeWfTag(sub);
			if (wfTag == nullptr) {
				prev = prev->next;
			}
			else {
				++pos;
				Cohort* c;
				while (cos.size() < pos + 1) {
					c = alloc_cohort(cohort->parent);
					c->global_number = gWindow->cohort_counter++;
					cohort->parent->appendCohort(c);
					if(pretext.size() > 0) {
						c->text = pretext;
						pretext.clear();
					}
					cos.push_back(c);
				}
				c = cos[pos];

				const size_t wfBeg = 2; // index after the initial '"<'
				const size_t spBeg0 = wfTag->tag.find_first_not_of(rtrimblank, wfBeg); // index skipping initial space
				const size_t spBeg = sub->next ? spBeg0 : wfBeg; // can't put pretext on first word / deepest reading
				const size_t wfEnd = wfTag->tag.size() - 3; // index before the final '>"'
				const size_t spEnd = 1 + wfTag->tag.find_last_not_of(rtrimblank, wfEnd); // index before post-space
				const UString& wf =
					  wfTag->tag.substr(0,     wfBeg)
					+ wfTag->tag.substr(spBeg, spEnd - spBeg)
					+ wfTag->tag.substr(wfEnd + 1);
				if (c->wordform != 0 && wf != c->wordform->tag) {
					u_fprintf(ux_stderr, "WARNING: Line %u: Ambiguous wordform-tags for same cohort, '%S' vs '%S', not splitting.\n", numLines, wf.data(), c->wordform->tag.data());
					cos.clear();
					cos.push_back(cohort);
					return cos;
				}
				c->wordform = addTag(wf);
				if (spBeg > wfBeg) {
					pretext = textprefix + wfTag->tag.substr(wfBeg, spBeg - wfBeg);
				}
				if (spEnd < wfEnd + 1) {
					c->text = textprefix + wfTag->tag.substr(spEnd, wfEnd + 1 - spEnd);
				}

				Reading* rNew = alloc_reading(*sub);
				for (size_t i = 0; i < rNew->tags_list.size(); ++i) {
					auto& tter = rNew->tags_list[i];
					if (tter == wfTag->hash || tter == rNew->parent->wordform->hash) {
						rNew->tags_list.erase(rNew->tags_list.begin() + i);
						rNew->tags.erase(tter);
					}
				}
				cos[pos]->appendReading(rNew);
				rNew->parent = cos[pos];

				if (prev != nullptr) {
					free_reading(prev->next);
				}
				prev = rNew;
			}
		}
	}
	if (cos.size() == 0) {
		u_fprintf(ux_stderr, "WARNING: Line %u: Tried splitting %S, but got no new cohorts; shouldn't happen.", numLines, cohort->wordform->tag.data());
		cos.push_back(cohort);
	}
	// The last word forms are the top readings:
	cos[0]->text = cohort->text;
	std::reverse(cos.begin(), cos.end());
	return cos;
}

void MweSplitApplicator::printSingleWindow(SingleWindow* window, std::ostream& output, bool profiling) {
	for (auto var : window->variables_output) {
		Tag* key = grammar->single_tags[var];
		auto iter = window->variables_set.find(var);
		if (iter != window->variables_set.end()) {
			if (iter->second != grammar->tag_any) {
				Tag* value = grammar->single_tags[iter->second];
				u_fprintf(output, "%S%S=%S>\n", STR_CMD_SETVAR.data(), key->tag.data(), value->tag.data());
			}
			else {
				u_fprintf(output, "%S%S>\n", STR_CMD_SETVAR.data(), key->tag.data());
			}
		}
		else {
			u_fprintf(output, "%S%S>\n", STR_CMD_REMVAR.data(), key->tag.data());
		}
	}

	if (!window->text.empty()) {
		u_fprintf(output, "%S", window->text.data());
		if (!ISNL(window->text.back())) {
			u_fputc('\n', output);
		}
	}

	auto cs = UI32(window->cohorts.size());
	for (uint32_t c = 0; c < cs; c++) {
		Cohort* cohort = window->cohorts[c];
		std::vector<Cohort*> cs = splitMwe(cohort);
		for (auto iter : cs) {
			printCohort(iter, output, profiling);
		}
	}

	if (!window->text_post.empty()) {
		u_fprintf(output, "%S", window->text_post.data());
		if (!ISNL(window->text_post.back())) {
			u_fputc('\n', output);
		}
	}

	u_fputc('\n', output);
	if (window->flush_after) {
		u_fprintf(output, "%S\n", STR_CMD_FLUSH.data());
	}
	u_fflush(output);
}
}
