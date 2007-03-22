bin_PROGRAMS = vislcg3
vislcg3_SOURCES = hash.cpp GrammarParser.cpp Strings.cpp \
		GrammarParser_parseList.cpp GrammarParser_parseSet.cpp icu_cmemory.cpp icu_cstring.cpp \
		icu_uoptions.cpp main.cpp uextras.cpp GrammarParser_parseDelimiters.cpp \
		GrammarParser_parsePreferredTargets.cpp CompositeTag.cpp Grammar.cpp GrammarParser_parseTarget.cpp Rule.cpp \
		Section.cpp Set.cpp Tag.cpp ContextualTest.cpp \
		GrammarParser_parseContextualTest.cpp Anchor.cpp GrammarParser_parseAddMapReplaceAppend.cpp \
		GrammarParser_parseSelectRemoveIffDelimitMatch.cpp GrammarParser_parseAnchor.cpp GrammarParser_parseSection.cpp Cohort.cpp \
	GPRE2C.cpp GrammarApplicator.cpp GrammarApplicator_matchSet.cpp \
	GrammarApplicator_reflow.cpp GrammarApplicator_runGrammar.cpp GrammarApplicator_runRules.cpp \
	GrammarParser_parseRemSetVariable.cpp GrammarParser_parseSoftDelimiters.cpp GrammarParser_parseSubstitute.cpp \
	GrammarWriter.cpp Index.cpp PACC_Timer.cpp Reading.cpp SingleWindow.cpp Window.cpp

# set the include path found by configure
INCLUDES= $(all_includes)

# the library search path.
vislcg3_LDFLAGS = $(all_libraries) 
vislcg3_LDADD = -licuio -licuuc
