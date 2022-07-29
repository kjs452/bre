/*** $Id$ ***/

/*
 */
#ifndef lint
static char rcsid[] = "@(#)$Id$";
#endif

/***********************************************************************
 * BRE Parse Tree Related Routines
 *
 * 'EXPR' DATASTRUCTURE:
 *	This structure is used to construct parse tree's for
 *	an entire expression.
 *
 *	Example:	foo + 12.3 >= 3000 and mystring = "hello"
 *
 *			+---------------+
 *			|     AND	|
 *			+--+---------+--+
 *			   |         |
 *		     +-----+         +------------+
 *		     |			          |
 *		     V                            V
 *		+---------------+	    +---------------+
 *		|      >=	|	    |      =        |
 *		+--+---------+--+	    +--+---------+--+
 *		   |	     |		       |	 |
 *	+----------+	     |		   +---+	 |
 *	|		     |		   |		 |
 *	V		     V		   V		 V
 * +---------+		+---------+	+-----------+	+---------+
 * |    +    |		|  3000   |	|'mystring' |	| "hello" |
 * +-+-----+-+		+---------+	+-----------+	+---------+
 *   |     |
 *   V     V
 * 'foo'  12.3
 *
 * Each EXPR node in the tree stores several bits of information:
 *	op	=	The kind of node this is, ie. E_ADD, E_EQ, E_INTEGER, ...
 *
 *	type	=	What data type this tree evaluates to.
 *
 *	constant =	This flag is true if the expression consists of all
 *			constant data (ie. "12+4*100")
 *
 *	symbol	=	Some expression nodes need a pointer to a symbol. Such
 *			as variables, function calls, etc..
 *
 *	value	=	Literal values like a string, or integer would store it
 *			in 'value'.
 *
 *	left	=	The left-side of the expression
 *	right	=	The right-side of the expression
 *
 * PARSE TREE BUILDING ALGORITHM:
 *	The yacc parser drives the action of building parse trees for
 *	expressions. Yacc is a bottom-up style parser, so that the tokens
 *	are first reduced into expression parse tree's. INTEGER's, FLOAT's
 *	and IDENTIFIERS will form the leaves of the parse tree.
 *
 *	Each grammar rule returns a parse tree, and parse tree's get
 *	joined together by rules such as:
 *
 *		expr	:  expr '+' expr
 *
 *	Building the parse tree for this is simply a matter of allocating
 *	a new EXPR node and attaching the left and right pointers to
 *	the sub-tree's.
 *
 *	If that were all we had to do, this source file would not be as
 *	large as it is.
 *
 * LANGUAGE SEMANTICS:
 *	As the parse tree is being constructed, we perform all SEMANTIC
 *	checks here. This means that all types have to be validated,
 *	the number of parameters used in calling a function, etc...
 *
 *	Throughout this source file, anytime a function performs a
 *	language check, I note what that check is under the
 *	comment 'LANGUAGE SEMANTICS'
 */

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <string.h>
#include <assert.h>

#include "bre_api.h"
#include "bre_prv.h"

/*
 * Forward declarations
 *
 */
static SYMBOL	*declare_variable(char *, SYM_TYPE);
static void	free_idlist(ID_LIST *);

typedef enum {
	STMT_UNKNOWN,
	STMT_EXPR,		/* an expression */
} STMT_TYPE;

/* ----------------------------------------------------------------------
 * PARSE CONTEXT VARIABLE 'pc'
 * This global structure is used to hold
 * some state information while the BRE language
 * file is being parsers. (pc stands for 'Parse Context').
 *
 */
static struct {
	SYMBOL	*rule;			/* non-NULL while parsing a rule */
	SYMBOL	*func;			/* non-NULL while parsing a function */
	SYMBOL	*group;			/* non-NULL while parsing a group */
	SYMBOL	*using_table;		/* for rules that have a using clause */

	STMT_TYPE	last_stmt;	/* type of last statement */
	int		stmt_count;	/* # of statements */
	SYM_TYPE	last_type;	/* type of statement last statement */

} pc;

static void reset_parse_context(void)
{
	pc.rule		= NULL;
	pc.func		= NULL;
	pc.group	= NULL;
	pc.using_table	= NULL;
	pc.last_stmt	= STMT_UNKNOWN;
	pc.stmt_count	= 0;
	pc.last_type.primary = T_UNKNOWN;
}

/***********************************************************************
 * Convert a sym->kind enum to a string.
 *
 */
static char *symkind_string(SYM_KIND kind)
{
	switch(kind) {
	case S_EXTFUNC:		return "external function";
	case S_EXTLOOKUP:	return "lookup";
	case S_CONSTANT:	return "constant";
	case S_FUNCTION:	return "function";
	case S_GROUP:		return "group";
	case S_RULE:		return "rule";
	case S_TABLE:		return "table";
	case S_FIELD:		return "field";
	case S_LUFIELD:		return "lookup field";
	case S_PARAMETER:	return "parameter";
	case S_VARIABLE:	return "variable";
	default:	BRE_ASSERT(0);
	}
}

/***********************************************************************
 * Convert a T_FLAGS enum into a string.
 *
 */
static char *tflags_string(T_FLAGS tf)
{
	switch(tf) {
	case T_UNKNOWN:		return "unknown";
	case T_MIXED:		return "mixed";
	case T_BOOLEAN:		return "boolean";
	case T_INTEGER:		return "integer";
	case T_FLOAT:		return "float";
	case T_STRING:		return "string";
	case T_LIST:		return "list";
	case T_ROW:		return "table row pointer";
	case T_REC:		return "lookup record";
	default:	BRE_ASSERT(0);
	}
}

/*----------------------------------------------------------------------
 * Convert a type 't' into a pretty name. ie. "list of integer"
 *
 */
static char *symtype_string(SYM_TYPE t)
{
	static char sbuf[100];

	if( t.primary == T_LIST ) {
		sprintf(sbuf, "list of %s", tflags_string(t.u.subtype));
		return sbuf;
	} else {
		return tflags_string(t.primary);
	}
}

/***********************************************************************
 * Convert a EXPR_OPERATOR into a string.
 *
 */
static char *operator_string(EXPR_OPERATOR op)
{
	switch(op) {
	case E_NONE:		BRE_ASSERT(0);
	case E_FORALL:		return "FOR ALL";
	case E_FORSOME:		return "FOR SOME";
	case E_COMPUTE:		return "COMPUTE";
	case E_IF:		return "IF/THEN";
	case E_IFELSE:		return "IF/THEN/ELSE";
	case E_OR:		return "OR";
	case E_AND:		return "AND";
	case E_IN:		return "IN";
	case E_BETWEEN:		return "BETWEEN";
	case E_LIKE:		return "LIKE";
	case E_NIN:		return "NOT IN";
	case E_NBETWEEN:	return "NOT BETWEEN";
	case E_NLIKE:		return "NOT LIKE";
	case E_EQ:		return "=";
	case E_NE:		return "<>";
	case E_LT:		return "<";
	case E_LE:		return "<=";
	case E_GT:		return ">";
	case E_GE:		return ">=";
	case E_CONCAT:		return "+";
	case E_ADD:		return "+";
	case E_ADD_LIST:	return "+";
	case E_SUB:		return "-";
	case E_MUL:		return "*";
	case E_DIV:		return "/";
	case E_MOD:		return "%";
	case E_NOT:		return "NOT";
	case E_NEG:		return "unary minus";
	case E_COUNT:		return "COUNT";
	case E_SUM:		return "SUM";
	case E_MIN:		return "MIN";
	case E_MAX:		return "MAX";
	case E_CALL:		return "function call";
	case E_INTEGER:		return "literal integer";
	case E_FLOAT:		return "literal float";
	case E_STRING:		return "literal string";
	case E_BOOLEAN:		return "literal boolean";
	case E_NULL:		return "null";
	case E_FIELD:		return "field";
	case E_OLDFIELD:	return "old field";
	case E_VARIABLE:	return "variable";
	case E_SUBSTR:		return "substring";
	case E_SUBLST:		return "sub list";
	case E_LOOKUP_INDEX:	return "lookup";
	case E_TABLE_INDEX:	return "table";
	case E_LIST_INDEX:	return "list";
	case E_LIST:		return "{...}";
	case E_WARN:		return "warn";
	case E_FAIL:		return "fail";
	case E_ASSIGN:		return "assignment";
	case E_BLOCK:		return "begin/end block";
	case E_CONSTANT:	return "constant";
	case E_GROUP:		return "group";
	case E_DOT:		return ".";
	case E_IMPLICIT:	return "implicit table row pointer";
	case E_ROWNUM:		return "ROWNUM()";
	default:		BRE_ASSERT(0);
	}
}

/*
 * Debugging routine.
 */
static void symbol_print(SYMBOL *sym)
{
	printf("Name=%s\n", sym->name);
	printf("Kind=%s\n", symkind_string(sym->kind));
}

/*----------------------------------------------------------------------
 * Returns true if 'type' is one of the basic types
 *
 */
static int is_basic_type(T_FLAGS type)
{
	return	type == T_BOOLEAN
		|| type == T_INTEGER
		|| type == T_FLOAT
		|| type == T_STRING;
}

/*----------------------------------------------------------------------
 * Returns true if the type flag type1 and type2 are compatible.
 * Two types are compatible if they are both a float or integer, or
 * if both are exactly the same.
 */
static int compatible_typeflags(T_FLAGS t1, T_FLAGS t2)
{
	if( (t1 == T_INTEGER || t1 == T_FLOAT)
			&& (t2 == T_INTEGER || t2 == T_FLOAT) )
		return 1;

	else if( is_basic_type(t1) && t1 == t2 )
		return 1;

	else
		return 0;
}

/*----------------------------------------------------------------------
 * Returns true if type1 and type2 are compatible.
 * Two types are compatible if they are both a float or integer, or
 * if both are exactly the same.
 */
static int compatible_types(SYM_TYPE t1, SYM_TYPE t2)
{
	if( (t1.primary == T_INTEGER || t1.primary == T_FLOAT)
			&& (t2.primary == T_INTEGER || t2.primary == T_FLOAT) )
		return 1;

	if( t1.primary == T_MIXED || t1.primary == T_UNKNOWN
		|| t2.primary == T_MIXED || t2.primary == T_UNKNOWN )
		return 0;

	if( t1.primary != t2.primary )
		return 0;

	if( is_basic_type(t1.primary) )
		return 1;

	if( t1.primary == T_LIST )
		return compatible_typeflags(t1.u.subtype, t2.u.subtype);

	if( t1.primary == T_ROW )
		return t1.u.symbol == t2.u.symbol;

	if( t1.primary == T_REC )
		return t1.u.symbol == t2.u.symbol;

	BRE_ASSERT(0);
}

/*----------------------------------------------------------------------
 * Returns true if type can be compared
 * using the operators: = <>
 *
 */
static int is_comparable_type(T_FLAGS type)
{
	return type == T_BOOLEAN
		|| type == T_INTEGER
		|| type == T_FLOAT
		|| type == T_STRING
		|| type == T_ROW
		|| type == T_REC;
}

/*----------------------------------------------------------------------
 * Returns true if type is an orderable type.
 *
 */
static int is_orderable_type(T_FLAGS type)
{
	return type == T_INTEGER || type == T_FLOAT || type == T_STRING;
}

/*----------------------------------------------------------------------
 * Returns true if type is a countable type.
 *
 */
static int is_countable_type(T_FLAGS type)
{
	return type == T_STRING || type == T_LIST || type == T_ROW;
}

/*----------------------------------------------------------------------
 * Returns true if type can be assigned or returned from
 * a function.
 *
 */
static int is_assignable_type(SYM_TYPE type)
{
	return is_basic_type(type.primary)
		|| (type.primary == T_LIST && is_basic_type(type.u.subtype))
		|| type.primary == T_REC
		|| type.primary == T_ROW;
}

/*----------------------------------------------------------------------
 * Allocate an EXPR node, and set the 'op' member to 'op'.
 *
 */
static EXPR *expr_new(EXPR_OPERATOR op)
{
	EXPR *expr;

	expr = (EXPR*)BRE_MALLOC( sizeof(EXPR) );
	if( expr == NULL ) {
		Bre_Error(BREMSG_DIE_NOMEM);
	}

	expr->op = op;

	return expr;
}

/*----------------------------------------------------------------------
 * IF e1 THEN e2
 *
 * LANGUAGE SEMANTICS:
 *	1. e1 must be a boolean expression.
 *	2. If 'e2' is boolean the type of the IF/THEN will be boolean.
 *	3. If 'e2' is NOT boolean the type will be T_UNKNOWN.
 *
 */
EXPR *expr_if(EXPR *e1, EXPR *e2)
{
	EXPR *e;

	e = expr_new(E_IF);

	e->left		= e1;
	e->right	= e2;
	e->constant	= e1->constant && e2->constant;

	if( e1->type.primary != T_BOOLEAN ) {
		Bre_Parse_Warn(BREMSG_IF_NOBOOL);

	}

	if( e2->type.primary != T_BOOLEAN ) {
		e->type.primary	= T_UNKNOWN;
	} else {
		e->type.primary	= T_BOOLEAN;
	}

	return e;
}

/*----------------------------------------------------------------------
 * IF e1 THEN e2 ELSE e3
 *
 * LANGUAGE SEMANTICS:
 *	1. e1 must be a boolean expression.
 *	2. If e2 and e3 are compatible types, then the type of the IF/THEN/ELSE will be
 *	   that type.
 *	3. If e2 and e3 are not compatible, then the type of the IF/THEN/ELSE will be
 *	   T_UNKNOWN.
 */
EXPR *expr_ifelse(EXPR *e1, EXPR *e2, EXPR *e3)
{
	EXPR *e, *expr;

	expr = expr_new(E_NONE);
	expr->left	= e2;
	expr->right	= e3;

	e = expr_new(E_IFELSE);
	e->left		= e1;
	e->right	= expr;
	e->constant	= e1->constant && e2->constant && e3->constant;

	if( e1->type.primary != T_BOOLEAN ) {
		Bre_Parse_Warn(BREMSG_IF_NOBOOL);
	}

	if( !compatible_types(e2->type, e3->type) ) {
		e->type.primary = T_UNKNOWN;
	} else if( e2->type.primary == T_FLOAT || e3->type.primary == T_FLOAT ) {
		e->type.primary = T_FLOAT;
	} else {
		e->type = e2->type;
	}

	return e;
}

/*----------------------------------------------------------------------
 * LOGICAL OPERATORS: OR, AND, NOT
 *	e1 OP e2
 *
 * LANGUAGE SEMANTICS:
 *	1. operands to these operators must be of type boolean.
 *
 */
EXPR *expr_logical(EXPR_OPERATOR type, EXPR *e1, EXPR *e2)
{
	EXPR *e;

	if( e1 && e1->type.primary != T_BOOLEAN ) {
		Bre_Parse_Warn(BREMSG_LOG_LNBOOL, operator_string(type));
	}

	if( e2->type.primary != T_BOOLEAN ) {
		Bre_Parse_Warn(BREMSG_LOG_RNBOOL, operator_string(type));
	}

	e = expr_new(type);
	e->type.primary	= T_BOOLEAN;
	e->constant	= (e1 && e1->constant) && e2->constant;
	e->left		= e1;
	e->right	= e2;

	return e;
}

/*----------------------------------------------------------------------
 * The IN operator.
 *	e1 IN e2
 *	e1 NOT IN e2
 *
 * LANGUAGE SEMANTICS
 *	1. 'e2' must be a list or string.
 *
 *	2. If e2 is a list, then it must contain
 *	   objects of type 'e1'
 *
 *	3. If e2 is a list, then it must be a list of basic types.
 *
 *	4. If e2 is a string, e1 must be a string. (this becomes a
 *		substring checker)
 *
 *	5. This operator always yeilds a boolean type.
 *
 *	6. e1 must be a basic type (integer, boolean, float, string, date,time)
 *
 */
EXPR *expr_in(EXPR_OPERATOR type, EXPR *e1, EXPR *e2)
{
	EXPR *e;

	if( e2->type.primary == T_LIST ) {
		if( !compatible_typeflags(e1->type.primary, e2->type.u.subtype) ) {
			Bre_Parse_Warn(BREMSG_INCOMPATABLE_TYPES, operator_string(type));
		}

	} else if( e2->type.primary == T_STRING ) {
		if( e1->type.primary != T_STRING ) {
			Bre_Parse_Warn(BREMSG_INCOMPATABLE_TYPES, operator_string(type));
		}
		
	} else {
		Bre_Parse_Warn(BREMSG_IN_BADRIGHT, operator_string(type));
	}

	e = expr_new(type);
	e->type.primary	= T_BOOLEAN;
	e->constant	= e1->constant && e2->constant;
	e->left		= e1;
	e->right	= e2;

	return e;
}

/*----------------------------------------------------------------------
 * The BETWEEN operator.
 *	e1 BETWEEN e2
 *	e1 NOT BETWEEN e2
 *
 * LANGUAGE SEMANTICS:
 *	1. e1 must be a orderable_type (integer,float,string,date,timestamp)
 *	2. e2 must be a list of a type compatible with e1.
 *	3. e2 must be a list of length exactly 2.
 *
 * IMPORTANT:	Check #3 cannot always be done at compile time. A run-time
 *		check is still nessesary.....
 */
EXPR *expr_between(EXPR_OPERATOR type, EXPR *e1, EXPR *e2)
{
	EXPR *e;

	if( e2->type.primary == T_LIST ) {
		if( !is_orderable_type(e1->type.primary) ) {
			Bre_Parse_Warn(BREMSG_WRONG_RIGHT, operator_string(type));
		}

		if( !compatible_typeflags(e1->type.primary, e2->type.u.subtype) ) {
			Bre_Parse_Warn(BREMSG_INCOMPATABLE_TYPES, operator_string(type));
		}

		if( e2->op == E_LIST && e2->value.i.val != 2 ) {
			Bre_Parse_Warn(BREMSG_BETWEEN_TWO, operator_string(type));
		}

	} else {
		Bre_Parse_Warn(BREMSG_BETWEEN_RIGHT, operator_string(type));
	}

	e = expr_new(type);
	e->type.primary	= T_BOOLEAN;
	e->constant	= e1->constant && e2->constant;
	e->left		= e1;
	e->right	= e2;

	return e;
}


/*----------------------------------------------------------------------
 * STRING MATCHING OPERATORS: LIKE, NOT LIKE
 *	e1 LIKE e2
 *	e1 NOT LIKE e2
 *
 * LANGUAGE SEMANTICS:
 *	1. Both operands must be string.
 *
 */
EXPR *expr_like(EXPR_OPERATOR type, EXPR *e1, EXPR *e2)
{
	EXPR *e;

	if( e1->type.primary != T_STRING ) {
		Bre_Parse_Warn(BREMSG_LEFT_STR, operator_string(type));
	} else if( e2->type.primary != T_STRING ) {
		Bre_Parse_Warn(BREMSG_RIGHT_STR, operator_string(type));
	}

	e = expr_new(type);
	e->type.primary	= T_BOOLEAN;
	e->constant	= e1->constant && e2->constant;
	e->left		= e1;
	e->right	= e2;

	return e;
}

/*----------------------------------------------------------------------
 * EQUALITY OPERATORS: = <>
 *	e1 OP e2
 *
 * LANGUAGE SEMANTICS:
 *	1. Both operands must be is_comparable_type()
 *	2. One of the operands is allowed to be 'null'.
 *
 */
EXPR *expr_equality(EXPR_OPERATOR type, EXPR *e1, EXPR *e2)
{
	EXPR *e;

	if( e1->op == E_NULL && e2->op == E_NULL ) {
		e1->type.primary = T_BOOLEAN;
		e2->type.primary = T_BOOLEAN;
	} else if( e1->op == E_NULL ) {
		e1->type = e2->type;	/* assign NULL node a type */
	} else if( e2->op == E_NULL ) {
		e2->type = e1->type;	/* assign NULL node a type */
	}

	if( !is_comparable_type(e1->type.primary)
					|| !compatible_types(e1->type, e2->type) ) {

		Bre_Parse_Warn(BREMSG_WRONG_TYPES, operator_string(type) );
	}

	e = expr_new(type);
	e->type.primary	= T_BOOLEAN;
	e->constant	= e1->constant && e2->constant;
	e->left		= e1;
	e->right	= e2;

	return e;
}

/*----------------------------------------------------------------------
 * RELATIONAL OPERATORS: > < >= <=
 *	e1 OP e2
 *
 * LANGUAGE SEMANTICS:
 *	1. Both operands must be is_comparable_type() (and compatible with each other)
 *
 */
EXPR *expr_relational(EXPR_OPERATOR type, EXPR *e1, EXPR *e2)
{
	EXPR *e;

	if( !compatible_types(e1->type, e2->type) ) {
		Bre_Parse_Warn(BREMSG_INCOMPATABLE_TYPES, operator_string(type));
	} else if( !is_orderable_type(e1->type.primary) ) {
		Bre_Parse_Warn(BREMSG_WRONG_TYPES, operator_string(type));
	}

	e = expr_new(type);
	e->type.primary	= T_BOOLEAN;
	e->constant	= e1->constant && e2->constant;
	e->left		= e1;
	e->right	= e2;

	return e;
}

/*----------------------------------------------------------------------
 * ADD OPERATOR: +
 *	e1 + e2
 *
 * LANGUAGE SEMANTICS:
 *	1. Both operands must be compatible.
 *	2. Must be a string, integer or float.
 *	3. A list and a basic type can be added.
 *
 */
EXPR *expr_add(EXPR *e1, EXPR *e2)
{
	EXPR *e;
	SYM_TYPE result;
	EXPR_OPERATOR op;

	if( e1->type.primary == T_LIST
				&& compatible_typeflags(e1->type.u.subtype, e2->type.primary)) {
		result	= e1->type;
		op = E_ADD_LIST;
	} else if( e2->type.primary == T_LIST
				&& compatible_typeflags(e2->type.u.subtype, e1->type.primary)) {
		result = e2->type;
		op = E_ADD_LIST;
	} else if( !compatible_types(e1->type, e2->type) ) {
		Bre_Parse_Warn(BREMSG_INCOMPATABLE_TYPES, "+");
		result.primary = T_UNKNOWN;
		op = E_ADD;
	} else if( e1->type.primary == T_STRING ) {
		result.primary = T_STRING;
		op = E_CONCAT;
	} else if( e1->type.primary == T_FLOAT || e2->type.primary == T_FLOAT ) {
		result.primary = T_FLOAT;
		op = E_ADD;
	} else if( e1->type.primary == T_INTEGER ) {
		result.primary = T_INTEGER;
		op = E_ADD;
	} else {
		Bre_Parse_Warn(BREMSG_ADD_ERR);
		result.primary = T_UNKNOWN;
		op = E_ADD;
	}

	e = expr_new(op);
	e->type		= result;
	e->constant	= e1->constant && e2->constant;
	e->left		= e1;
	e->right	= e2;

	return e;
}

/*----------------------------------------------------------------------
 * ARITHMETIC OPERATORS: - * / % unary minus
 *	e1 OP e2
 *
 * LANGUAGE SEMANTICS:
 *	1. Both operands must be compatible.
 *	2. Both operands must be integer or float.
 *	3. Mod operator is only valid for integer operands.
 *
 */
EXPR *expr_arithmetic(EXPR_OPERATOR type, EXPR *e1, EXPR *e2)
{
	EXPR *e;
	T_FLAGS result;
	SYM_TYPE t1, t2;

	if( e1 == NULL ) {
		t1 = e2->type;
		t2 = e2->type;
	} else {
		t1 = e1->type;
		t2 = e2->type;
	}

	if( !compatible_types(t1, t2) ) {
		Bre_Parse_Warn(BREMSG_INCOMPATABLE_TYPES, operator_string(type));
		result = T_UNKNOWN;
	} else if( t1.primary == T_FLOAT || t2.primary == T_FLOAT ) {
		if( type == E_MOD ) {
			Bre_Parse_Warn(BREMSG_NOTINTEGER, operator_string(type));
		}
		result = T_FLOAT;
	} else if( t1.primary == T_INTEGER ) {
		result = T_INTEGER;
	} else {
		Bre_Parse_Warn(BREMSG_NUMERICAL, operator_string(type));
		result = T_UNKNOWN;
	}

	e = expr_new(type);
	e->type.primary	= result;
	e->constant	= (e1 && e1->constant) && e2->constant;
	e->left		= e1;
	e->right	= e2;

	return e;
}

/*----------------------------------------------------------------------
 * GROUP FUNCTION APPEARING INSIDE OF COMPUTE LOOP.
 *
 * The GROUP FUNCTIONS: SUM, MIN, MAX, COUNT
 *	SUM(e1)
 *	MIN(e1), MAX(e1), COUNT
 *
 * Check that the proper types are being used.
 *
 * LANGUAGE SEMANTICS:
 *	1. COUNT cannot have any argument.
 *	2. SUM, MIN, MAX must have exactly 1 argument.
 *	3. The type of argument for SUM, MIN, MAX must be INTEGER or FLOAT.
 *	4. MIN and MAX will also work with string types.
 *	5. The type returned is equal to the type of the argument, except for
 *	   COUNT() which always returns an integer.
 *	6. If the expression 'e1' consists of a E_DOT operator with a E_GROUP
 *	   as the table, then the group function will yeild a list of <basic type>
 */
EXPR *expr_grpfunc(EXPR_OPERATOR type, EXPR *e1)
{
	EXPR *e;

	e = expr_new(type);

	e->left		= NULL;
	e->right	= e1;
	e->constant	= 0;

	if( e1 == NULL ) {
		if( type == E_MIN || type == E_MAX || type == E_SUM ) {
			Bre_Parse_Warn(BREMSG_GROUP_ARGS, operator_string(type));
		}
		e->type.primary = T_INTEGER;
	} else {
		if( type == E_COUNT ) {
			Bre_Parse_Warn(BREMSG_COUNT_ARGS);
			e->type.primary = T_INTEGER;
		} else if( e1->type.primary == T_INTEGER) {
			e->type.primary = T_INTEGER;
		} else if( e1->type.primary == T_FLOAT ) {
			e->type.primary = T_FLOAT;
		} else if( e1->type.primary == T_STRING && type != E_SUM) {
			e->type.primary = T_STRING;
		} else {
			Bre_Parse_Warn(BREMSG_GROUP_INVTYPE, operator_string(type));
			e->type.primary = T_INTEGER;
		}
	}

	return e;
}

/*----------------------------------------------------------------------
 * GROUP FUNCTIONS (Stand Alone Case)
 *
 * The GROUP FUNCTIONS: SUM, MIN, MAX, COUNT
 *	SUM(e1), MIN(e1), MAX(e1), COUNT(e1)
 *
 * This function is for the group functions when the appear
 * outside of a FOR ... COMPUTE ... WHERE construct (ie. "stand alone")
 *
 * LANGUAGE SEMANTICS:
 *	1. COUNT, SUM, MIN, MAX must have exactly 1 argument.
 *	2. The expression e1, argument for SUM, MIN, MAX must be:
 *		a. Of type "List of INTEGER"
 *		b. Of type "List of FLOAT"
 *		b. Of type "List of STRING"
 *		c. A E_DOT opererator with no indexing (on a table, not a lookup)
 *		   and returning an string, integer or float.
 *
 *	3. The type returned is STRING, INTEGER or FLOAT depending on 'e1'. Except
 *	   COUNT() which always returns an integer.
 *
 *	4. The COUNT function can take as argument a string, list or table name.
 *
 *	5. If the expression consists of a '.' operator with a GROUP name, then
 *	   the return type will be a list of <some basic type>.
 */
EXPR *expr_grpfunc_sa(EXPR_OPERATOR type, EXPR *e1)
{
	EXPR *e;

	e = expr_new(type);

	e->left		= NULL;
	e->right	= e1;
	e->constant	= 0;

	BRE_ASSERT( e1 != NULL );

	if( type == E_COUNT ) {
		if( !is_countable_type(e1->type.primary) ) {
			Bre_Parse_Warn(BREMSG_GROUP_INVTYPE, "GROUP");
		}
		e->type.primary = T_INTEGER;

	} else if( e1->type.primary == T_LIST && e1->type.u.subtype == T_INTEGER ) {
		e->type.primary = T_INTEGER;

	} else if( e1->type.primary == T_LIST && e1->type.u.subtype == T_FLOAT ) {
		e->type.primary = T_FLOAT;

	} else if( e1->type.primary == T_LIST && e1->type.u.subtype == T_STRING && type != E_SUM ) {
		e->type.primary = T_STRING;

	} else if( e1->op == E_DOT
			&& (e1->left->op == E_TABLE_INDEX || e1->left->op == E_IMPLICIT )
			&& e1->left->right == NULL
			&& (e1->type.primary == T_INTEGER || e1->type.primary == T_FLOAT
				|| e1->type.primary == T_STRING) ) {
		e->type.primary = e1->type.primary;

	} else if( e1->op == E_DOT && e1->left->op == E_GROUP
			&& (e1->type.primary == T_INTEGER || e1->type.primary == T_FLOAT
						|| e1->type.primary == T_STRING) ) {
		e->type.primary = T_LIST;
		e->type.u.subtype = e1->type.primary;

	} else {
		Bre_Parse_Warn(BREMSG_GROUP_INVTYPE, operator_string(type));
		e->type.primary = T_INTEGER;
	}

	return e;
}

/*----------------------------------------------------------------------
 * Check that the arguments 'args' match the function 'sym' for
 * number of arguments and types.
 *
 */
static void check_function_args(SYMBOL *sym, EXPR *args)
{
	EXPR *curr, *arg;
	SYMBOL_LIST *curr_sym;
	SYMBOL *sarg;
	int i, args_count;
	char num1[30], num2[30];

	args_count = (args==NULL) ? 0 : args->value.i.val;

	if( args_count < sym->u.fun.nargs ) {
		sprintf(num1, "%d", args->value.i.val);
		sprintf(num2, "%d", sym->u.fun.nargs);
		Bre_Parse_Warn(BREMSG_ARGS_FEW, sym->name, num1, num2);
		return;
	}

	if( args_count > sym->u.fun.nargs ) {
		sprintf(num1, "%d", args->value.i.val);
		sprintf(num2, "%d", sym->u.fun.nargs);
		Bre_Parse_Warn(BREMSG_ARGS_MANY, sym->name, num1, num2);
		return;
	}

	/*
	 * Check each arguments' type is compatible with
	 * the proto-type
	 */
	i = 0;
	curr_sym = sym->u.fun.parms;
	for(curr = args; curr; curr=curr->right) {
		arg	= curr->left;
		sarg	= curr_sym->sym;

		if( arg->op == E_NULL )
			arg->type = sarg->u.par.type;	/* assign NULL node a type */

		if( !compatible_types(arg->type, sarg->u.par.type) ) {
			sprintf(num1, "%d", i+1);
			Bre_Parse_Warn(BREMSG_ARGS_INCOMPATIBLE, num1,
					tflags_string(arg->type.primary),
					tflags_string(sarg->u.par.type.primary));
		}

		i += 1;
		curr_sym = curr_sym->next;
	}
}

SYM_TYPE type_from_char(char c)
{
	SYM_TYPE t;

	switch( c ) {
	case 'B':
		t.primary = T_BOOLEAN;
		break;
	case 'I':
		t.primary = T_INTEGER;
		break;
	case 'F':
		t.primary = T_FLOAT;
		break;
	case 'S':
		t.primary = T_STRING;
		break;
	default:
		BRE_ASSERT(0);
	}

	return t;
}

/*----------------------------------------------------------------------
 * Get the type that an external function returns.
 */
static SYM_TYPE get_ret_type(SYMBOL *sym)
{
	SYM_TYPE t;

	t = type_from_char(sym->u.ext.args_out[0]);
	return t;
}

/*----------------------------------------------------------------------
 * Check that the arguments 'args' match the external function 'sym' for
 * number of arguments and types.
 *
 */
static void check_external_function_args(SYMBOL *sym, EXPR *args)
{
	EXPR *curr, *arg;
	int i, nargs, args_count;
	char *p;
	SYM_TYPE par_type;
	char num1[30], num2[30];

	args_count = (args==NULL) ? 0 : args->value.i.val;

	nargs = strlen(sym->u.ext.args_in);

	if( args_count < nargs ) {
		sprintf(num1, "%d", args->value.i.val);
		sprintf(num2, "%d", nargs);
		Bre_Parse_Warn(BREMSG_ARGS_FEW, sym->name, num1, num2);
		return;
	}

	if( args_count > nargs ) {
		sprintf(num1, "%d", args->value.i.val);
		sprintf(num2, "%d", nargs);
		Bre_Parse_Warn(BREMSG_ARGS_MANY, sym->name, num1, num2);
		return;
	}

	/*
	 * Check each arguments' type is compatible with
	 * the proto-type
	 */
	i = 0;
	p = sym->u.ext.args_in;
	for(curr = args; curr; curr=curr->right) {
		arg = curr->left;
		par_type = type_from_char(*p);

		if( arg->op == E_NULL )
			arg->type = par_type;	/* assign NULL node a type */

		if( !compatible_types(arg->type, par_type) ) {
			sprintf(num1, "%d", i+1);
			Bre_Parse_Warn(BREMSG_ARGS_INCOMPATIBLE, num1,
					tflags_string(arg->type.primary),
					tflags_string(par_type.primary) );
		}

		i += 1;
		p++;
	}
}

/*----------------------------------------------------------------------
 * A Function Call:
 *	id ( args )
 *
 * This could be an external function call, or a internally defined function.
 *
 * LANGUAGE SEMANTICS:
 *	1. The symbol 'id' must be an external function, or a internally defined
 *	   function.
 *	2. The number of arguments must match that of its definition.
 *	3. The types of the arguments must be compatible.
 *	4. The type of this expression is the type the function returns.
 *	5. A 'null' argument is assumed to be the type of that parameter.
 *	6. Recursion is not allowed.
 *
 */
EXPR *expr_call(char *id, EXPR *args)
{
	EXPR *e;
	SYMBOL *sym;

	e = expr_new(E_CALL);
	e->left		= NULL;
	e->right	= args;

	sym = sym_lookup(id);
	if( sym == NULL ) {
		Bre_Parse_Warn(BREMSG_NOSUCH_FUNCTION, id);
		e->type.primary	= T_UNKNOWN;
		e->constant	= 0;
		e->symbol	= NULL;
	} else if( sym->kind != S_EXTFUNC && sym->kind != S_FUNCTION ) {
		Bre_Parse_Warn(BREMSG_INV_FUNCCALL, symkind_string(sym->kind), sym->name);
		e->type.primary	= T_UNKNOWN;
		e->constant	= 0;
		e->symbol	= NULL;
	} else if( sym->kind == S_FUNCTION && pc.func == sym ) {
		Bre_Parse_Warn(BREMSG_INV_RECURSE, id);
		e->type.primary	= T_UNKNOWN;
		e->constant	= 0;
		e->symbol	= NULL;

	} else if( sym->kind == S_FUNCTION ) {
		e->symbol	= sym;
		e->constant	= 0;
		e->type		= sym->u.fun.ret_type;

		sym->u.fun.refcount += 1;

		check_function_args(sym, args);
	} else {
		e->symbol	= sym;
		e->constant	= 0;
		e->type		= get_ret_type(sym);

		sym->u.ext.refcount += 1;

		check_external_function_args(sym, args);
	}

	BRE_FREE(id);

	return e;
}

/*----------------------------------------------------------------------
 * Integer Literal: 200, 56, 0, ...
 *
 */
EXPR *expr_integer(long val)
{
	EXPR *expr;

	expr = expr_new(E_INTEGER);

	expr->constant		= 1;
	expr->type.primary	= T_INTEGER;
	expr->value.i.val	= val;
	expr->value.i.nil	= 0;
	expr->left		= NULL;
	expr->right		= NULL;

	return expr;
}

/*----------------------------------------------------------------------
 * Floating Point Literal: 200.1, 56.5, 0.0, ...
 *
 */
EXPR *expr_float(double val)
{
	EXPR *expr;

	expr = expr_new(E_FLOAT);

	expr->constant		= 1;
	expr->type.primary	= T_FLOAT;
	expr->value.f.val	= val;
	expr->left		= NULL;
	expr->right		= NULL;

	return expr;
}

/*----------------------------------------------------------------------
 * String Literal: "Hello", "", "Junk"
 * Assumes the string 'str' has already been
 * allocated using 'malloc()'.
 *
 */
EXPR *expr_string(char *str)
{
	EXPR *expr;

	expr = expr_new(E_STRING);

	expr->constant		= 1;
	expr->type.primary	= T_STRING;
	expr->value.s.str	= str;
	expr->value.s.len	= strlen(str);
	expr->left		= NULL;
	expr->right		= NULL;

	return expr;
}

/*----------------------------------------------------------------------
 * Boolean Literal: true false
 *
 */
EXPR *expr_boolean(long val)
{
	EXPR *expr;

	expr = expr_new(E_BOOLEAN);

	expr->constant		= 1;
	expr->type.primary	= T_BOOLEAN;
	expr->value.i.val	= val;
	expr->value.i.nil	= 0;
	expr->left		= NULL;
	expr->right		= NULL;

	return expr;
}

/*----------------------------------------------------------------------
 * The keyword 'null'.
 *
 */
EXPR *expr_null(void)
{
	EXPR *expr;

	expr = expr_new(E_NULL);

	expr->constant		= 1;
	expr->type.primary	= T_UNKNOWN;
	expr->value.i.val	= 0;
	expr->value.i.nil	= 1;
	expr->left		= NULL;
	expr->right		= NULL;

	return expr;
}

/*----------------------------------------------------------------------
 * Argument List
 *
 *	(....args...., e1)
 *
 * This function creats a list of expressions.
 *
 * If 'e1' is NULL, then simply return 'args'. This
 * is how the null statement ';' is handled.
 * (Null statements occur inside of the BEGIN ... END blocks)
 *
 * The head of the argument list will contain:
 *	a. expr->constant	<- true if all expressions are constants.
 *	b. expr->type		<- set to the type of all arguments.
 *				   or T_MIXED if the types vary inside the
 *				   argument list.
 *	c. expr->value.i.val	<- number of items in arg list.
 */
EXPR *expr_args(EXPR *args, EXPR *e1)
{
	EXPR *e, *endp;

	if( e1 == NULL )
		return args;

	e = expr_new(E_NONE);
	e->left  = e1;
	e->right = NULL;

	if( args == NULL ) {
		e->type			= e1->type;
		e->constant		= e1->constant;
		e->value.i.val		= 1;
		return e;
	} else {
		/*
		 * Attatch 'e' to end of list (where endp points)
		 */
		endp=args;
		while( endp->right )
			endp = endp->right;
		endp->right = e;

		/*
		 * Set header information for list:
		 *	1. If we were constant before, and the new element
		 *	   is constant, then the list is still constant.
		 *	2. Increment the counter for the number of items.
		 *	3. If the types are compatible, then we keep the
		 *	   type of the first item, otherwise we set the
		 *	   type to T_MIXED.
		 */
		args->constant		= args->constant && e1->constant;
		args->value.i.val	+= 1;

		if( !compatible_types(args->type, e1->type) ) {
			args->type.primary = T_MIXED;
		} else if( e1->type.primary == T_FLOAT ) {
			/*
			 * a list containing a mixture of float's and integer's
			 * becomes a list of float's.
			 */
			args->type.primary = T_FLOAT;
		}

		return args;
	}
}

/*----------------------------------------------------------------------
 * List
 *
 *	{ ...items... }
 *
 * LANGUAGE SEMANTICS:
 *	1. A list can only contain BOOLEAN, INTEGER, FLOAT, STRING, DATE
 *	   or TIMESTAMP types.
 *
 *	2. A list is allowed to contain a mixture of these data types.
 *
 *	3. The 'null' keyword cannot be used as one of the elements.
 *
 *	4. An empty list has an T_UNKNOWN type.
 *
 */
EXPR *expr_list(EXPR *items)
{
	EXPR *e, *curr, *item;
	int i, bad;
	char num1[30];

	e = expr_new(E_LIST);
	e->left		= NULL;
	e->right	= items;

	if( items == NULL ) {
		e->constant	= 1;
		e->value.i.val	= 0;
		e->type.primary	= T_LIST;
		e->type.u.subtype = T_UNKNOWN;
		return e;
	}

	e->constant	= items->constant;
	e->value.i.val	= items->value.i.val;

	/*
	 * Check each item in the list and make sure
	 * they are all of basic type.
	 */
	i = 0;
	bad = 0;
	for(curr=items; curr; curr=curr->right) {
		item = curr->left;
		if( item->op == E_NULL ) {
			sprintf(num1, "%d", i+1);
			Bre_Parse_Warn(BREMSG_LIST_NULL, num1);
			bad += 1;
		} else if( !is_basic_type(item->type.primary) ) {
			sprintf(num1, "%d", i+1);
			Bre_Parse_Warn(BREMSG_LIST_TYPE, tflags_string(item->type.primary), num1);
			bad += 1;
		}
		i += 1;
	}

	if( bad ) {
		e->type.primary = T_UNKNOWN;
	} else {
		e->type.primary = T_LIST;
		e->type.u.subtype = items->type.primary;
	}

	return e;
}

/*
 * Recursively look at the parse tree 'e'. Change any E_FIELD node, to
 * an E_OLDFIELD node.
 *
 */
static int set_old_flag(EXPR *e)
{
	int n, m, found;

	if( e == NULL )
		return 0;

	found = 0;
	if( e->op == E_FIELD ) {
		e->op = E_OLDFIELD;
		found = 1;
	}

	n = set_old_flag(e->left);
	m = set_old_flag(e->right);

	return n || m || found;
}

/*----------------------------------------------------------------------
 * Old operator
 *
 *	old e
 *
 * Instructs the compiler that we want to calculate
 * the 'old' value.
 *
 * LANGUAGE SEMANTICS:
 *	1. The old operator can only be applied to FIELD references.
 *
 */
EXPR *expr_old(EXPR *e)
{
	int found;

	found = set_old_flag(e);

	if( !found ) {
		Bre_Parse_Warn(BREMSG_OLD_ERR);
	}

	return e;
}

/*----------------------------------------------------------------------
 * ROWNUM function
 *
 * LANGUAGE SEMANTICS:
 *	1. The expression 'e1' must be of type 'T_ROW'
 *	2. This function returns an integer type.
 *
 */
EXPR *expr_rownum(EXPR *e1)
{
	EXPR *e;

	if( e1->type.primary != T_ROW ) {
		Bre_Parse_Warn(BREMSG_ROWNUM_TYPE, operator_string(E_ROWNUM));
	}

	e = expr_new(E_ROWNUM);
	e->left		= NULL;
	e->right	= e1;
	e->constant	= 0;
	e->type.primary	= T_INTEGER;

	return e;
}

/*----------------------------------------------------------------------
 * Substring Reference:
 *	[ e1 : e2 ]
 *
 * LANGUAGE SEMANTICS:
 *	1. Both expressions must be of type integer.
 *	
 */
EXPR *expr_substr(EXPR *e1, EXPR *e2)
{
	EXPR *e;

	if( e1->type.primary != T_INTEGER ) {
		Bre_Parse_Warn(BREMSG_SUBSTR_INT1);
	}
	if( e2->type.primary != T_INTEGER ) {
		Bre_Parse_Warn(BREMSG_SUBSTR_INT2);
	}

	e = expr_new(E_NONE);
	e->type.primary	= T_INTEGER;
	e->constant	= e1->constant && e2->constant;
	e->left		= e1;
	e->right	= e2;

	return e;
}

/*----------------------------------------------------------------------
 * Process an expression and an OPTIONAL substring specifier.
 *
 *	e1 [ n : m ]
 *
 * LANGUAGE SEMANTICS:
 *	1. The expression 'e1' must be a string type or list.
 *
 */
EXPR *apply_substring(EXPR *e1, EXPR *e2)
{
	EXPR *e;
	EXPR_OPERATOR op;

	if( e2 == NULL )
		return e1;

	BRE_ASSERT(e2->op == E_NONE);

	if( e1->type.primary == T_STRING ) {
		op = E_SUBSTR;
	} else if( e1->type.primary == T_LIST ) {
		op = E_SUBLST;
	} else {
		op = E_SUBSTR;
		Bre_Parse_Warn(BREMSG_SUBSTR_COMPAT, tflags_string(e1->type.primary));
	}

	e = expr_new(op);
	e->type		= e1->type;
	e->left		= e1;
	e->right	= e2;
	e->constant	= e1->constant && e2->constant;

	return e;
}

static EXPR *expr_implicit(SYMBOL *table)
{
	EXPR *e;

	BRE_ASSERT(table != NULL);

	e = expr_new(E_IMPLICIT);
	e->left		= NULL;
	e->right	= NULL;
	e->constant	= 0;
	e->type.primary	= T_ROW;
	e->type.u.symbol = table;

	return e;
}

static EXPR *expr_field(SYMBOL *sym)
{
	EXPR *e;

	e = expr_new(E_FIELD);
	e->left		= NULL;
	e->right	= NULL;
	e->constant	= 0;
	e->type		= sym->u.fld.type;
	e->symbol	= sym;

	sym->u.fld.refcount += 1;

	return e;
}

static EXPR *expr_lufield(SYMBOL *sym)
{
	EXPR *e;

	e = expr_new(E_LUFIELD);
	e->left		= NULL;
	e->right	= NULL;
	e->constant	= 0;
	e->type		= sym->u.luf.type;
	e->symbol	= sym;

	return e;
}

/*----------------------------------------------------------------------
 * Variable Reference (with optional substring spec e1)
 *	IDENTIFIER
 *	IDENTIFIER [n:m]
 *
 * LANGUAGE SEMANTICS:
 *	1. IDENTIFIER will be one of the following:
 *		a. CONSTANT
 *		b. PARAMETER
 *		c. VARIABLE
 *		d. FUNCTION with no arguments
 *		e. GROUP
 *		f. FIELD NAME
 *		g. TABLE NAME
 *		h. EXTERNAL FUNCTION with no arguments
 *	2. A variable must be assigned to before being used.
 *	3. Search order: Use the using clause first, then a regular lookup.
 */
EXPR *expr_variable(char *id, EXPR *e1)
{
	EXPR *e;
	SYMBOL *sym;

	sym = NULL;
	if( pc.using_table )
		sym = sym_lookup_pair(pc.using_table->name, id);

	if( sym == NULL )
		sym = sym_lookup(id);

	if( sym == NULL ) {
		Bre_Parse_Warn(BREMSG_UNDEF_VAR, id);
		e = expr_integer(0);
		goto done;
	}

	switch( sym->kind ) {
	case S_EXTFUNC:
		e = expr_call(BRE_STRDUP(id), NULL);
		break;

	case S_EXTLOOKUP:
		Bre_Parse_Warn(BREMSG_INVCONTEXT_LOOKUP, sym->name);
		e = expr_integer(0);
		break;

	case S_CONSTANT:
		e = expr_new(E_CONSTANT);
		e->left		= NULL;
		e->right	= NULL;
		e->constant	= 1;
		e->type		= sym->u.con.type;
		e->symbol	= sym;

		sym->u.con.refcount += 1;
		break;

	case S_FUNCTION:
		e = expr_call(BRE_STRDUP(id), NULL);
		break;

	case S_GROUP:
		e = expr_new(E_GROUP);
		e->left		= NULL;
		e->right	= NULL;
		e->constant	= 0;
		e->type.primary	= T_ROW;
		e->type.u.symbol= sym->u.grp.using;
		e->symbol	= sym;
		sym->u.grp.refcount += 1;
		break;

	case S_RULE:
		BRE_ASSERT(0);

	case S_TABLE:
		/*
		 * Table name
		 */
		e = expr_new(E_TABLE_INDEX);
		e->left		= NULL;
		e->right	= NULL;
		e->constant	= 0;
		e->type.primary	= T_ROW;
		e->type.u.symbol = sym;
		e->symbol	= sym;
		break;

	case S_FIELD:
		/*
		 * Implicit table.field access
		 */
		e = expr_new(E_DOT);
		e->left		= expr_implicit(pc.using_table);
		e->right	= expr_field(sym);
		e->constant	= 0;
		e->type		= e->right->type;
		break;

	case S_LUFIELD:
		BRE_ASSERT(0);

	case S_PARAMETER:
		e = expr_new(E_VARIABLE);
		e->left		= NULL;
		e->right	= NULL;
		e->constant	= 0;
		e->type		= sym->u.par.type;
		e->symbol	= sym;

		sym->u.par.refcount += 1;
		break;

	case S_VARIABLE:
		e = expr_new(E_VARIABLE);
		e->left		= NULL;
		e->right	= NULL;
		e->constant	= 0;
		e->type		= sym->u.var.type;
		e->symbol	= sym;

		if( sym->u.var.assigned == 0 ) {
			Bre_Parse_Warn(BREMSG_NOT_INIT, sym->name);
		}

		sym->u.var.refcount += 1;
		break;

	default:
		BRE_ASSERT(0);
	}

done:
	BRE_FREE(id);
	return apply_substring(e, e1);
}

/*----------------------------------------------------------------------
 * Return a parse tree for a bogus indexing. This
 * is used to allow parsing to continue.
 *
 */
static EXPR *expr_bogus_indexing(SYMBOL *sym, EXPR *exprs)
{
	EXPR *e;
	T_FLAGS tflags;

	BRE_ASSERT(sym != NULL);

	if( sym->kind == S_EXTLOOKUP )
		tflags = T_REC;
	else
		tflags = T_ROW;

	e = expr_new(E_TABLE_INDEX);
	e->left		= NULL;
	e->right	= exprs;
	e->constant	= 0;
	e->type.primary	= tflags;
	e->type.u.symbol= sym;
	e->symbol	= sym;

	return e;
}

/*----------------------------------------------------------------------
 * Check for a valid table indexing, and return
 * a parse tree for the construction.
 *
 */
static EXPR *do_table_indexing(SYMBOL *sym, EXPR *exprs)
{
	EXPR *e;

	if( exprs->value.i.val != 1 ) {
		Bre_Parse_Warn(BREMSG_ILLEGAL_INDEXING, sym->name);
		e = expr_bogus_indexing(sym, exprs);

	} else if( exprs->type.primary != T_INTEGER ) {
		Bre_Parse_Warn(BREMSG_NONINT_INDEXING, sym->name);
		e= expr_bogus_indexing(sym, exprs);

	} else {
		e = expr_new(E_TABLE_INDEX);

		e->left		= NULL;
		e->right	= exprs;
		e->constant	= 0;
		e->type.primary	= T_ROW;
		e->type.u.symbol= sym;
		e->symbol	= sym;

	}

	return e;
}

/*----------------------------------------------------------------------
 * Check for correct lookup table indexing, and
 * return a parse tree.
 *
 */
static EXPR *do_lookup_indexing(SYMBOL *sym, EXPR *exprs)
{
	EXPR *e;
	int nkeys, i;
	char *p;
	EXPR *curr, *arg;
	SYM_TYPE key_type;
	char num1[30];

	nkeys = strlen(sym->u.ext.args_in);

	if( exprs->value.i.val > nkeys ) {
		sprintf(num1, "%d", nkeys);
		Bre_Parse_Warn(BREMSG_MANY_KEYS, sym->name, num1);
		e = expr_bogus_indexing(sym, exprs);

	} else if( exprs->value.i.val < nkeys ) {
		sprintf(num1, "%d", nkeys);
		Bre_Parse_Warn(BREMSG_FEW_KEYS, sym->name, num1);
		e = expr_bogus_indexing(sym, exprs);
	} else {
		/*
		 * Check for proper types of each lookup key
		 */
		i = 0;
		p = sym->u.ext.args_in;
		for(curr=exprs; curr; curr=curr->right) {
			arg = curr->left;
			key_type = type_from_char(*p);

			if( arg->op == E_NULL )
				arg->type = key_type;	/* assign NULL node a type */

			if( !compatible_types(arg->type, key_type) ) {
				sprintf(num1, "%d", i+1);
				Bre_Parse_Warn(BREMSG_INVTYPE_KEYS, num1,
						sym->name,
						tflags_string(arg->type.primary),
						tflags_string(key_type.primary) );
			}

			i += 1;
			p++;
		}

		e = expr_new(E_LOOKUP_INDEX);

		e->left		= NULL;
		e->right	= exprs;
		e->constant	= 0;
		e->type.primary	= T_REC;
		e->type.u.symbol= sym;
		e->symbol	= sym;
	}

	return e;
}

/*----------------------------------------------------------------------
 * Indexing a variable,parameter,function,constant. Only allowed if
 * the type is T_LIST.
 *
 */
static EXPR *do_other_indexing(SYMBOL *sym, EXPR *exprs)
{
	EXPR *e, *evar;
	SYM_TYPE type;
	char *id;

	id = BRE_STRDUP(sym->name);
	evar = expr_variable(id, NULL);

	if( evar->type.primary != T_LIST ) {
		Bre_Parse_Warn(BREMSG_INDEX_VARPAR, symkind_string(sym->kind), sym->name);
		type.primary = T_LIST;
		type.u.subtype = T_INTEGER;

	} else if( exprs->value.i.val != 1 || exprs->type.primary != T_INTEGER ) {
		Bre_Parse_Warn(BREMSG_VARPAR_NOTINT, symkind_string(sym->kind), sym->name);
		type.primary = T_LIST;
		type.u.subtype = T_INTEGER;

	} else {
		type = evar->type;
	}

	e = expr_new(E_LIST_INDEX);
	e->left		= evar;
	e->right	= exprs;
	e->constant	= 0;
	e->type.primary	= type.u.subtype;
	e->symbol	= sym;

	return e;
}

/*----------------------------------------------------------------------
 * Indexed Table
 *	IDENTIFIER [ exprs ]
 *
 * LANGUAGE SEMANTICS:
 *	1. IDENTIFIER can be:
 *		a. TABLE
 *		b. EXTLOOKUP
 *		c. variable/parameter that is a list.
 *	2. If IDENTIFIER is a table, then there can only be
 *	   one expression in exprs, and it must evaluate to T_INTEGER.
 *
 *	3. If IDENTIFIER is a EXTLOOKUP, then each expression must match
 *	   the proto-type for that lookup table.
 *
 *	4. NULL can be used as a valid key.
 *
 */
EXPR *expr_indexed_table(char *id, EXPR *exprs)
{
	EXPR *e;
	SYMBOL *sym;

	sym = sym_lookup(id);
	if( sym == NULL ) {
		if( pc.using_table )
			sym = sym_lookup_pair(pc.using_table->name, id);
		else
			sym = NULL;

		if( sym )
			Bre_Parse_Error(BREMSG_INVINDEX_FIELD, id);
		else
			Bre_Parse_Error(BREMSG_UNKNOWN_IDENTIFIER, id);

	} else if( sym->kind == S_TABLE ) {
		e = do_table_indexing(sym, exprs);

	} else if( sym->kind == S_EXTLOOKUP ) {
		e = do_lookup_indexing(sym, exprs);

	} else if( sym->kind == S_VARIABLE || sym->kind == S_PARAMETER
			|| sym->kind == S_CONSTANT || sym->kind == S_FUNCTION ) {
		e = do_other_indexing(sym, exprs);

	} else {
		Bre_Parse_Warn(BREMSG_ILLEGAL_CONTEXT, symkind_string(sym->kind), id);
		e = expr_bogus_indexing(sym, exprs);
	}

	BRE_FREE(id);
	return e;
}

/*----------------------------------------------------------------------
 * Table Field reference (with optional substring spec e1)
 *	IDENTIFIER1 . IDENTIFIER2
 *	IDENTIFIER1 . IDENTIFIER2 [n:m]
 *
 * LANGUAGE SEMANTICS:
 *	1. IDENTIFIER1 must be the a table name or variable/parameter of type T_REC
 *		and T_ROW.
 *
 *	2. IDENTIFIER2 must be a field name in the domain of IDENTIFIER1.
 *	3. IDENTIFIER1 (if it is a variable) must be initialized before it
 *	   is used.
 */
EXPR *expr_tablefield(char *id1, char *id2, EXPR *e1)
{
	char *tbl;
	EXPR *e, *e_tbl, *e_fld;
	SYMBOL *sym1, *sym2;

	sym1 = sym_lookup(id1);
	if( sym1 == NULL ) {
		Bre_Parse_Error(BREMSG_UNKNOWN_IDENTIFIER, id1);
	} else if( sym1->kind == S_TABLE ) {
		tbl = sym1->name;
		sym2 = sym_lookup_pair(tbl, id2);

		e_tbl = expr_new(E_TABLE_INDEX);
		e_tbl->left		= NULL;
		e_tbl->right		= NULL;
		e_tbl->constant		= 0;
		e_tbl->type.primary	= T_ROW;
		e_tbl->type.u.symbol	= sym1;
		e_tbl->symbol		= sym1;

	} else if( sym1->kind == S_EXTLOOKUP ) {
		Bre_Parse_Error(BREMSG_MISSING_KEYS, sym1->name);

	} else if( sym1->kind == S_VARIABLE &&
			(sym1->u.var.type.primary == T_ROW
				|| sym1->u.var.type.primary == T_REC) ) {
		tbl = sym1->u.var.type.u.symbol->name;
		sym2 = sym_lookup_pair(tbl, id2);

		e_tbl = expr_new(E_VARIABLE);
		e_tbl->left	= NULL;
		e_tbl->right	= NULL;
		e_tbl->constant	= 0;
		e_tbl->type	= sym1->u.var.type;
		e_tbl->symbol	= sym1;
		sym1->u.var.refcount += 1;

		if( sym1->u.var.assigned == 0 )
			Bre_Parse_Warn(BREMSG_NOT_INIT, sym1->name);

	} else if( sym1->kind == S_PARAMETER &&
			(sym1->u.par.type.primary == T_ROW
				|| sym1->u.par.type.primary == T_REC) ) {
		tbl = sym1->u.par.type.u.symbol->name;
		sym2 = sym_lookup_pair(tbl, id2);

		e_tbl = expr_new(E_VARIABLE);
		e_tbl->left	= NULL;
		e_tbl->right	= NULL;
		e_tbl->constant	= 0;
		e_tbl->type	= sym1->u.par.type;
		e_tbl->symbol	= sym1;
		sym1->u.par.refcount += 1;

	} else if( sym1->kind == S_GROUP ) {
		tbl = sym1->u.grp.using->name;
		sym2 = sym_lookup_pair(tbl, id2);

		e_tbl = expr_new(E_GROUP);
		e_tbl->left		= NULL;
		e_tbl->right		= NULL;
		e_tbl->constant		= 0;
		e_tbl->type.primary	= T_ROW;
		e_tbl->type.u.symbol	= sym1->u.grp.using;
		e_tbl->symbol		= sym1;
		sym1->u.grp.refcount += 1;

	} else {
		Bre_Parse_Error(BREMSG_ILLEGAL_CONTEXT, symkind_string(sym1->kind), id1);
	}

	if( sym2 == NULL ) {
		Bre_Parse_Error(BREMSG_NOSUCH_FIELD, id2, tbl);
	} else if( sym2->kind == S_FIELD ) {
		e_fld = expr_field(sym2);
	} else if( sym2->kind == S_LUFIELD ) {
		e_fld = expr_lufield(sym2);
	} else {
		BRE_ASSERT(0);
	}

	e = expr_new(E_DOT);
	e->left		= e_tbl;
	e->right	= e_fld;
	e->constant	= 0;
	e->type		= e->right->type;

	BRE_FREE(id1);
	BRE_FREE(id2);
	return apply_substring(e, e1);
}

/*----------------------------------------------------------------------
 * Table Field access (with optional substring spec e2)
 *	IDENTIFIER1 [...] . IDENTIFIER2
 *	IDENTIFIER1 [...] . IDENTIFIER2 [n:m]
 *
 * LANGUAGE SEMANTICS:
 *	1. IDENTIFIER1 must be a S_TABLE or a S_EXTLOOKUP.
 *	2. IDENTIFIER2 must be a proper field name.
 *	
 */
EXPR *expr_indexed_tablefield(char *id1, EXPR *e1, char *id2, EXPR *e2)
{
	EXPR *e, *e_idx, *e_fld;
	SYMBOL *sym;
	char *tbl;

	/* NOTE: calling this function will free 'id1' for us. */
	e_idx = expr_indexed_table(id1, e1);

	if( e_idx->op == E_TABLE_INDEX ) {
		tbl = e_idx->symbol->name;
		sym = sym_lookup_pair(tbl, id2);

		if( sym == NULL )
			Bre_Parse_Error(BREMSG_NOSUCH_FIELD, id2, tbl);
		else
			e_fld = expr_field(sym);

	} else if( e_idx->op == E_LOOKUP_INDEX ) {
		tbl = e_idx->symbol->name;
		sym = sym_lookup_pair(tbl, id2);

		if( sym == NULL )
			Bre_Parse_Error(BREMSG_NOSUCH_FIELD, id2, tbl);
		else
			e_fld = expr_lufield(sym);

	} else {
		BRE_ASSERT(0);
	}

	e = expr_new(E_DOT);
	e->left		= e_idx;
	e->right	= e_fld;
	e->constant	= 0;
	e->type		= e->right->type;

	BRE_FREE(id2);
	return apply_substring(e, e2);
}

/*----------------------------------------------------------------------
 * Create the variables used in FOR ALL, FOR SOME and COMPUTE
 * loops.
 *
 * LANGUAGE SEMANTICS:
 *	1. Each variable name must not already be in use.
 *	2. If a using_clause is active, the variables cannot
 *	   clash with field names.
 *
 */
EXPR *expr_for_idlist(ID_LIST *idlist)
{
	char *id;
	EXPR *e, *head, *previous;
	ID_LIST *curr;
	SYMBOL *sym;
	SYM_TYPE type;

	/*
	 * Check that the names in 'idlist' are not being used.
	 */
	for(curr=idlist; curr; curr=curr->next) {
		id = curr->id;

		sym = sym_lookup(id);
		if( sym ) {
			Bre_Parse_Error(BREMSG_NAME_CLASH, id, symkind_string(sym->kind));
		}

		if( pc.using_table )
			sym = sym_lookup_pair(pc.using_table->name, id);
		else
			sym = NULL;
		if( sym ) {
			Bre_Parse_Error(BREMSG_CLASH_FIELDTABLE, id, pc.using_table->name);
		}
	}

	/*
	 * Go through the list and add the new variables.
	 */
	head = NULL;
	for(curr=idlist; curr; curr=curr->next) {
		id = curr->id;
		sym = sym_lookup(id);
		if( sym ) {
			Bre_Parse_Error(BREMSG_DUPLICATE_VAR, id);
		}

		type.primary = T_UNKNOWN;
		sym = declare_variable(id, type);

		e = expr_new(E_NONE);
		e->left		= NULL;
		e->right	= NULL;
		e->symbol	= sym;
		e->constant	= 0;
		e->type.primary	= T_UNKNOWN;

		sym->u.var.assigned = 1;

		if( head == NULL )
			head = e;
		else
			previous->right = e;

		previous = e;
		
	}
	free_idlist(idlist);

	return head;
}

/*----------------------------------------------------------------------
 * Validate string iterator expression.
 *	- Can only be one variable.
 *	- The variable will be assigned a string type.
 */
static EXPR *expr_string_iterator(EXPR_OPERATOR type, EXPR *vars, int var_count, EXPR *e1)
{
	EXPR *e;
	SYMBOL *sym;

	if( var_count != 1 ) {
		Bre_Parse_Warn(BREMSG_BAD_STRITERATE, operator_string(type));
	}

	sym = vars->symbol;
	sym->u.var.type.primary = T_STRING;

	e = expr_new(E_NONE);
	e->left		= vars;
	e->right	= e1;
	e->constant	= 0;
	e->type.primary	= T_STRING;

	return e;
}

/*----------------------------------------------------------------------
 * Process a list iterator expression.
 *	- Multiple variables are allowed
 *	- When multiple variables are used and an explicit list
 *	  is given, then the iterator list must have correct number of elements
 *	  and consistant types.
 *
 * 12/29/97
 *	Removed restriction on lists when we don't know the number of elements.
 */
static EXPR *expr_list_iterator(EXPR_OPERATOR type, EXPR *vars, int var_count, EXPR *e1)
{
	EXPR *e, *i, *curr, *v;
	SYMBOL *sym;
	int n;
	char num1[30];

	if( e1->op != E_LIST ) {
#if 0
		if( var_count > 1 )
			Bre_Parse_Warn(BREMSG_LSIZE_UNKNOWN, operator_string(type) );
#endif
		for(v=vars; v; v=v->right) {
			v->symbol->u.var.type.primary = e1->type.u.subtype;
		}
	} else if( e1->op == E_LIST ) {
		if( e1->value.i.val % var_count != 0 ) {
			Bre_Parse_Warn(BREMSG_WRONG_NVARS, operator_string(type) );
		}

		n = 0;
		v = vars;
		for(curr=e1->right; curr; curr=curr->right) {
			sym = v->symbol;
			i = curr->left;

			if( sym->u.var.type.primary == T_UNKNOWN ) {
				sym->u.var.type = i->type;
			} else if( sym->u.var.type.primary != i->type.primary ) {
				sprintf(num1, "%d", n+1);
				Bre_Parse_Warn(BREMSG_VARTYPE_NOTMATCH,
						operator_string(type), sym->name, num1);
			}

			v = v->right;
			if( v == NULL )
				v = vars;
			n += 1;
		}
	} else {
		BRE_ASSERT(0);
	}

	e = expr_new(E_NONE);
	e->left		= vars;
	e->right	= e1;
	e->constant	= 0;
	e->type		= e1->type;

	return e;
}

/*----------------------------------------------------------------------
 * Process a table iterator expression.
 *	- The table name must not be indexed.
 *
 */
static EXPR *expr_table_iterator(EXPR_OPERATOR type, EXPR *vars, int var_count, EXPR *e1)
{
	EXPR *e, *v;
	SYMBOL *sym;

	if( e1->op != E_TABLE_INDEX && e1->op != E_GROUP ) {
		Bre_Parse_Warn(BREMSG_NOTTABLE_EXPR, operator_string(type));
	}

	if( e1->op == E_TABLE_INDEX && var_count != 1 ) {
		Bre_Parse_Warn(BREMSG_ILLEGAL_MULTIVAR, operator_string(type));
	} else if( e1->op == E_GROUP && var_count != 2 ) {
		Bre_Parse_Warn(BREMSG_GROUP_VARS, operator_string(type));
	}

	if( e1->right != NULL ) {
		Bre_Parse_Warn(BREMSG_INV_TABLEEXPR, operator_string(type));
	}

	/* set the type of the variable(s) */
	for(v=vars; v; v=v->right) {
		sym = v->symbol;
		sym->u.var.type = e1->type;
	}

	e = expr_new(E_NONE);
	e->left		= vars;
	e->right	= e1;
	e->constant	= 0;
	e->type		= e1->type;

	return e;
}

/*----------------------------------------------------------------------
 * Iterator expressions can only be of the form:
 *
 *	{ "a", "list", "of", "stuff" }
 *		- An expression of type T_LIST.
 *
 *	TABLENAME
 *		- An expression of type T_ROW.
 *		- Expression node of E_TABLE_INDEX, with left/right branches
 *		set to NULL.
 *
 *	"STRING EXPRESSION"
 *		- An expression of type T_STRING.
 *
 */
EXPR *expr_iterator(EXPR_OPERATOR type, EXPR *vars, EXPR *e1)
{
	EXPR *e, *curr;
	int var_count;

	/*
	 * Count the number of variables.
	 */
	var_count = 0;
	for(curr=vars; curr; curr=curr->right)
		var_count += 1;

	if( e1->type.primary == T_STRING ) {
		e = expr_string_iterator(type, vars, var_count, e1);

	} else if( e1->type.primary == T_LIST ) {
		e = expr_list_iterator(type, vars, var_count, e1);

	} else if( e1->type.primary == T_ROW ) {
		e = expr_table_iterator(type, vars, var_count, e1);

	} else {
		Bre_Parse_Error(BREMSG_BAD_ITERATOR,
				tflags_string(e1->type.primary), operator_string(type));
	}

	return e;
}

/*----------------------------------------------------------------------
 * FOR ITERATION: FOR ALL, FOR SOME
 *	FOR ALL idlist IN e1 e2
 *	FOR SOME idlist IN e1 e2
 *
 * LANGUAGE SEMANTICS:
 *	1. The identifiers in idlist must not already
 *	   be in use. (the same rules apply as to variable names)
 *	   
 *	2. The identifiers remain in existence until the end of the rule
 *	   or function.
 *
 *	3. The expression 'e2' must evaluate to a boolean type.
 *
 *	4. Multiple identifiers can only be used when 'e1' is a list
 *	   type.
 *
 *	5. The number of elements in the list must be evenly
 *		   divisible by the number of identifiers.
 *
 *	6. The data types of every nth element in the list must
 *	   be the same. For n upto the number of identifiers.
 *
 *	7. 'e1' must be list type, string, or table row.
 *
 */
EXPR *expr_for(EXPR_OPERATOR type, EXPR *iterator, EXPR *e2)
{
	EXPR *e;

	if( e2->type.primary != T_BOOLEAN ) {
		Bre_Parse_Warn(BREMSG_BOOLEAN_REQUIRED, operator_string(type));
	}

	e = expr_new(type);
	e->left		= iterator;
	e->right	= e2;
	e->constant	= 0;
	e->type.primary	= T_BOOLEAN;

	return e;
}

/*----------------------------------------------------------------------
 * COMPUTE CONSTRUCT:
 *	FOR idlist IN e2 COMPUTE e1 WHERE e3
 *	FOR idlist IN e2 COMPUTE e1
 *
 * LANGUAGE SEMANTICS:
 *	1. e1 must be a group expression.
 *	2. e3 is optional.
 *	3. When e3 is specified it must evaluate to a boolean type
 *	4. Same rules for 'idlist' and 'e2' apply as in the FOR ALL and FOR SOME
 *	   loops.
 *	5. The group function COUNT cannot have an argument when used in this context.
 */
EXPR *expr_compute(EXPR *e1, EXPR *iterator, EXPR *e3)
{
	EXPR *e, *where_expr, *loop_expr;
	SYM_TYPE type;

	if( e3 ) {
		where_expr = e3;
	} else {
		where_expr = expr_boolean(1);
	}

	loop_expr = expr_for(E_COMPUTE, iterator, where_expr);
	loop_expr->op = E_NONE;

	if( e1->op == E_COUNT && e1->right != NULL ) {
		Bre_Parse_Warn(BREMSG_BADARGS_COUNT);
	}

	if( iterator->right->op == E_GROUP ) {
		type.primary	= T_LIST;
		type.u.subtype	= e1->type.primary;
	} else {
		type = e1->type;
	}

	e = expr_new(E_COMPUTE);
	e->left		= e1;
	e->right	= loop_expr;
	e->constant	= 0;
	e->type		= type;

	return e;
}

/*----------------------------------------------------------------------
 * IDENTIFIER LIST:
 *	id, id, id, ...
 *	
 */
ID_LIST *idlist(ID_LIST *idlist, char *id)
{
	ID_LIST *curr, *p, *prev;

	p = (ID_LIST*)BRE_MALLOC( sizeof(ID_LIST) );
	if( p == NULL ) {
		Bre_Error(BREMSG_DIE_NOMEM);
	}
	p->id = id;
	p->next = NULL;

	prev = NULL;
	for(curr=idlist; curr; curr=curr->next)
		prev = curr;

	if( prev == NULL ) {
		return p;
	} else {
		prev->next = p;
		return idlist;
	}
}

/***********************************************************************
 * Free all the memory associated with an
 * ID_LIST.
 *
 */
static void free_idlist(ID_LIST *head)
{
	ID_LIST *curr, *nxt;

	curr=head;
	while(curr) {
		nxt = curr->next;
		BRE_FREE(curr->id);
		BRE_FREE(curr);
		curr= nxt;
	}
}

/*----------------------------------------------------------------------
 * basic type: integer, float, string, boolean, date, timestamp
 *	
 */
SYM_TYPE basic_type(int token)
{
	SYM_TYPE t;

	switch(token) {
	case KW_BOOLEAN:
		t.primary = T_BOOLEAN;
		break;

	case KW_INTEGER:
		t.primary = T_INTEGER;
		break;

	case KW_FLOAT:
		t.primary = T_FLOAT;
		break;

	case KW_STRING:
		t.primary = T_STRING;
		break;

	default:
		BRE_ASSERT(0);
	}

	return t;
}

/*----------------------------------------------------------------------
 * basic type: {integer}, {float}, {string}, {boolean}, {date},
 *	{timestamp}
 *	
 */
SYM_TYPE list_type(int token)
{
	SYM_TYPE t;

	switch(token) {
	case KW_BOOLEAN:
		t.primary = T_LIST;
		t.u.subtype = T_BOOLEAN;
		break;

	case KW_INTEGER:
		t.primary = T_LIST;
		t.u.subtype = T_INTEGER;
		break;

	case KW_FLOAT:
		t.primary = T_LIST;
		t.u.subtype = T_FLOAT;
		break;

	case KW_STRING:
		t.primary = T_LIST;
		t.u.subtype = T_STRING;
		break;

	default:
		BRE_ASSERT(0);
	}

	return t;
}

/*----------------------------------------------------------------------
 * complex type: IDENTIFIER
 *
 * LANGUAGE SEMANTICS:
 *	1. The identifier must be the name of an external lookup or
 *	   table.
 */
SYM_TYPE complex_type(char *id)
{
	SYMBOL *sym;
	SYM_TYPE t;

	sym = sym_lookup(id);
	if( sym == NULL ) {
		Bre_Parse_Warn(BREMSG_UNKNOWN_TABLE, id);
		t.primary = T_UNKNOWN;
	} else if( sym->kind == S_EXTLOOKUP ) {
		t.primary = T_REC;
		t.u.symbol = sym;
	} else if( sym->kind == S_TABLE ) {
		t.primary = T_ROW;
		t.u.symbol = sym;
	} else {
		Bre_Parse_Warn(BREMSG_BAD_TYPE, symkind_string(sym->kind), id);
		t.primary = T_UNKNOWN;
	}

	BRE_FREE(id);
	return t;
}

/*----------------------------------------------------------------------
 * complex type: {IDENTIFIER}
 *
 * LANGUAGE SEMANTICS:
 *	1. It is not allowed to declare a list of T_ROW or T_REC.
 */
SYM_TYPE complex_list_type(char *id)
{
	SYMBOL *sym;
	SYM_TYPE t;

	sym = sym_lookup(id);
	if( sym == NULL ) {
		Bre_Parse_Warn(BREMSG_BAD_DECL, "unknown", id);
	} else {
		Bre_Parse_Warn(BREMSG_BAD_DECL, symkind_string(sym->kind), id);
	}

	t.primary = T_UNKNOWN;

	BRE_FREE(id);
	return t;
}

/***********************************************************************
 * If the parameter already has a type, then
 * give an error. Otherwise set the
 * type of this parameter.
 *
 */
static void declare_parameter(SYMBOL *sym, SYM_TYPE type)
{
	if( sym->u.par.type.primary != T_UNKNOWN ) {
		Bre_Parse_Warn(BREMSG_BAD_REDECL, sym->name);
	} else {
		sym->u.par.type = type;
	}
}

/***********************************************************************
 * Create a new variable called 'id' with the type 'type'
 * Assign the variable a new offset value.
 * Add this symbol to the SYM_LIST for the function or
 * rule that is declaring it.
 *
 */
static SYMBOL *declare_variable(char *id, SYM_TYPE type)
{
	SYMBOL *sym;
	int offset;

	if( pc.func )
		offset = pc.func->u.fun.nlocals++;
	else if( pc.rule )
		offset = pc.rule->u.rul.nlocals++;
	else
		Bre_Parse_Error(BREMSG_VAR_CONTEXT, id);

	sym = sym_insert(id, S_VARIABLE);

	sym->u.var.assigned	= 0;
	sym->u.var.refcount	= 0;
	sym->u.var.type		= type;
	sym->u.var.fp		= offset;

	if( pc.func )
		symlist_add(&pc.func->u.fun.vars, sym);
	else if( pc.rule )
		symlist_add(&pc.rule->u.rul.vars, sym);
	else
		Bre_Parse_Error(BREMSG_VAR_CONTEXT, id);

	return sym;
}

/***********************************************************************
 * This function handles a declaration in the body of
 * a rule or function of the form:
 *
 *	x, y, z: INTEGER;
 *
 *	(x, y, and z may be a parameter or new variable)
 *
 * LANGUAGE SYMANTICS:
 *	1. Variable names cannot conflict with field names from the
 *	   using clause.
 *	2. A variable name must be unique to this rule.
 *	3. Variable declarations must occur first in a rule or function.
 *
 */
void declare_type(ID_LIST *idlist, SYM_TYPE type)
{
	ID_LIST *curr;
	SYMBOL *sym;

	for(curr=idlist; curr; curr=curr->next) {
		sym = sym_lookup(curr->id);

		if( sym && sym->kind == S_PARAMETER ) {
			declare_parameter(sym, type);

		} else if( sym ) {
			Bre_Parse_Warn(BREMSG_NAME_CLASH, curr->id, symkind_string(sym->kind));

		} else {
			if( pc.using_table ) {
				sym = sym_lookup_pair(pc.using_table->name, curr->id);
				if( sym ) {
					Bre_Parse_Warn(BREMSG_CLASH_FIELD, curr->id);
				}
			}

			if( sym == NULL )
				(void)declare_variable(curr->id, type);
		}
	}

	if( pc.last_stmt != STMT_UNKNOWN ) {
		Bre_Parse_Warn(BREMSG_UNEXPECTED_DECL);
	}

	free_idlist(idlist);
}

/*----------------------------------------------------------------------
 * Convert the integer value 'l' into a string for
 * when a rule id is entered by the user as a number.
 *
 */
char *make_ruleid(long l)
{
	char buf[ 100 ];

	sprintf(buf, "%ld", l);
	return BRE_STRDUP(buf);
}

static void insert_current_row_parameter(SYMBOL *using_table)
{
	SYMBOL *sym;

	sym = sym_lookup("current_row");
	if( sym != NULL ) {
		Bre_Parse_Warn(BREMSG_REDEF_CURROW);
	} else {
		sym = sym_insert("current_row", S_PARAMETER);
		sym->u.par.refcount		= 0;
		sym->u.par.type.primary		= T_ROW;
		sym->u.par.type.u.symbol	= using_table;
		sym->u.par.fp			= 0;
	}
}

static void remove_current_row_parameter(void)
{
	SYMBOL *sym;

	sym = sym_lookup("current_row");
	BRE_ASSERT( sym != NULL );
	sym_remove(sym);
}

/*----------------------------------------------------------------------
 * Create a new rule.
 *
 * LANGUAGE SEMANTICS:
 *	1. The rule id must be unique.
 *	2. The table name in the using clause must refer to
 *	   a real table.
 *	3. If the 'using clause' exists, add a table row pointer variable
 *	   called 'current_row'
 *
 */
void declare_rule(char *id, char *using_clause)
{
	SYMBOL *sym, *using;
	char prefix[ 200 ];

	sprintf(prefix, "%d", BRE()->file_count);

	sym = sym_lookup_pair(prefix, id);
	if( sym ) {
		Bre_Parse_Error(BREMSG_RULE_CLASH, id, symkind_string(sym->kind));
	}

	if( using_clause ) {
		using = sym_lookup(using_clause);
		if( using == NULL ) {
			Bre_Parse_Error(BREMSG_UNKNOWN_TABLENAME, using_clause);
		} else if( using->kind != S_TABLE ) {
			Bre_Parse_Error(BREMSG_TABLE_EXPECTED,
						symkind_string(using->kind), using_clause);
		}
	} else
		using = NULL;

	sym = sym_insert_pair(prefix, id, S_RULE);

	sym->u.rul.entry_point	= 0;
	sym->u.rul.nlocals	= 0;
	sym->u.rul.vars		= NULL;
	sym->u.rul.using	= using;

	/* configure the parsing context for a new rule */
	reset_parse_context();
	pc.func			= NULL;
	pc.rule			= sym;
	pc.using_table		= using;
	pc.last_stmt		= STMT_UNKNOWN;
	pc.stmt_count		= 0;

	sym->u.rul.entry_point = gencode_rule_entry(BRE()->prog);

	BRE_FREE(id);

	if( using_clause ) {
		insert_current_row_parameter(using);
		BRE_FREE(using_clause);
	}
}

/*----------------------------------------------------------------------
 * Create a new function whose name is 'id'.
 *
 * LANGUAGE SEMANTICS:
 *	1. The function name must be already used.
 *	2. The parameter names must be unique.
 *	3. Paramater names must not clash with a
 *	   previosuly declared object.
 *
 */
void declare_func(char *id, ID_LIST *parameters)
{
	SYMBOL *sym, *parm;
	ID_LIST *curr;

	sym = sym_lookup(id);
	if( sym ) {
		Bre_Parse_Error(BREMSG_FUNC_CLASH, id, symkind_string(sym->kind));
	}

	sym = sym_insert(id, S_FUNCTION);

	sym->u.fun.entry_point		= 0;
	sym->u.fun.nlocals		= 0;
	sym->u.fun.nargs		= 0;
	sym->u.fun.parms		= NULL;
	sym->u.fun.vars			= NULL;
	sym->u.fun.refcount		= 0;

	/* configure the parsing context for a new function */
	reset_parse_context();
	pc.func			= sym;
	pc.rule			= NULL;
	pc.using_table		= NULL;
	pc.last_stmt		= STMT_UNKNOWN;
	pc.stmt_count		= 0;

	for(curr=parameters; curr; curr=curr->next) {
		parm = sym_lookup(curr->id);
		if( parm && parm->kind == S_PARAMETER ) {
			Bre_Parse_Error(BREMSG_DUP_PARAM, curr->id);
		} else if( parm ) {
			Bre_Parse_Error(BREMSG_PARM_CLASH, curr->id, symkind_string(parm->kind));
		}

		parm = sym_insert(curr->id, S_PARAMETER);

		parm->u.par.refcount		= 0;
		parm->u.par.type.primary	= T_UNKNOWN;
		parm->u.par.fp			= sym->u.fun.nargs++;

		symlist_add(&sym->u.fun.parms, parm);
	}

	sym->u.fun.entry_point = gencode_function_entry(BRE()->prog);

	free_idlist(parameters);
	BRE_FREE(id);
}

/*----------------------------------------------------------------------
 * CONSTANT IDENTIFIER IS e1
 *
 * LANGUAGE SEMANTICS:
 *	1. The name of the constant must be unique.
 *	2. The expression 'e1' must be constant.
 *
 */
void declare_constant(char *id, EXPR *e1)
{
	SYMBOL *sym, *parm;
	int lc;

	sym = sym_lookup(id);
	if( sym ) {
		Bre_Parse_Error(BREMSG_CONST_CLASH, id, symkind_string(sym->kind));
	}

	if( e1->constant == 0 ) {
		Bre_Parse_Error(BREMSG_NON_CONSTANT, id);
	}

	lc = gencode_function_entry(BRE()->prog);

	sym = sym_insert(id, S_CONSTANT);

	sym->u.con.refcount	= 0;
	sym->u.con.type		= e1->type;
	sym->u.con.entry_point	= lc;

	if( bre_error_count() == 0 )
		gencode(G_GENCODE, BRE()->prog, e1);

	gencode_function_exit(BRE()->prog, lc, 0, 0);

	BRE_FREE(id);
}

/*----------------------------------------------------------------------
 * Finish off the function.
 * Generate the RETURN instruction. Remove
 * parameters and variables from the symbol table.
 * Make sure the type of the function is okay.
 *
 * LANGUAGE SEMANTICS:
 *	1. A function can only return assignable types.
 *	2. The type of the last statement is the return type
 *	   for the function.
 *	3. A function must consists of at least 1 statement.
 *
 */
void terminate_func(void)
{
	SYMBOL_LIST *curr;

	if( pc.last_stmt == STMT_UNKNOWN ) {
		Bre_Parse_Warn(BREMSG_EMPTY_FUNC, pc.func->name);
	}

	/*
	 * Detatch the function parameters from the symbol table.
	 * (We still want to keep them attatched to the function
	 * symbol, to be used for function proto-typing.
	 */
	symlist_detach_all(pc.func->u.fun.parms);

	/*
	 * Remove the list of variables from this
	 * function, as well as from the symbol table.
	 */
	symlist_remove_all(&pc.func->u.fun.vars);

	/*
	 * Verify that the return type for the function is
	 * valid, and update the symbol table entry with the correct
	 * type.
	 */
	if( !is_assignable_type(pc.last_type) ) {
		Bre_Parse_Warn(BREMSG_INV_RETURN, pc.func->name);
		pc.func->u.fun.ret_type.primary = T_UNKNOWN;
	} else {
		pc.func->u.fun.ret_type = pc.last_type;
	}

	gencode_function_exit(BRE()->prog, pc.func->u.fun.entry_point,
					pc.func->u.fun.nlocals, pc.func->u.fun.nargs);

	reset_parse_context();
}

/*----------------------------------------------------------------------
 * Finish off the rule.
 * Generate the END instruction. Remove
 * variables from the symbol table.
 *
 * LANGUAGE SEMANTICS:
 *	1. A rule must evaluate to a BOOLEAN type.
 *	2. The last statement must return BOOLEAN.
 *	3. A rule must have at least 1 statement.
 *	4. 
 *
 */
void terminate_rule(EXPR *error_clause)
{
	char prefix[ 200 ], name[ 200 ];

	if( pc.last_stmt == STMT_UNKNOWN ) {
		sym_extract_pair(pc.rule->name, prefix, name);
		Bre_Parse_Warn(BREMSG_EMPTY_RULE, name);
	} else if( pc.last_type.primary != T_BOOLEAN ) {
		sym_extract_pair(pc.rule->name, prefix, name);
		Bre_Parse_Warn(BREMSG_RULE_BOOLEAN, name);
	}

	if( error_clause && error_clause->op == E_WARN ) {
		pc.rule->u.rul.diag = BRE_WARN;
	} else if( error_clause && error_clause->op == E_FAIL ) {
		pc.rule->u.rul.diag = BRE_FAIL;
	} else {
		pc.rule->u.rul.diag = BRE_FAIL;
	}

	/*
	 * Remove the list of variables from this
	 * rule, as well as from the symbol table.
	 */
	symlist_remove_all(&pc.rule->u.rul.vars);

	if( bre_error_count() == 0 )
		gencode_rule_exit(BRE()->prog, pc.rule, error_clause);

	if( pc.using_table ) {
		remove_current_row_parameter();
	}

	reset_parse_context();
}

/*----------------------------------------------------------------------
 * Verify the proto-type for an external function
 * matches the dynamically linked symbol.
 * RETURNS:
 *	0 - external function arguments are invalid.
 *	1 - external function arguments are okay.
 *
 */
static int valid_extfunc_args(BRE_EXTERNAL *extp, ID_LIST *parameters)
{
	ID_LIST *curr1, *curr2;
	char *parm1, *parm2;
	int count;
	int nargs_in, nargs_out;
	char num1[30];

	nargs_in = strlen(extp->args_in);
	nargs_out = strlen(extp->args_out);

	if( nargs_out > 1 ) {
		sprintf(num1, "%d", nargs_out);
		Bre_Parse_Warn(BREMSG_DYNAMIC_LOOKUP, extp->name, nargs_out);
		return 0;
	}

	/*
	 * Count parameters and make sure names are unique.
	 */
	count = 0;
	for(curr1=parameters; curr1; curr1=curr1->next) {
		parm1 = curr1->id;
		for(curr2=parameters; curr2; curr2=curr2->next) {
			parm2 = curr2->id;
			if( curr2 == curr1 )
				continue;
			if( symbol_match(parm1, parm2) ) {
				Bre_Parse_Warn(BREMSG_FUNCPARM_UNIQUE, parm1);
				return 0;
			}
		}
		count += 1;
	}

	if( count > nargs_in ) {
		sprintf(num1, "%d", nargs_in);
		Bre_Parse_Warn(BREMSG_EXTFUNC_MANYARGS, extp->name, nargs_in);
		return 0;
	} else if( count < nargs_in ) {
		sprintf(num1, "%d", nargs_in);
		Bre_Parse_Warn(BREMSG_EXTFUNC_FEWARGS, extp->name, nargs_in);
		return 0;
	}

	return 1;
}


/*----------------------------------------------------------------------
 * An external function is being declared.
 *
 * LANGUAGE SEMANTICS:
 *	1. The name of this function must not already be used by another object.
 *	2. Give a warning if the name is not found in the
 *	   internal list of external objects. (BRE()->externals).
 *
 *	3. If the name is found, check that the number of input parameters
 *	   match.
 *
 *	4. Check that the parameters are unique. (There is no reason for this
 *	   except to be consistent)
 *
 */
void declare_extfunc(char *id, ID_LIST *parameters)
{
	BRE_EXTERNAL *extp;
	SYMBOL *sym, *parm;
	ID_LIST *curr;

	sym = sym_lookup(id);
	if( sym ) {
		Bre_Parse_Warn(BREMSG_EXTFUNC_CLASH, id, symkind_string(sym->kind));
	} else if( extp = sym_find_external(id) ) {
		if( valid_extfunc_args(extp, parameters) ) {
			sym = sym_insert(id, S_EXTFUNC);
			sym->u.ext.external_cookie = extp->external_cookie;
			sym->u.ext.refcount	= 0;
			sym->u.ext.args_in	= BRE_STRDUP(extp->args_in);
			sym->u.ext.args_out	= BRE_STRDUP(extp->args_out);
			sym->u.ext.func		= extp->func;
		}
	} else {
		Bre_Parse_Warn(BREMSG_SYM_NOTLINKED, id);
	}


	BRE_FREE(id);
	free_idlist(parameters);
}

/*----------------------------------------------------------------------
 * Verify that the number of keys matches the
 * number of input arguments to the dynamically linked
 * symbol 'extp'.
 * 
 * Make sure the key names are unique.
 *
 * RETURNS: 0 - bad keys, 1 - keys are okay.
 *
 */
static int valid_lookup_keys(BRE_EXTERNAL *extp, ID_LIST *keys)
{
	ID_LIST *curr1, *curr2;
	int count;
	int nargs_in;
	char num1[30];

	nargs_in = strlen(extp->args_in);

	/*
	 * Count parameters and make sure names are unique.
	 */
	count = 0;
	for(curr1=keys; curr1; curr1=curr1->next) {
		for(curr2=keys; curr2; curr2=curr2->next) {
			if( curr2 == curr1 )
				continue;
			if( symbol_match(curr1->id, curr2->id) ) {
				Bre_Parse_Warn(BREMSG_KEY_NOTUNIQUE, curr1->id);
				return 0;
			}
		}
		count += 1;
	}

	if( count > nargs_in ) {
		sprintf(num1, "%d", nargs_in);
		Bre_Parse_Warn(BREMSG_MANYKEYS_LINKED, extp->name, num1);
		return 0;
	} else if( count < nargs_in ) {
		sprintf(num1, "%d", nargs_in);
		Bre_Parse_Warn(BREMSG_FEWKEYS_LINKED, extp->name, num1);
		return 0;
	}

	return 1;
}

/*----------------------------------------------------------------------
 * Verify that the number of fields match the
 * number of output arguments in 'extp'
 *
 * Verify that the field names are unique.
 *
 * Insert the fields into the symbol table (as SYM_LUFIELD).
 *
 */
static int valid_lookup_fields(BRE_EXTERNAL *extp, ID_LIST *fields)
{
	ID_LIST *curr1, *curr2;
	int count, i;
	int nargs_out;
	char *p;
	SYMBOL *sym;
	char num1[30];

	nargs_out = strlen(extp->args_out);

	/*
	 * Count parameters and make sure names are unique.
	 */
	count = 0;
	for(curr1=fields; curr1; curr1=curr1->next) {
		for(curr2=fields; curr2; curr2=curr2->next) {
			if( curr2 == curr1 )
				continue;
			if( symbol_match(curr1->id, curr2->id) ) {
				Bre_Parse_Warn(BREMSG_LOOKUPFIELD_UNIQUE, curr1->id);
				return 0;
			}
		}
		count += 1;
	}

	if( count > nargs_out ) {
		sprintf(num1, "%d", nargs_out);
		Bre_Parse_Warn(BREMSG_MANYFIELDS_LINKED, extp->name, num1);
		return 0;
	} else if( count < nargs_out ) {
		sprintf(num1, "%d", nargs_out);
		Bre_Parse_Warn(BREMSG_FEWFIELDS_LINKED, extp->name, num1);
		return 0;
	}

	/*
	 * Insert the lookup fields
	 */
	i = 0;
	p = extp->args_out;
	for(curr1=fields; curr1; curr1=curr1->next) {
		sym = sym_insert_pair(extp->name, curr1->id, S_LUFIELD);
		sym->u.luf.refcount	= 0;
		sym->u.luf.offset	= i;
		sym->u.luf.type		= type_from_char(*p);
		i += 1;
		p++;
	}

	return 1;
}

/*----------------------------------------------------------------------
 * An external lookup being declared.
 *
 * Insert the field names into the symbol table, and associate them
 * with their corresponding external lookup.
 *
 * LANGUAGE SEMANTICS:
 *	1. Give a warning if the name was not found in the internal
 *	   list of external objects.
 *
 *	2. If the name is found, check that the number of key arguments
 *	   equals the number of input arguments to the lookup callback.
 *
 *	3. Check that the number of fields equals the number of output
 *	   arguments to the lookup callback.
 *
 *	4. Make sure field names are unique.
 *	5. Make sure key fields are unique.
 *
 */
void declare_lookup(char *id, ID_LIST *keys, ID_LIST *fields)
{
	BRE_EXTERNAL *extp;
	SYMBOL *sym, *parm;

	sym = sym_lookup(id);
	if( sym ) {
		Bre_Parse_Warn(BREMSG_EXTLOOKUP_CLASH, id, symkind_string(sym->kind));

	} else if( extp = sym_find_external(id) ) {
		if( valid_lookup_keys(extp, keys)
					&& valid_lookup_fields(extp, fields) ) {

			sym = sym_insert(id, S_EXTLOOKUP);
			sym->u.ext.external_cookie = extp->external_cookie;
			sym->u.ext.refcount	= 0;
			sym->u.ext.args_in	= BRE_STRDUP(extp->args_in);
			sym->u.ext.args_out	= BRE_STRDUP(extp->args_out);
			sym->u.ext.func		= extp->func;
		}
	} else {
		Bre_Parse_Warn(BREMSG_SYM_NOTLINKED, id);
	}

	BRE_FREE(id);
	free_idlist(keys);
	free_idlist(fields);
}

/*----------------------------------------------------------------------
 * WARN WITH: arg, arg, arg, ...
 * FAIL WITH: arg, arg, arg, ...
 *
 * LANGUAGE SEMANTICS:
 *	1. The first argument is mandatory.
 *	2. All arguments must be integer, string, or boolean.
 *
 */
EXPR *expr_error(EXPR_OPERATOR type, EXPR *args)
{
	EXPR *expr, *curr, *arg;
	int i;
	char num1[30];

	expr = expr_new(type);

	expr->left	= NULL;
	expr->right	= args;

	i = 0;
	for(curr=args; curr; curr=curr->right) {
		arg = curr->left;
		if( !is_basic_type(arg->type.primary) ) {
			sprintf(num1, "%d", i+1);
			Bre_Parse_Warn(BREMSG_ERROR_ARGS, num1, operator_string(type));
		}

		i += 1;
	}

	return expr;
}

/*----------------------------------------------------------------------
 * Assignment statments.
 *
 * IDENTIFIER := e1
 *
 * LANGUAGE SEMANTICS:
 *	1. The type must match the type of the identifier.
 *	2. This statement does not have a type.
 *	3. Assignments are only allowed to variables.
 *	4. Assignments using null are allowed.
 */
EXPR *expr_assign(char *id, EXPR *e1)
{
	EXPR *e;
	SYMBOL *sym;

	sym = sym_lookup(id);

	e = expr_new(E_ASSIGN);
	e->left		= NULL;
	e->right	= e1;
	e->constant	= 0;
	e->type.primary	= T_UNKNOWN;
	e->symbol	= sym;

	if( sym == NULL ) {
		if( pc.using_table )
			sym = sym_lookup_pair(pc.using_table->name, id);
		else
			sym = NULL;

		if( sym )
			Bre_Parse_Warn(BREMSG_ILLEGAL_ASSIGNFIELD, id);
		else
			Bre_Parse_Warn(BREMSG_NOSUCH_VAR, id);

	} else if( sym->kind != S_VARIABLE ) {
		Bre_Parse_Warn(BREMSG_ILLEGAL_ASSIGN, symkind_string(sym->kind), id);
	} else if( e1->op != E_NULL
				&& !compatible_types(sym->u.var.type, e1->type) ) {
		Bre_Parse_Warn(BREMSG_ASSIGN_TYPES, tflags_string(sym->u.var.type.primary),
					tflags_string(e1->type.primary) );
	} else {
		if( e1->op == E_NULL )
			e1->type = sym->u.var.type;	/* assign NULL node a type */
		sym->u.var.assigned = 1;
	}

	BRE_FREE(id);

	return e;
}

/*----------------------------------------------------------------------
 * Process a Statement from the statement list.
 *
 * A rule or function consists of a series of statements.
 * A statement can be an expression, assignment or variable declaration.
 * (also the null statement).
 *
 * RULE IDENTIFIER using_clause IS	statement_list
 * FUNCTION IDENTIFIER parameters IS	statement_list
 *
 * This function processes each statement in the order in which
 * it is encountered.
 *
 * If errors have been reported, don't try and generate any
 * byte-code.
 *
 * LANGAUAGE SEMANTICS:
 *	1. For functions, All parameters must have been assigned a type.
 *	2. It is illegal for a statement to evaulate to a T_MIXED list.
 *
 */
void statement(EXPR *stat)
{
	SYMBOL_LIST *curr;
	SYMBOL *sym;

	if( stat == NULL )
		return;

	if( stat->type.primary == T_LIST && stat->type.u.subtype == T_MIXED ) {
		Bre_Parse_Warn(BREMSG_MIXED_LIST);
	}

	pc.stmt_count += 1;

	/*
	 * If this is the first "real" statement of the
	 * function, check to make sure all parameters have been
	 * given types.
	 */
	if( pc.func && pc.last_stmt == STMT_UNKNOWN ) {
		for(curr=pc.func->u.fun.parms; curr; curr=curr->next) {
			sym = curr->sym;
			if( sym->u.par.type.primary == T_UNKNOWN ) {
				Bre_Parse_Warn(BREMSG_PARM_NOTDECL, sym->name);
			}
		}
	}

	/*
	 * POP the value from the previous statement.
	 */
	if( pc.last_stmt == STMT_EXPR )
		gencode_pop(G_GENCODE, BRE()->prog);

	pc.last_stmt = STMT_EXPR;
	pc.last_type = stat->type;

	if( bre_error_count() == 0 )
		gencode(G_GENCODE, BRE()->prog, stat);
}

/*----------------------------------------------------------------------
 * BEGIN ...statements... END
 *
 * LANGUAGE SEMANTICS:
 *
 */
EXPR *expr_block(EXPR *stmt_list)
{
	EXPR *e;
	EXPR *curr, *last;

	e = expr_new(E_BLOCK);

	e->left		= NULL;
	e->right	= stmt_list;
	e->constant	= 0;

	last = NULL;
	for(curr = stmt_list; curr; curr=curr->right)
		last = curr;

	if( last == NULL ) {
		e->type.primary = T_UNKNOWN;
	} else {
		e->type.primary = last->left->type.primary;
	}

	return e;
}

/*----------------------------------------------------------------------
 * GROUP id USING using IS expr
 *
 * LANGUAGE SEMANTICS:
 *	1. The using clause is mandatory.
 *	2. The table name in the using clause must refer to
 *	   a real table.
 *	3. The group name must be unique.
 *
 */
void declare_group(char *id, char *using_clause)
{
	SYMBOL *sym, *using;

	sym = sym_lookup(id);
	if( sym ) {
		Bre_Parse_Error(BREMSG_GROUP_CLASH, id, symkind_string(sym->kind));
	}

	if( using_clause == NULL ) {
		Bre_Parse_Error(BREMSG_GROUP_USING, id);
	}

	using = sym_lookup(using_clause);
	if( using == NULL ) {
		Bre_Parse_Error(BREMSG_UNKNOWN_TABLENAME, using_clause);
	} else if( using->kind != S_TABLE ) {
		Bre_Parse_Error(BREMSG_TABLE_EXPECTED,
				symkind_string(using->kind), using_clause);
	}

	sym = sym_insert(id, S_GROUP);
	sym->u.grp.entry_point	= 0;
	sym->u.grp.using	= using;
	sym->u.grp.refcount	= 0;

	/* configure the parsing context for a new rule */
	reset_parse_context();
	pc.func			= NULL;
	pc.rule			= NULL;
	pc.group		= sym;
	pc.using_table		= NULL;
	pc.last_stmt		= STMT_UNKNOWN;
	pc.stmt_count		= 0;

	sym->u.grp.entry_point = gencode_function_entry(BRE()->prog);

	BRE_FREE(id);
	BRE_FREE(using_clause);

	/*
	 * Declare the two parameters 'previous_row' and 'current_row'
	 */
	sym = sym_lookup("previous_row");
	if( sym != NULL ) {
		Bre_Parse_Warn(BREMSG_REDEF_PRVROW);
	} else {
		sym = sym_insert("previous_row", S_PARAMETER);
		sym->u.par.refcount		= 0;
		sym->u.par.type.primary		= T_ROW;
		sym->u.par.type.u.symbol	= using;
		sym->u.par.fp			= 0;
	}

	sym = sym_lookup("current_row");
	if( sym != NULL ) {
		Bre_Parse_Warn(BREMSG_REDEF_CURROW);
	} else {
		sym = sym_insert("current_row", S_PARAMETER);
		sym->u.par.refcount		= 0;
		sym->u.par.type.primary		= T_ROW;
		sym->u.par.type.u.symbol	= using;
		sym->u.par.fp			= 1;
	}
}

/*----------------------------------------------------------------------
 * GROUP id USING using IS expr
 *
 * LANGUAGE SEMANTICS:
 *	1. Type of expression 'expr' must be boolean.
 *
 *
 */
void terminate_group(EXPR *expr)
{
	SYMBOL *sym;

	sym = pc.group;

	if( expr->type.primary != T_BOOLEAN ) {
		Bre_Parse_Warn(BREMSG_GROUP_RETURN, sym->name);
	}

	if( bre_error_count() == 0 )
		gencode(G_GENCODE, BRE()->prog, expr);

	gencode_function_exit(BRE()->prog, sym->u.grp.entry_point, 0, 2);

	sym = sym_lookup("current_row");
	BRE_ASSERT( sym != NULL );
	sym_remove(sym);

	sym = sym_lookup("previous_row");
	BRE_ASSERT( sym != NULL );
	sym_remove(sym);

	reset_parse_context();
}
