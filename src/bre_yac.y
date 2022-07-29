%{
/*
 */
#ifndef lint
static char rcsid[] = "@(#)$Id$";
#endif

/***********************************************************************
 * BRE GRAMMAR
 */
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <string.h>
#include <assert.h>

#include "bre_api.h"
#include "bre_prv.h"

#define YYSTYPE		union yystack

%}

/* types of the non-terminals */
%type <expr>	args arg_list list_items list variable opt_substr
%type <expr>	factor group_expr for_expr expr index_list index_expr
%type <expr>	statement block block_stmt group_expr_sa
%type <expr>	error_exprs error_clause for_idlist where_clause
%type <op>	grpfunc
%type <type>	type_decl
%type <idlist>	id_list parameters
%type <str>	using_clause rule_id

/* Definition of returned tokens from the lexical analyzer */

/*
 * Tokens representing operators in expressions
 */
%token OR		/* or */
%token AND		/* and */
%token LE		/* <= */
%token GE		/* >= */
%token NE		/* <> */
%token IN		/* in */
%token BETWEEN		/* between */
%token ASSIGN_OP	/* := */
%token NOT		/* not */
%token OLD		/* old */
%token LIKE		/* like */

/*
 * Tokens represeting language elements
 */
%token EXTERNAL
%token CONSTANT
%token FUNCTION
%token LOOKUP
%token RULE
%token GROUP
%token IS
%token USING
%token FAIL
%token WARN
%token WITH

/*
 * Tokens representing statements/iterators
 */
%token COMPUTE
%token WHERE
%token FOR
%token ALL
%token SOME
%token IF
%token THEN
%token ELSE
%token BEGIN
%token END

/*
 * Tokens representing group functions
 */
%token COUNT
%token MAX
%token MIN
%token SUM

%token ROWNUM

/*
 * Tokens representing data types
 */
%token KW_INTEGER
%token KW_FLOAT
%token KW_BOOLEAN
%token KW_STRING

/*
 * Tokens representing literals
 */
%token	<l>	INTEGER
%token	<d>	FLOAT
%token	<str>	STRING
%token	<l>	BOOLEAN		/* eg. true or false */
%token		KW_NULL		/* null */

/*
 * Tokens representing symbols or identifiers
 */
%token	<str>	IDENTIFIER

/*
 * Operators precedence
 */
%left ASSIGN_OP
%left IF FOR COMPUTE
%left ELSE WHERE
%left OR
%left AND
%nonassoc NOT
%left IN '=' NE '<' LE '>' GE BETWEEN LIKE
%left '+' '-'
%left '*' '/' '%'
%left '.'
%nonassoc OLD

%start program

%%
program		: decl_list
		;

decl_list	: declaration
		| decl_list declaration
		;

declaration	: constant_decl
		| function_decl
		| group_decl
		| external_func
		| external_lookup
		| rule_decl
		;

external_func	: EXTERNAL FUNCTION IDENTIFIER parameters
							{ declare_extfunc($3, $4); }
		;

external_lookup	: EXTERNAL LOOKUP IDENTIFIER '[' id_list ']' '.' '{' id_list '}'
							{ declare_lookup($3, $5, $9); }
		;

constant_decl	: CONSTANT IDENTIFIER IS expr		{ declare_constant($2, $4); }
		;

function_decl	: FUNCTION IDENTIFIER parameters IS 	{ declare_func($2, $3); }
				stmt_list		{ terminate_func(); }
		;

group_decl	: GROUP IDENTIFIER using_clause IS	{ declare_group($2, $3); }
						expr	{ terminate_group($6); }
		;

rule_decl	: RULE rule_id using_clause IS		{ declare_rule($2, $3); }
				stmt_list error_clause	{ terminate_rule($7); }
		;

parameters	: '(' id_list ')'			{ $$ = $2; }
		| /* empty */				{ $$ = NULL; }
		;

id_list		: IDENTIFIER				{ $$ = idlist(NULL, $1); }
		| id_list ',' IDENTIFIER		{ $$ = idlist($1, $3); }
		;

rule_id		: IDENTIFIER				{ $$ = $1; }
		| INTEGER				{ $$ = make_ruleid($1); }
		;

using_clause	: USING IDENTIFIER			{ $$ = $2; }
		| /* empty */				{ $$ = NULL; }
		;

error_clause	: WARN WITH ':' error_exprs		{ $$ = expr_error(E_WARN, $4); }
		| FAIL WITH ':' error_exprs		{ $$ = expr_error(E_FAIL, $4); }
		| /* empty */				{ $$ = NULL; }
		;

error_exprs	: expr					{ $$ = expr_args(NULL, $1); }
		| error_exprs ',' expr			{ $$ = expr_args($1, $3); }
		;

stmt_list	: statement				{ statement($1); }
		| stmt_list ';' statement		{ statement($3); }
		;

statement	: var_decl				{ $$ = NULL; }
		| expr					{ $$ = $1; }
		| /* empty */				{ $$ = NULL; }
		;

var_decl	: id_list ':' type_decl			{ declare_type($1, $3); }
		;

type_decl	: KW_INTEGER				{ $$ = basic_type(KW_INTEGER); }
		| '{' KW_INTEGER '}'			{ $$ = list_type(KW_INTEGER); }
		| KW_STRING				{ $$ = basic_type(KW_STRING); }
		| '{' KW_STRING '}'			{ $$ = list_type(KW_STRING); }
		| KW_FLOAT				{ $$ = basic_type(KW_FLOAT); }
		| '{' KW_FLOAT '}'			{ $$ = list_type(KW_FLOAT); }
		| KW_BOOLEAN				{ $$ = basic_type(KW_BOOLEAN); }
		| '{' KW_BOOLEAN '}'			{ $$ = list_type(KW_BOOLEAN); }
		| IDENTIFIER				{ $$ = complex_type($1); }
		| '{' IDENTIFIER '}'			{ $$ = complex_list_type($2); }
		;

expr		: FOR ALL for_idlist IN for_expr	{ $$ = expr_iterator(E_FORALL, $3, $5); }
				 expr			%prec FOR
							{ $$ = expr_for(E_FORALL, $<expr>6, $7); }

		| FOR SOME for_idlist IN for_expr	{ $$ = expr_iterator(E_FORSOME, $3, $5); }
				expr			%prec FOR
							{ $$ = expr_for(E_FORSOME, $<expr>6, $7); }

		| FOR for_idlist IN for_expr
							{ $$ = expr_iterator(E_COMPUTE, $2, $4); }
			COMPUTE group_expr where_clause		%prec COMPUTE
							{ $$ = expr_compute($7, $<expr>5, $8); }

		| IF expr THEN expr ELSE expr		%prec IF
							{ $$ = expr_ifelse($2, $4, $6); }

		| IF expr THEN expr	%prec IF	{ $$ = expr_if($2, $4); }
	
		| BEGIN block END	%prec FOR	{ $$ = expr_block($2); }

		| expr OR expr				{ $$ = expr_logical(E_OR, $1, $3); }
		| expr AND expr				{ $$ = expr_logical(E_AND, $1, $3); }
		| expr IN expr				{ $$ = expr_in(E_IN, $1, $3); }
		| expr BETWEEN expr			{ $$ = expr_between(E_BETWEEN, $1, $3); }
		| expr NOT IN expr	%prec IN	{ $$ = expr_in(E_NIN, $1, $4); }
		| expr NOT BETWEEN expr	%prec BETWEEN	{ $$ = expr_between(E_NBETWEEN, $1, $4); }
		| expr LIKE expr			{ $$ = expr_like(E_LIKE, $1, $3); }
		| expr NOT LIKE expr %prec LIKE		{ $$ = expr_like(E_NLIKE, $1, $4); }
		| expr '=' expr				{ $$ = expr_equality(E_EQ, $1, $3); }
		| expr NE expr				{ $$ = expr_equality(E_NE, $1, $3); }
		| expr '<' expr				{ $$ = expr_relational(E_LT, $1, $3); }
		| expr LE expr				{ $$ = expr_relational(E_LE, $1, $3); }
		| expr '>' expr				{ $$ = expr_relational(E_GT, $1, $3); }
		| expr GE expr				{ $$ = expr_relational(E_GE, $1, $3); }
		| expr '+' expr				{ $$ = expr_add($1, $3); }
		| expr '-' expr				{ $$ = expr_arithmetic(E_SUB, $1, $3); }
		| expr '*' expr				{ $$ = expr_arithmetic(E_MUL, $1, $3); }
		| expr '/' expr				{ $$ = expr_arithmetic(E_DIV, $1, $3); }
		| expr '%' expr				{ $$ = expr_arithmetic(E_MOD, $1, $3); }
		| NOT expr				{ $$ = expr_logical(E_NOT, NULL, $2); }
		| '-' expr	%prec OLD		{ $$ = expr_arithmetic(E_NEG, NULL, $2); }
		| factor				{ $$ = $1; }
		;

for_expr	: variable				{ $$ = $1; }
		| list					{ $$ = $1; }
		;

for_idlist	: id_list				{ $$ = expr_for_idlist($1); }
		;

where_clause	: WHERE expr				{ $$ = $2; }
		| /* empty */				{ $$ = NULL; }
		;

group_expr	: grpfunc				{ $$ = expr_grpfunc($1, NULL); }
		| grpfunc '(' expr ')'			{ $$ = expr_grpfunc($1, $3); }
		;

group_expr_sa	: grpfunc '(' expr ')'			{ $$ = expr_grpfunc_sa($1, $3); }
		;

grpfunc		: COUNT					{ $$ = E_COUNT; }
		| SUM					{ $$ = E_SUM; }
		| MIN					{ $$ = E_MIN; }
		| MAX					{ $$ = E_MAX; }
		;

factor		: '(' expr ')'				{ $$ = $2; }
		| IDENTIFIER arg_list			{ $$ = expr_call($1,$2); }
		| IDENTIFIER ASSIGN_OP expr		{ $$ = expr_assign($1, $3); }
		| group_expr_sa				{ $$ = $1; }
		| INTEGER				{ $$ = expr_integer($1); }
		| FLOAT					{ $$ = expr_float($1); }
		| STRING				{ $$ = expr_string($1); }
		| BOOLEAN				{ $$ = expr_boolean($1); }
		| KW_NULL				{ $$ = expr_null(); }
		| variable				{ $$ = $1; }
		| OLD variable				{ $$ = expr_old($2); }
		| ROWNUM '(' expr ')'			{ $$ = expr_rownum($3); }
		| list					{ $$ = $1; }
		;

variable	: IDENTIFIER opt_substr			{ $$ = expr_variable($1, $2); }
		| IDENTIFIER '[' index_list ']'		{ $$ = expr_indexed_table($1, $3); }
		| IDENTIFIER '.' IDENTIFIER opt_substr	{ $$ = expr_tablefield($1, $3, $4); }
		| IDENTIFIER '[' index_list ']' '.' IDENTIFIER opt_substr
							{ $$ = expr_indexed_tablefield($1, $3, $6, $7); }
		;

opt_substr	: '[' expr ':' expr ']'			{ $$ = expr_substr($2, $4); }
		| /* empty */				{ $$ = NULL; }
		;

index_list	: index_expr				{ $$ = expr_args(NULL, $1); }
		| index_list ',' index_expr		{ $$ = expr_args($1, $3); }
		;

index_expr	: expr					{ $$ = $1; }
		| '*'					{ $$ = expr_null(); }
		;

list		: '{' list_items '}'			{ $$ = expr_list($2); }
		| '{' '}'				{ $$ = expr_list(NULL); }
		;

list_items	: expr					{ $$ = expr_args(NULL, $1); }
		| list_items ',' expr			{ $$ = expr_args($1, $3); }
		;

arg_list	: '(' args ')'				{ $$ = $2; }
		;

args		: expr					{ $$ = expr_args(NULL, $1); }
		| args ',' expr				{ $$ = expr_args($1, $3); }
		;

block		: block_stmt				{ $$ = expr_args(NULL, $1); }
		| block ';' block_stmt			{ $$ = expr_args($1, $3); }
		;

block_stmt	: expr					{ $$ = $1; }
		| /* empty */				{ $$ = NULL; }
		;
%%

/* yyerror -
 *	Pass control to the "real" edits-engine error function.
 */
static void yyerror(char *s)
{
	Bre_Parse_Error(BREMSG_YACC_SYNTAX, s);
}
