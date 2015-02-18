// -*- encoding: UTF-8; tab-width: 4; -*-
grammar cg3;

// Caveat: All keywords are really case-insensitive. They're upper-case here only to make the grammar readable.

/*
Lines that should parse, but don't:

# Set names that are also keywords. It is valid to name sets the same as almost any keyword (notably not TARGET or IF)
LIST ALL = waffle ;
SELECT ALL ;

# Set names with # in them...
LIST name# = waffle ;
SELECT name# ;

# Lower or mixed case keywords...
list name = waffle ;
Select name ;
/*/

/*
Lines that should not parse, but do:

# Missing set name. See contexttest
SELECT x (-1 LINK 1 x) ;

# See contexttest. I am considering disallowing ',' in set names to make it valid. Nobody uses ',' in set names anyway.
TEMPLATE name = [x,y,z] ;
/*/

cg 	:	stat+ 'END'? ;

stat
	:	delimiters
	|	soft_delimiters
	|	preferred_targets
	|	static_sets
	|	parentheses
	|	mapping_prefix
	|	subreadings
	|	'SETS'
	|	list
	|	set
	|	template
	|	include

	|	before_sections
	|	section
	|	after_sections
	|	null_section

	|	rule
	|	rule_substitute_etc
	|	rule_map_etc
	|	rule_parentchild
	|	rule_move
	|	rule_switch
	|	rule_relation
	|	rule_relations
	|	rule_addcohort
	|	rule_external
	;

delimiters
	:	'DELIMITERS' '=' taglist ';'
	;

soft_delimiters
	:	'SOFT-DELIMITERS' '=' taglist ';'
	;

preferred_targets
	:	'PREFERRED-TARGETS' '=' taglist ';'
	;

static_sets
	:	'STATIC-SETS' '=' setname+ ';'
	;

mapping_prefix
	:	'MAPPING-PREFIX' '=' prefix ';'
	;

subreadings
	:	'SUBREADINGS' '=' ('RTL'|'LTR') ';'
	;

parentheses
// While parsed as regular composite tags, each compotag must have exactly 2 tags
	:	'PARENTHESES' '=' compotag+ ';'
	;

list
	:	'LIST' setname '=' taglist ';'
	;

set
	:	'SET' setname '=' inlineset ';'
	;

include
	:	'INCLUDE' rawpath ';'
	;

before_sections
	:	'BEFORE-SECTIONS'
	|	'MAPPINGS'
	|	'CORRECTIONS'
	;

section
	:	'SECTION'
	|	'CONSTRAINTS'
	;

after_sections
	:	'AFTER-SECTIONS'
	;

null_section
	:	'NULL-SECTION'
	;

rule
	:	qtag? ruletype ruleflag* 'TARGET'? inlineset 'IF'? ('(' contexttest ')')* ';'
	;

rule_substitute_etc
	:	qtag? ruletype_substitute_etc ruleflag* inlineset inlineset 'TARGET'? inlineset 'IF'? ('(' contexttest ')')* ';'
	;

rule_map_etc
	:	qtag? ruletype_map_etc ruleflag* inlineset 'TARGET'? inlineset 'IF'? ('(' contexttest ')')* ';'
	;

rule_parentchild
	:	qtag? ruletype_parentchild ruleflag* 'TARGET'? inlineset 'IF'? ('(' contexttest ')')* ('TO'|'FROM') ('(' contexttest ')')+ ';'
	;

rule_move
	:	qtag? ruletype_move ruleflag* ('WITHCHILD' inlineset|'NOCHILD')? 'TARGET'? inlineset 'IF'? ('(' contexttest ')')* ('BEFORE'|'AFTER') ('WITHCHILD' inlineset|'NOCHILD')? ('(' contexttest ')')+ ';'
	;

rule_switch
	:	qtag? ruletype_switch ruleflag* 'TARGET'? inlineset 'IF'? ('(' contexttest ')')* 'WITH' ('(' contexttest ')')+ ';'
	;

rule_relation
	:	qtag? ruletype_relation ruleflag* inlineset 'TARGET'? inlineset 'IF'? ('(' contexttest ')')* ('TO'|'FROM') ('(' contexttest ')')+ ';'
	;

rule_relations
	:	qtag? ruletype_relations ruleflag* inlineset inlineset 'TARGET'? inlineset 'IF'? ('(' contexttest ')')* ('TO'|'FROM') ('(' contexttest ')')+ ';'
	;

rule_addcohort
	:	qtag? ruletype_addcohort ruleflag* inlineset ('BEFORE'|'AFTER') 'TARGET'? inlineset 'IF'? ('(' contexttest ')')* ';'
	;

rule_external
	:	qtag? ruletype_external ('ONCE'|'ALWAYS') filepath ruleflag* 'TARGET'? inlineset 'IF'? ('(' contexttest ')')* ';'
	;

template
	:	'TEMPLATE' ntag '=' contexttest ';'
	;

contexttest
// Fixme: Not perfect. Parses, but goes in the wrong category...
// Fixme: It really should be the contextpos that's optional, and that it is only optional if inlineset starts with T:, but dunno if that should be expressed in BNF
// Fixme: Until above is fixed, this will allow invalid constructs through, such as (-1 LINK ...)
	:	('ALL'|'NONE'|'NOT'|'NEGATE')? contextpos inlineset? (('BARRIER'|'CBARRIER') inlineset)? ('LINK' contexttest)?
// Fixme: ',' must be followed by any amount of whitespace. [x, y, z] and [x,    y  ,  z] is valid; [x,y,z] is not valid (must currently be parsed as a single set name "x,y,z" since set names may contain ','.
	|	'[' inlineset (',' inlineset)* ']' ('LINK' contexttest)?
	|	'(' contexttest ')' ('OR' '(' contexttest ')')*
	;

inlineset
	:	inlineset_single (set_op inlineset_single)*
	;

inlineset_single
	:	'(' taglist ')'
	|	setname
	;

set_op
	:	'OR'|'|'|'+'|'-'|'^'|'\u2206'|'\u2229'
	;

taglist
	:	(tag | compotag)+
	;

compotag
	:	'(' tag+ ')'
	;

rawpath
// Fixme: This actually just needs to eat everything except ';' and '#', including spaces, as a single token.
	:	ntag+
	;

filepath
	:	ntag|qtag
	;

tag
	:	ntag|qtag
	;

contextpos
// Fixme: Actually parsing this is rather more complex, but for tokenization the rules for NTAG are ok.
// Fixme: This really should also allow ','|'['|']'
	:	NTAG
	;

ntag
	:	NTAG
	;

qtag
	:	QTAG
	;

prefix
// Fixme: Really should be a single non-space non-comment character...not a whole tag.
	:	NTAG
	;

setname
// Fixme: See SETNAME
//	:	SETNAME
	:	NTAG
	;

ruletype
	:	RULETYPE
	;

ruletype_substitute_etc
	:	RULETYPE_SUBSTITUTE_ETC
	;

ruletype_map_etc
	:	RULETYPE_MAP_ETC
	;

ruletype_parentchild
	:	RULETYPE_PARENTCHILD
	;

ruletype_move
	:	RULETYPE_MOVE
	;

ruletype_switch
	:	RULETYPE_SWITCH
	;

ruletype_relation
	:	RULETYPE_RELATION
	;

ruletype_relations
	:	RULETYPE_RELATIONS
	;

ruletype_addcohort
	:	RULETYPE_ADDCOHORT
	;

ruletype_external
	:	RULETYPE_EXTERNAL
	;

ruleflag
	:	RULEFLAG
	;

RULETYPE
	:	(
// Left all rule types in the list but commented out to keep track of them all
//		'ADD'
//	|	'MAP'
//	|	'REPLACE'
		'SELECT'
	|	'REMOVE'
	|	'IFF'
//	|	'APPEND'
//	|	'SUBSTITUTE'
//	|	'EXECUTE'
//	|	'JUMP'
//	|	'REMVARIABLE'
//	|	'SETVARIABLE'
	|	'DELIMIT'
	|	'MATCH'
//	|	'SETPARENT'
//	|	'SETCHILD'
//	|	'ADDRELATION'
//	|	'SETRELATION'
//	|	'REMRELATION'
//	|	'ADDRELATIONS'
//	|	'SETRELATIONS'
//	|	'REMRELATIONS'
//	|	'MOVE'
//	|	'SWITCH'
	|	'REMCOHORT'
	|	'UNMAP'
//	|	'COPY'
//	|	'ADDCOHORT'
//	|	'EXTERNAL'
	) (':' NTAG)?
	;

RULETYPE_SUBSTITUTE_ETC
	:	(
		'SUBSTITUTE'
	|	'SETVARIABLE'
	|	'EXECUTE'
	) (':' NTAG)?
	;

RULETYPE_PARENTCHILD
	:	(
		'SETPARENT'
	|	'SETCHILD'
	) (':' NTAG)?
	;

RULETYPE_RELATION
	:	(
		'ADDRELATION'
	|	'SETRELATION'
	|	'REMRELATION'
	) (':' NTAG)?
	;

RULETYPE_RELATIONS
	:	(
		'ADDRELATIONS'
	|	'SETRELATIONS'
	|	'REMRELATIONS'
	) (':' NTAG)?
	;

RULETYPE_MAP_ETC
	:	(
		'ADD'
	|	'MAP'
	|	'REPLACE'
	|	'APPEND'
	|	'COPY'
	|	'REMVARIABLE'
	|	'JUMP'
	) (':' NTAG)?
	;

RULETYPE_ADDCOHORT
	:	'ADDCOHORT' (':' NTAG)?
	;

RULETYPE_MOVE
	:	'MOVE' (':' NTAG)?
	;

RULETYPE_SWITCH
	:	'SWITCH' (':' NTAG)?
	;

RULETYPE_EXTERNAL
	:	'EXTERNAL' (':' NTAG)?
	;

RULEFLAG
	:	(
		'NEAREST'
	|	'ALLOWLOOP'
	|	'DELAYED'
	|	'IMMEDIATE'
	|	'LOOKDELETED'
	|	'LOOKDELAYED'
	|	'UNSAFE'
	|	'SAFE'
	|	'REMEMBERX'
	|	'RESETX'
	|	'KEEPORDER'
	|	'VARYORDER'
	|	'ENCL_INNER'
	|	'ENCL_OUTER'
	|	'ENCL_FINAL'
	|	'ENCL_ANY'
	|	'ALLOWCROSS'
//	|	'WITHCHILD'
//	|	'NOCHILD'
	|	'ITERATE'
	|	'NOITERATE'
	|	'UNMAPLAST'
	|	'REVERSE'
	|	'SUB'
	) (':' NTAG)?
	;

/*
// Fixme: It breaks if I enable this here and above in setname, but I don't know why...
SETNAME
// Unfortunately, almost everything is valid in set names, even # which is otherwise a comment.
// Fixme: Set names may not start with '[' and may not end with ','|']' but may contain those otherwise, but dunno how to express that...
	:	~('"'|'('|')'|';'|','|'['|']'|SPACE)+
	;
/*/

/*
// Fixme: See rawpath
RAWPATH
	:	~('#'|';')+
	;
/*/

NTAG
// If a tag doesn't start with " then it's fairly simple...
// Fixme: Tags may contain ','|'['|']' but had to leave them out due to also parsing set names with this for now.
	:	~('"'|'#'|'('|')'|';'|','|'['|']'|SPACE)+
	;

QTAG
// If a tag starts with " then the scanner skips to the next unescaped ", and from there skips to the next space or semicolon.
// The entire such capture is one tag, and it eliminates the need for most escaping in strings.
	:	('^'|'!')? '"' ('\\' ~('\n'|'\r') | ~('\\'|'"'))+ '"' ~('('|')'|';'|SPACE)*
	;

COMMENT
	:   '#' ~('\n'|'\r')* '\r'? '\n' {$channel=HIDDEN;}
	;

WS
	:   SPACE {$channel=HIDDEN;}
	;

fragment
SPACE
	:	(' '|'\t'|'\r'|'\n')
	;

fragment
SET_OP
	:	'OR'|'|'|'+'|'-'|'^'|'\u2206'|'\u2229'
	;
