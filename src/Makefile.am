bin_PROGRAMS = vislcg3
vislcg3_SOURCES = hash.cpp GrammarParser.cpp Strings.cpp \
	GrammarParser_parseList.cpp GrammarParser_parseSelectRemoveIffDelimit.cpp \
	GrammarParser_parseSet.cpp icu_cmemory.cpp icu_cstring.cpp icu_uoptions.cpp main.cpp uextras.cpp \
	GrammarParser_parseDelimiters.cpp GrammarParser_parsePreferredTargets.cpp

# set the include path found by configure
INCLUDES= $(all_includes)

# the library search path.
vislcg3_LDFLAGS = $(all_libraries) 
vislcg3_LDADD = -licuio -licuuc
