bin_PROGRAMS = vislcg3
vislcg3_SOURCES = hash.cpp GrammarParser.cpp Strings.cpp \
	GrammarParser_parseList.cpp GrammarParser_parseSet.cpp icu_cmemory.cpp icu_cstring.cpp \
	icu_uoptions.cpp main.cpp uextras.cpp GrammarParser_parseDelimiters.cpp \
	GrammarParser_parsePreferredTargets.cpp CompositeTag.cpp Grammar.cpp GrammarParser_parseTarget.cpp Rule.cpp \
	Section.cpp Set.cpp Tag.cpp ContextualTest.cpp \
	GrammarParser_parseContextualTest.cpp Anchor.cpp GrammarParser_parseAddMapReplaceAppend.cpp \
	GrammarParser_parseSelectRemoveIffDelimitMatch.cpp

# set the include path found by configure
INCLUDES= $(all_includes)

# the library search path.
vislcg3_LDFLAGS = $(all_libraries) 
vislcg3_LDADD = -licuio -licuuc
