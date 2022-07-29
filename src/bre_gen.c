/*** $Id$ ***/

/*
 */
#ifndef lint
static char rcsid[] = "@(#)$Id$";
#endif

/***********************************************************************
 * BRE Byte-Code Generation routines
 *
 * This file contains the routines that traverse the parse tree, and
 * generate the byte codes.
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
 * To generate code for this parse tree, we enter into a recursive
 * algorithm that starts at the leaves of the tree and works its
 * way back up.
 *
 * The byte code for this expression would look something like:
 *
 *	OP_GETFP	'foo'		; Fetch foo and place value on stack.
 *	OP_PUSH_F	12.3		; Push '12.3' on the stack.
 *	OP_ADD_F			; add operator
 *	OP_PUSH_I	3000		; push '3000' on stack.
 *	OP_CONV_IF			; convert integer to float.
 *	OP_CMP_F			; comare two floats.
 *	OP_GE				; greater than or equal?
 *	...
 *
 */

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <string.h>
#include <assert.h>
#include <limits.h>
#include <float.h>

#include "bre_api.h"
#include "bre_prv.h"

/*----------------------------------------------------------------------
 * Byte-code generation routines. The following routines
 * recursively traverse the parser tree and generate
 * byte-code.
 *
 *	gencode()				<- top-level function
 *	gencode_relational()
 *	gencode_equality()
 *	gencode_arithmetic()
 *	gencode_add_list()
 *	gencode_logical()
 *	gencode_if()
 *	gencode_ifelse()
 *	gencode_in()
 *	gencode_between()
 *	gencode_block()
 *	gencode_function_args()
 *	gencode_external_args()
 *	gencode_call()
 *	gencode_assignment()
 *	gencode_null()
 *	gencode_pop()
 *	gencode_variable()
 *	gencode_bre_datum()
 *	gencode_list()
 *	gencode_substr()
 *	gencode_sublst()
 *	gencode_constant()
 *	gencode_error_clause()
 *	gencode_count()
 *	gencode_grpfunc_group()
 *	gencode_grpfunc()
 *	gencode_dot()
 *	gencode_lookup_index()
 *	gencode_for_list()
 *	gencode_for_string()
 *	gencode_for_table()
 *	gencode_for_group()
 *	gencode_for()
 *	gencode_table_index()
 *	gencode_list_index()
 *	gencode_group_name()
 *	gencode_compute_group()
 *	gencode_compute()
 */

/*----------------------------------------------------------------------
 * Generate code for equality operator: = <>
 *
 */
static void gencode_equality(GENCODE_MODE mode, PROGRAM *prog, EXPR *expr)
{
	OPCODE opcode, op1, op2;
	PROGRAM_INSTRUCTION instr;
	int convert_left, convert_right;
	int null_test;

	convert_left = 0;
	convert_right = 0;
	if( expr->left->type.primary != expr->right->type.primary ) {
		if( expr->left->type.primary == T_FLOAT )
			convert_right = 1;

		if( expr->right->type.primary == T_FLOAT )
			convert_left = 1;
	}

	if( expr->left->type.primary == T_FLOAT
					|| expr->right->type.primary == T_FLOAT ) {
		op1 = OP_CMP_F;
		op2 = OP_ISNULL_F;
	} else if( expr->left->type.primary == T_INTEGER
					|| expr->right->type.primary == T_INTEGER ) {
		op1 = OP_CMP_I;
		op2 = OP_ISNULL_I;
	} else if( expr->left->type.primary == T_BOOLEAN ) {
		op1 = OP_CMP_I;
		op2 = OP_ISNULL_I;
	} else if( expr->left->type.primary == T_STRING ) {
		op1 = OP_CMP_S;
		op2 = OP_ISNULL_S;
	} else if( expr->left->type.primary == T_ROW ) {
		op1 = OP_CMP_TR;
		op2 = OP_ISNULL_TR;
	} else if( expr->left->type.primary == T_REC ) {
		op1 = OP_CMP_I;
		op2 = OP_ISNULL_L;
	} else
		BRE_ASSERT(0);

	opcode = op1;

	null_test = 0;
	if( expr->left->op == E_NULL ) {
		opcode = op2;
		null_test = 1;
		BRE_FREE(expr->left);
	} else {
		gencode(mode, prog, expr->left);
		if( convert_left ) {
			instr.op = OP_CONV_IF;
			prog_encode(prog, &instr);
		}
	}

	if( expr->right->op == E_NULL ) {
		opcode = op2;
		null_test = 1;
		BRE_FREE(expr->right);
	} else {
		gencode(mode, prog, expr->right);
		if( convert_right ) {
			instr.op = OP_CONV_IF;
			prog_encode(prog, &instr);
		}
	}

	instr.op = opcode;
	prog_encode(prog, &instr);

	if( !null_test ) {
		instr.op = OP_EQ;
		prog_encode(prog, &instr);
	}
}

/*----------------------------------------------------------------------
 * Generate code for relational operators:
 *	> >= < <= <>
 *
 */
static void gencode_relational(GENCODE_MODE mode, PROGRAM *prog, EXPR *expr, OPCODE relop)
{
	OPCODE opcode;
	PROGRAM_INSTRUCTION instr;
	int convert_left, convert_right;

	convert_left = 0;
	convert_right = 0;
	if( expr->left->type.primary != expr->right->type.primary ) {
		if( expr->left->type.primary == T_FLOAT )
			convert_right = 1;

		if( expr->right->type.primary == T_FLOAT )
			convert_left = 1;
	}

	if( expr->left->type.primary == T_FLOAT
					|| expr->right->type.primary == T_FLOAT )
		opcode = OP_CMP_F;
	else if( expr->left->type.primary == T_INTEGER
					|| expr->right->type.primary == T_INTEGER )
		opcode = OP_CMP_I;
	else if( expr->left->type.primary == T_STRING )
		opcode = OP_CMP_S;
	else if( expr->left->type.primary == T_ROW )
		opcode = OP_CMP_TR;
	else if( expr->left->type.primary == T_REC )
		opcode = OP_CMP_I;
	else
		BRE_ASSERT(0);

	gencode(mode, prog, expr->left);
	if( convert_left ) {
		instr.op = OP_CONV_IF;
		prog_encode(prog, &instr);
	}

	gencode(mode, prog, expr->right);
	if( convert_right ) {
		instr.op = OP_CONV_IF;
		prog_encode(prog, &instr);
	}

	instr.op = opcode;
	prog_encode(prog, &instr);

	instr.op = relop;
	prog_encode(prog, &instr);
}

/*----------------------------------------------------------------------
 * Generate code for the arithmetic operators.
 *
 */
static void gencode_arithmetic(GENCODE_MODE mode, PROGRAM *prog, EXPR *expr, OPCODE opi, OPCODE opf)
{
	PROGRAM_INSTRUCTION instr;
	OPCODE opcode;
	int convert_left, convert_right;

	convert_left = 0;
	convert_right = 0;
	if( expr->type.primary == T_FLOAT ) {
		if( expr->left->type.primary == T_INTEGER )
			convert_left = 1;
		if( expr->right->type.primary == T_INTEGER )
			convert_right = 1;

		opcode = opf;
	} else {
		opcode = opi;
	}

	gencode(mode, prog, expr->left);

	if( convert_left ) {
		instr.op = OP_CONV_IF;
		prog_encode(prog, &instr);
	}

	gencode(mode, prog, expr->right);

	if( convert_right ) {
		instr.op = OP_CONV_IF;
		prog_encode(prog, &instr);
	}

	instr.op = opcode;
	prog_encode(prog, &instr);
}

/*----------------------------------------------------------------------
 * Generate code for list append and list prepend operations.
 *
 */
static void gencode_add_list(GENCODE_MODE mode, PROGRAM *prog, EXPR *expr)
{
	PROGRAM_INSTRUCTION instr;
	EXPR *lst, *val;
	OPCODE opcode, convop;
	int convert;

	if( expr->left->type.primary == T_LIST ) {
		opcode = OP_APPEND;
		lst = expr->left;
		val = expr->right;
	} else {
		opcode = OP_PREPEND;
		lst = expr->right;
		val = expr->left;
	}

	if( lst->type.u.subtype == T_FLOAT && val->type.primary == T_INTEGER ) {
		convert	= 1;
		convop	= OP_CONV_IF;
	} else if( lst->type.u.subtype == T_INTEGER && val->type.primary == T_FLOAT ) {
		convert	= 1;
		convop	= OP_CONV_FI;
	} else
		convert = 0;

	gencode(mode, prog, lst);
	gencode(mode, prog, val);

	if( convert ) {
		instr.op = convop;
		prog_encode(prog, &instr);
	}

	instr.op = opcode;
	prog_encode(prog, &instr);
}

/*----------------------------------------------------------------------
 * Generate code for logical operators: OR AND
 *
 *             lc1                      lc2
 * +-------+ +------+ +---+ +---------+ +----
 * |   A   | |Branch| |POP| |    B    | | ......
 * |       | | T/F  | |   | |         | |
 * +-------+ +------+ +---+ +---------+ +----
 *
 * Short cicruit behavior,
 * The OR operator branches if A is TRUE.
 * The AND operator branches if A if false.
 *
 */
static void gencode_logical(GENCODE_MODE mode, PROGRAM *prog, EXPR *expr, OPCODE opcode)
{
	PROGRAM_INSTRUCTION instr, instr2;
	int lc1, lc2;

	gencode(mode, prog, expr->left);

	instr.op = opcode;
	instr.l[0] = 0;
	instr.b[0] = 0;
	lc1 = prog_encode(prog, &instr);

	instr2.op = OP_POP;
	prog_encode(prog, &instr2);

	gencode(mode, prog, expr->right);

	lc2 = prog_get_lc(prog);
	instr.l[0] = lc2;
	instr.b[0] = 0;
	prog_patch_instruction(prog, lc1, &instr);
}

/*----------------------------------------------------------------------
 * Generate code for for:
 *	IF (A) THEN (B)
 *
 * BOOLEAN CASE:
 *
 *                    lc1                     lc2
 * +-------+ +---+ +------+ +---+ +---------+ +----
 * |   A   | |NOT| |Branch| |POP| |    B    | | ......
 * |       | |   | |true  | |   | |         | |
 * +-------+ +---+ +------+ +---+ +---------+ +----
 *                    |________________________^
 *
 * ALL OTHER CASES:
 *                    lc1                lc2
 * +-------+ +------+ +---+ +---------+ +--------
 * |   A   | |Branch| |POP| |    B    | | ......
 * |       | |false | |   | |         | |
 * +-------+ +------+ +---+ +---------+ +--------
 *              |________________________^
 *
 */
static void gencode_if(GENCODE_MODE mode, PROGRAM *prog, EXPR *expr)
{
	PROGRAM_INSTRUCTION instr, instr2;
	int lc1, lc2;

	if( expr->type.primary == T_BOOLEAN ) {
		/*
		 * This is the BOOLEAN case, where
		 * we simulate the statement:
		 *	if A then B
		 * as the logical expression:
		 *	(not A or B)
		 */
		gencode(mode, prog, expr->left);

		instr.op = OP_NOT;
		prog_encode(prog, &instr);

		instr.op = OP_BRANCH_TRUE;
		instr.b[0] = 0;
		instr.l[0] = 0;
		lc1 = prog_encode(prog, &instr);

		instr2.op = OP_POP;
		prog_encode(prog, &instr2);

		gencode(mode, prog, expr->right);

		lc2 = prog_get_lc(prog);

		instr.op = OP_BRANCH_TRUE;
		instr.b[0] = 0;
		instr.l[0] = lc2;
		prog_patch_instruction(prog, lc1, &instr);
	} else {
		/*
		 * This is the case when the IF statement
		 * does not involve a boolean.
		 *
		 * if a > 100 then
		 *	12 * a
		 *
		 */
		gencode(mode, prog, expr->left);

		instr.op = OP_BRANCH_FALSE;
		instr.b[0] = 0;
		instr.l[0] = 0;
		lc1 = prog_encode(prog, &instr);

		instr2.op = OP_POP;
		prog_encode(prog, &instr2);

		gencode(mode, prog, expr->right);

		lc2 = prog_get_lc(prog);

		instr.op = OP_BRANCH_FALSE;
		instr.b[0] = 0;
		instr.l[0] = lc2;
		prog_patch_instruction(prog, lc1, &instr);
	}
}

/*----------------------------------------------------------------------
 * Generate code for for:
 *	IF (A) THEN (B) ELSE (C)
 *
 *             lc1                  lc2    lc3         lc4
 * +-------+ +------+ +---------+ +------+ +--------+ +----
 * |   A   | |Branch| |    B    | |branch| |   C    | | ...
 * |       | |false | |         | |      | |        | |
 * +-------+ +------+ +---------+ +------+ +--------+ +----
 *              |____________________|______^          ^
 *                                   |_________________|
 *
 */
static void gencode_ifelse(GENCODE_MODE mode, PROGRAM *prog, EXPR *expr)
{
	PROGRAM_INSTRUCTION instr, instr1, instr2;
	int lc1, lc2, lc3, lc4;

	gencode(mode, prog, expr->left);

	instr1.op = OP_BRANCH_FALSE;
	instr1.b[0] = 1;
	instr1.l[0] = 0;
	lc1 = prog_encode(prog, &instr1);

	gencode(mode, prog, expr->right->left);

	if( expr->type.primary == T_FLOAT && expr->right->left->type.primary == T_INTEGER ) {
		instr.op = OP_CONV_IF;
		prog_encode(prog, &instr);
	}

	instr2.op = OP_BRANCH;
	instr2.l[0] = 0;
	lc2 = prog_encode(prog, &instr2);

	lc3 = prog_get_lc(prog);
	gencode(mode, prog, expr->right->right);

	if( expr->type.primary == T_FLOAT && expr->right->right->type.primary == T_INTEGER ) {
		instr.op = OP_CONV_IF;
		prog_encode(prog, &instr);
	}

	lc4 = prog_get_lc(prog);

	instr1.l[0] = lc3;
	prog_patch_instruction(prog, lc1, &instr1);

	instr2.l[0] = lc4;
	prog_patch_instruction(prog, lc2, &instr2);

	BRE_FREE(expr->right);
}

/*----------------------------------------------------------------------
 * Generate code for the IN operator
 *	Two basic variants are possible.
 *	a. Check a data value in a list of values.
 *	b. Check a string to see if it is a substring of another string.
 *
 */
static void gencode_in(GENCODE_MODE mode, PROGRAM *prog, EXPR *expr)
{
	EXPR *left, *right;
	OPCODE opcode;
	int convert_left;
	OPCODE convert_opcode;
	PROGRAM_INSTRUCTION instr;

	convert_left = 0;

	left	= expr->left;
	right	= expr->right;

	if( right->type.primary == T_STRING ) {
		opcode = OP_IN_SS;

	} else if( right->type.u.subtype == T_BOOLEAN || right->type.u.subtype == T_INTEGER ) {
		if( left->type.primary == T_FLOAT ) {
			convert_left = 1;
			convert_opcode = OP_CONV_FI;
		}

		opcode = OP_IN_IL;

	} else if( right->type.u.subtype == T_FLOAT ) {
		if( left->type.primary == T_INTEGER ) {
			convert_left = 1;
			convert_opcode = OP_CONV_IF;
		}

		opcode = OP_IN_FL;

	} else if( right->type.u.subtype == T_STRING ) {
		opcode = OP_IN_SL;

	} else {
		BRE_ASSERT(0);
	}

	gencode(mode, prog, expr->left);

	if( convert_left ) {
		instr.op = convert_opcode;
		prog_encode(prog, &instr);
	}

	gencode(mode, prog, expr->right);

	instr.op = opcode;
	prog_encode(prog, &instr);
}

/*----------------------------------------------------------------------
 * Generate code for the BETWEEN operator
 *
 */
static void gencode_between(GENCODE_MODE mode, PROGRAM *prog, EXPR *expr)
{
	EXPR *left, *right;
	OPCODE opcode;
	int convert_left;
	OPCODE convert_opcode;
	PROGRAM_INSTRUCTION instr;

	convert_left = 0;

	left	= expr->left;
	right	= expr->right;

	if( right->type.u.subtype == T_INTEGER ) {
		if( left->type.primary == T_FLOAT ) {
			convert_left = 1;
			convert_opcode = OP_CONV_FI;
		}

		opcode = OP_BETWEEN_I;

	} else if( right->type.u.subtype == T_FLOAT ) {
		if( left->type.primary == T_INTEGER ) {
			convert_left = 1;
			convert_opcode = OP_CONV_IF;
		}

		opcode = OP_BETWEEN_F;

	} else if( right->type.u.subtype == T_STRING ) {
		opcode = OP_BETWEEN_S;

	} else {
		BRE_ASSERT(0);
	}

	gencode(mode, prog, expr->left);

	if( convert_left ) {
		instr.op = convert_opcode;
		prog_encode(prog, &instr);
	}

	gencode(mode, prog, expr->right);

	instr.op = opcode;
	prog_encode(prog, &instr);
}

/*----------------------------------------------------------------------
 * Generate code for the BEGIN ... END block
 *
 */
static void gencode_block(GENCODE_MODE mode, PROGRAM *prog, EXPR *expr)
{
	PROGRAM_INSTRUCTION instr;
	EXPR *curr, *nxt, *stmt;

	if( expr->right ) {
		curr=expr->right;
		while(curr) {
			stmt = curr->left;
			nxt = curr->right;

			gencode(mode, prog, stmt);

			if( nxt )
				gencode_pop(mode, prog);

			BRE_FREE(curr);
			curr = nxt;
		}
	} else {
		/*
		 * An empty block still needs to put something
		 * on the stack. So we simply push "0"
		 */
		instr.op = OP_PUSH_I;
		instr.l[0] = 0;
		prog_encode(prog, &instr);
	}
}

/*----------------------------------------------------------------------
 * Generate the code for the function argument
 * expressions (in reverse order).
 *
 * Convert data types as nessesary.
 *
 */
static void gencode_function_args(GENCODE_MODE mode, PROGRAM *prog, SYMBOL_LIST *sl, EXPR *expr)
{
	PROGRAM_INSTRUCTION instr;
	SYMBOL *sym;
	EXPR *arg;
	SYM_TYPE arg_type;

	if( expr == NULL )
		return;

	gencode_function_args(mode, prog, sl->next, expr->right);

	arg = expr->left;
	arg_type = arg->type;
	sym = sl->sym;

	gencode(mode, prog, arg);

	if( sym->u.par.type.primary != arg_type.primary ) {
		if( sym->u.par.type.primary == T_INTEGER )
			instr.op = OP_CONV_FI;
		else
			instr.op = OP_CONV_IF;

		prog_encode(prog, &instr);
	}

	BRE_FREE(expr);
}

/*----------------------------------------------------------------------
 * Generate the code for the external function argument
 * expressions (in order of occurance).
 */
static void gencode_external_args(GENCODE_MODE mode, PROGRAM *prog, char *tstr, EXPR *expr)
{
	PROGRAM_INSTRUCTION instr;
	SYM_TYPE type, arg_type;
	EXPR *arg, *nxt;

	if( expr == NULL )
		return;

	arg = expr->left;
	arg_type = arg->type;
	nxt = expr->right;

	gencode(mode, prog, arg);

	type = type_from_char( *tstr );

	if( type.primary != arg_type.primary ) {
		if( type.primary == T_INTEGER )
			instr.op = OP_CONV_FI;
		else
			instr.op = OP_CONV_IF;
		prog_encode(prog, &instr);
	}

	BRE_FREE(expr);
	gencode_external_args(mode, prog, tstr+1, nxt);
}

/*----------------------------------------------------------------------
 * Generate code for a function call.
 *
 */
static void gencode_call(GENCODE_MODE mode, PROGRAM *prog, EXPR *expr)
{
	SYMBOL *sym;
	int lc;
	BRE_EXTERNAL_CB *callback;
	int nargs_in, nargs_out;
	PROGRAM_INSTRUCTION instr;
	void *external_cookie;

	sym = expr->symbol;
	if( sym->kind == S_FUNCTION ) {
		gencode_function_args(mode, prog, sym->u.fun.parms, expr->right);
		lc = sym->u.fun.entry_point;
		instr.op	= OP_CALL;
		instr.l[0]	= lc;
	} else {
		nargs_in	= strlen(sym->u.ext.args_in);
		nargs_out	= strlen(sym->u.ext.args_out);
		callback	= sym->u.ext.func;
		external_cookie	= sym->u.ext.external_cookie;

		gencode_external_args(mode, prog, sym->u.ext.args_in, expr->right);

		instr.op	= OP_EXT_FUNC;
		instr.s[0]	= nargs_in;
		instr.s[1]	= nargs_out;
		instr.str[0]	= sym->u.ext.args_in;
		instr.str[1]	= sym->u.ext.args_out;
		instr.str[2]	= sym->name;
		instr.ptr[0]	= (void*)callback;
		instr.ptr[1]	= external_cookie;
	}

	prog_encode(prog, &instr);
}

/*----------------------------------------------------------------------
 * (NOTE: FP stands for 'Frame Pointer')
 *
 * Calculate the proper FP offset for the given parameter
 * or variable symbol.
 *
 * STACK FRAME NOTES:
 *
 *	+-------------------------------+
 *	|	Parameter 3 (fp=2)	|  FP-4
 *	+-------------------------------+
 *	|	Parameter 2 (fp=1)	|  FP-3
 *	+-------------------------------+
 *	|Parm. 1  or Implicit (fp=0)	|  FP-2
 *	+-------------------------------+
 *	|	return PC		|  FP-1
 *	+-------------------------------+
 *	|	restore FP		|  <------- FP (Frame Pointer)
 *	+-------------------------------+
 *	|	Variable 1 (fp=0)	|  FP+1
 *	+-------------------------------+
 *	|	Variable 2 (fp=1)	|  FP+2
 *	+-------------------------------+
 *	|	Variable 3 (fp=2)	|  FP+3
 *	+-------------------------------+
 *	|				|
 *	+-------------------------------+
 *	|				|
 *	+-------------------------------+
 *	|				|
 *	+-------------------------------+
 *	|				| <------ SP (Stack Pointer)
 *	+-------------------------------+
 *
 *	FP offset calculation for a variable:
 *		FP_OFFSET = fp+1
 *
 *	FP offset calculation for a parameter
 *		FP_OFFSET = -fp - 2
 *
 */
static int calculate_offset(SYM_KIND kind, int fp)
{
	int fp_offset;

	if( kind == S_VARIABLE )
		fp_offset = fp + 1;
	else if( kind == S_PARAMETER )
		fp_offset =  - fp - 2;
	else
		BRE_ASSERT(0);

	return fp_offset;
}

/*----------------------------------------------------------------------
 * Generate code for assignment statement.
 */
static void gencode_assignment(GENCODE_MODE mode, PROGRAM *prog, EXPR *expr)
{
	SYMBOL *sym;
	PROGRAM_INSTRUCTION instr;
	SYM_TYPE type;

	sym = expr->symbol;
	type = expr->right->type;

	gencode(mode, prog, expr->right);

	if( type.primary == T_FLOAT && sym->u.var.type.primary == T_INTEGER ) {
		instr.op = OP_CONV_FI;
		prog_encode(prog, &instr);
	} else if( type.primary == T_INTEGER && sym->u.var.type.primary == T_FLOAT ) {
		instr.op = OP_CONV_IF;
		prog_encode(prog, &instr);
	}

	instr.op	= OP_ASSIGN;
	instr.b[0]	= 0;
	instr.s[0]	= calculate_offset(sym->kind, sym->u.var.fp);

	prog_encode(prog, &instr);
}

/*----------------------------------------------------------------------
 * The E_NULL expression node represents places in the input
 * where 'null' was used in an expression.
 *
 * The parse tree builder will have already assigned a type
 * to the null value based on context.
 *
 */
static void gencode_null(GENCODE_MODE mode, PROGRAM *prog, EXPR *expr)
{
	PROGRAM_INSTRUCTION instr;
	
	if( expr->type.primary == T_BOOLEAN ) {
		instr.op = OP_NULL_I;
	} else if( expr->type.primary == T_INTEGER ) {
		instr.op = OP_NULL_I;
	} else if( expr->type.primary == T_FLOAT ) {
		instr.op = OP_NULL_F;
	} else if( expr->type.primary == T_STRING ) {
		instr.op = OP_NULL_S;
	} else if( expr->type.primary == T_LIST ) {
		instr.op = OP_NULL_L;
	} else if( expr->type.primary == T_ROW ) {
		instr.op = OP_NULL_TR;
		instr.ptr[0] = expr->type.u.symbol;
	} else if( expr->type.primary == T_REC ) {
		instr.op = OP_NULL_L;
	} else {
		BRE_ASSERT(0);
	}

	prog_encode(prog, &instr);
}

/*----------------------------------------------------------------------
 * Generate a POP instruction for the type of object 'type'
 *
 */
void gencode_pop(GENCODE_MODE mode, PROGRAM *prog)
{
	PROGRAM_INSTRUCTION instr;

	instr.op = OP_POP;
	prog_encode(prog, &instr);
}

/*----------------------------------------------------------------------
 * Compile code for a variable/parameter access.
 *
 */
static void gencode_variable(GENCODE_MODE mode, PROGRAM *prog, EXPR *expr)
{
	SYMBOL *sym;
	int fp;
	PROGRAM_INSTRUCTION instr;

	sym = expr->symbol;

	if( sym->kind == S_VARIABLE )
		fp	= sym->u.var.fp;
	else if( sym->kind == S_PARAMETER )
		fp	= sym->u.par.fp;
	else
		BRE_ASSERT(0);

	instr.op	= OP_GETFP;
	instr.s[0]	= calculate_offset(sym->kind, fp);
	prog_encode(prog, &instr);
}

/*----------------------------------------------------------------------
 * This function allocates a copy of the list in 'datum' and
 * and generates the byte codes to push this list.(OP_PUSH_L).
 *
 */
static void gencode_bre_list(GENCODE_MODE mode, PROGRAM *prog, EXPR *expr, BRE_DATUM *datum)
{
	PROGRAM_INSTRUCTION instr;
	BRE_DATUM ndatum;
	BRE_LIST old_list, new_list;
	int i;

	old_list = datum->l;
	new_list.len = old_list.len;
	new_list.list = (BRE_DATUM*)BRE_MALLOC( sizeof(BRE_DATUM)*old_list.len );
	if( new_list.list == NULL )
		Bre_Error(BREMSG_DIE_NOMEM);

	if( expr->type.u.subtype == T_STRING ) {
		for(i=0; i<new_list.len; i++) {
			new_list.list[i] = old_list.list[i];
			if( old_list.list[i].s.str )
				new_list.list[i].s.str = BRE_STRDUP(old_list.list[i].s.str);
		}
	} else {
		for(i=0; i<new_list.len; i++) {
			new_list.list[i] = old_list.list[i];
		}
	}

	if( expr->type.u.subtype == T_BOOLEAN || expr->type.u.subtype == T_INTEGER )
		instr.b[0] = 'I';
	else if( expr->type.u.subtype == T_FLOAT )
		instr.b[0] = 'F';
	else if( expr->type.u.subtype == T_STRING )
		instr.b[0] = 'S';
	else
		BRE_ASSERT(0);

	instr.op = OP_PUSH_L;
	instr.datum.l = new_list;
	prog_encode(prog, &instr);
}

/*----------------------------------------------------------------------
 * This function takes a BRE_DATUM and compiles the
 * opcodes needed to push it on the stack.
 * The type of 'datum' is contained in 'expr->type'
 *
 */
static void gencode_bre_datum(GENCODE_MODE mode, PROGRAM *prog, EXPR *expr, BRE_DATUM *datum)
{
	PROGRAM_INSTRUCTION instr;

	if( expr->type.primary == T_BOOLEAN ) {
		instr.op = OP_PUSH_I;
		instr.l[0] = datum->i.val;
		prog_encode(prog, &instr);
	} else if( expr->type.primary == T_INTEGER ) {
		if( datum->i.nil ) {
			instr.op = OP_NULL_I;
		} else {
			instr.op = OP_PUSH_I;
			instr.l[0] = datum->i.val;
		}
		prog_encode(prog, &instr);
	} else if( expr->type.primary == T_FLOAT ) {
		instr.op = OP_PUSH_F;
		instr.d[0] = datum->f.val;
		prog_encode(prog, &instr);
	} else if( expr->type.primary == T_STRING ) {
		if( datum->s.str == NULL ) {
			instr.op = OP_NULL_S;
		} else {
			instr.op = OP_PUSH_S;
			instr.str[0] = datum->s.str;
			instr.s[0] = strlen(datum->s.str);
		}
		prog_encode(prog, &instr);

	} else if( expr->type.primary == T_LIST ) {
			gencode_bre_list(mode, prog, expr, datum);
	} else
		BRE_ASSERT(0);
}

/*----------------------------------------------------------------------
 * Generate code for a list.
 *
 *
 *
 */
static void gencode_list(GENCODE_MODE mode, PROGRAM *prog, EXPR *expr)
{
	EXPR *curr, *item, *nxt;
	PROGRAM_INSTRUCTION instr;
	SYM_TYPE type;

	for(curr = expr->right; curr; curr=nxt) {
		item = curr->left;
		type = item->type;

		gencode(mode, prog, item);
		if( expr->type.u.subtype == T_FLOAT && type.primary == T_INTEGER ) {
			instr.op = OP_CONV_IF;
			prog_encode(prog, &instr);
		}
		nxt = curr->right;
		BRE_FREE(curr);
	}

	instr.op = OP_MKLIST;
	instr.s[0] = expr->value.i.val;

	prog_encode(prog, &instr);
}

/*----------------------------------------------------------------------
 * Generate code for the substring operator.
 * Generate the OP_SUBSTR.
 *
 */
static void gencode_substr(GENCODE_MODE mode, PROGRAM *prog, EXPR *expr)
{
	PROGRAM_INSTRUCTION instr;

	gencode(mode, prog, expr->left);
	gencode(mode, prog, expr->right->left);
	gencode(mode, prog, expr->right->right);

	instr.op = OP_SUBSTR;
	prog_encode(prog, &instr);

	BRE_FREE(expr->right);
}

/*----------------------------------------------------------------------
 * Generate code for the substring operator.
 * Generate the OP_SUBLST.
 *
 */
static void gencode_sublst(GENCODE_MODE mode, PROGRAM *prog, EXPR *expr)
{
	PROGRAM_INSTRUCTION instr;

	gencode(mode, prog, expr->left);
	gencode(mode, prog, expr->right->left);
	gencode(mode, prog, expr->right->right);

	instr.op = OP_SUBLST;
	prog_encode(prog, &instr);

	BRE_FREE(expr->right);
}

/*----------------------------------------------------------------------
 * A constant is accessed just like a function that takes
 * no arguments.
 *
 */
static void gencode_constant(GENCODE_MODE mode, PROGRAM *prog, EXPR *expr)
{
	PROGRAM_INSTRUCTION instr;
	SYMBOL *sym;

	sym = expr->symbol;

	instr.op = OP_CALL;
	instr.l[0] = sym->u.con.entry_point;
	prog_encode(prog, &instr);
}

/*----------------------------------------------------------------------
 * Generate code for the COUNT() group-function.
 *
 *
 */
static void gencode_count(GENCODE_MODE mode, PROGRAM *prog, EXPR *expr)
{
	PROGRAM_INSTRUCTION instr;
	SYM_TYPE type;

	BRE_ASSERT( expr->right != NULL );

	type = expr->right->type;
	gencode(mode, prog, expr->right);

	if( type.primary == T_STRING ) {
		instr.op = OP_COUNT_S;
	} else if( type.primary == T_LIST ) {
		instr.op = OP_COUNT_L;
	} else if( type.primary == T_ROW ) {
		instr.op = OP_COUNT_TR;
	} else
		BRE_ASSERT(0);

	prog_encode(prog, &instr);
}

static long group_start_integer(EXPR_OPERATOR op)
{
	switch( op ) {
	case E_MIN:	return LONG_MAX;
	case E_MAX:	return LONG_MIN;
	case E_SUM:	return 0;
	case E_COUNT:	return 0;
	default:
		BRE_ASSERT(0);
	}
}

static double group_start_float(EXPR_OPERATOR op)
{
	switch( op ) {
	case E_MIN:	return DBL_MAX;
	case E_MAX:	return DBL_MIN;
	case E_SUM:	return 0.0;
	case E_COUNT:	BRE_ASSERT(0);
	default:
		BRE_ASSERT(0);
	}
}

static char *group_start_string(EXPR_OPERATOR op)
{
	static char buf[ 100 ];		/* its okay to return a static string here */

	switch( op ) {
	case E_MIN:
		buf[0] = 0x7f; /* MAXIMUM ASCII VALUE */
		buf[1] = '\0';
		return buf;

	case E_MAX:
		buf[0] = 0x00; /* MINIMUM ASCI VALUE */
		buf[1] = '\0';
		return buf;

	case E_SUM:	BRE_ASSERT(0);
	case E_COUNT:	BRE_ASSERT(0);
	default:
		BRE_ASSERT(0);
	}
}

/*----------------------------------------------------------------------
 * Generate code for the SUM(), MIN(), and MAX() functions, when
 * a GROUP object was used. Eg.
 *
 *	GROUP_FUNC( group . field )
 *
 * (This is for stand alone mode)
 *
 */
static void gencode_grpfunc_group(GENCODE_MODE mode, PROGRAM *prog, EXPR *expr,
							OPCODE ops, OPCODE opi, OPCODE opf)
{
	PROGRAM_INSTRUCTION instr;
	EXPR *arg;
	int lc1, lc2, lc3;
	int lc_next, lc_loop, lc_skip, lc_out;
	int entry_point;

	arg = expr->right;

	entry_point = arg->left->symbol->u.grp.entry_point;

	gencode(mode, prog, arg->left);

	instr.op = OP_PUSH_I;
	instr.l[0] = 0;
	prog_encode(prog, &instr);

	instr.op = OP_NULL_I;
	prog_encode(prog, &instr);

	instr.op = OP_NULL_L;
	prog_encode(prog, &instr);

	instr.op = OP_GETSP;
	instr.s[0] = -4;
	prog_encode(prog, &instr);

	instr.op = OP_COUNT_TR;
	prog_encode(prog, &instr);

	instr.op = OP_BRANCH_ZERO;
	instr.l[0] = 0;
	instr.b[0] = 0;
	lc1 = prog_encode(prog, &instr);

/* NEXT: */
	if( expr->type.u.subtype == T_STRING ) {
		instr.op = OP_PUSH_S;
		instr.str[0] = group_start_string(expr->op);
		instr.s[0] = 1;
	} else if( expr->type.u.subtype == T_INTEGER ) {
		instr.op = OP_PUSH_I;
		instr.l[0] = group_start_integer(expr->op);
	} else if( expr->type.u.subtype == T_FLOAT ) {
		instr.op = OP_PUSH_F;
		instr.d[0] = group_start_float(expr->op);
	} else {
		BRE_ASSERT(0);
	}
	lc_next = prog_encode(prog, &instr);

/* LOOP */
	instr.op = OP_GETSP;
	instr.s[0] = -6;
	lc_loop = prog_encode(prog, &instr);

	instr.op = OP_GETSP;
	instr.s[0] = -6;
	prog_encode(prog, &instr);

	instr.op = OP_SET_ROW;
	prog_encode(prog, &instr);

	instr.op = OP_FETCH_FIELD;
	instr.ptr[0] = arg->right->symbol;
	instr.ptr[1] = (void*)BRE()->fieldquery_cb;
	instr.b[0] = 1; /* NEW */
	if( arg->right->type.primary == T_BOOLEAN
				|| arg->right->type.primary == T_INTEGER )
		instr.b[1] = 'I';
	else if( arg->right->type.primary == T_FLOAT )
		instr.b[1] = 'F';
	else if( arg->right->type.primary == T_STRING )
		instr.b[1] = 'S';
	else
		BRE_ASSERT(0);

	prog_encode(prog, &instr);

	instr.op = OP_GETSP;
	instr.s[0] = -2;
	prog_encode(prog, &instr);

	if( expr->type.u.subtype == T_STRING )
		instr.op = ops;
	else if( expr->type.u.subtype == T_INTEGER )
		instr.op = opi;
	else if( expr->type.u.subtype == T_FLOAT )
		instr.op = opf;
	else
		BRE_ASSERT(0);

	prog_encode(prog, &instr);

	instr.op = OP_PUTSP;
	instr.s[0] = -2;
	prog_encode(prog, &instr);

	instr.op = OP_GETSP;
	instr.s[0] = -5;
	prog_encode(prog, &instr);

	instr.op = OP_PUTSP;
	instr.s[0] = -5;
	prog_encode(prog, &instr);

	instr.op = OP_INCSP;
	instr.s[0] = -5;
	prog_encode(prog, &instr);

	instr.op = OP_GETSP;
	instr.s[0] = -5;
	prog_encode(prog, &instr);

	instr.op = OP_GETSP;
	instr.s[0] = -3;
	prog_encode(prog, &instr);

	instr.op = OP_CMP_I;
	prog_encode(prog, &instr);

	instr.op = OP_GE;
	prog_encode(prog, &instr);

	instr.op = OP_BRANCH_FALSE;
	instr.l[0] = 0;
	instr.b[0] = 1;
	lc2 = prog_encode(prog, &instr);

	instr.op = OP_GETSP;
	instr.s[0] = -3;
	prog_encode(prog, &instr);

	instr.op = OP_GETSP;
	instr.s[0] = -2;
	prog_encode(prog, &instr);

	instr.op = OP_APPEND;
	prog_encode(prog, &instr);

	instr.op = OP_PUTSP;
	instr.s[0] = -4;
	prog_encode(prog, &instr);

	instr.op = OP_POP;
	prog_encode(prog, &instr);

	instr.op = OP_BRANCH;
	instr.l[0] = 0;
	lc3 = prog_encode(prog, &instr);

/* skip: */
	instr.op = OP_GETSP;
	instr.s[0] = -6;
	lc_skip = prog_encode(prog, &instr);

	instr.op = OP_GETSP;
	instr.s[0] = -6;
	prog_encode(prog, &instr);

	instr.op = OP_SET_ROW;
	prog_encode(prog, &instr);

	instr.op = OP_GETSP;
	instr.s[0] = -7;
	prog_encode(prog, &instr);

	instr.op = OP_GETSP;
	instr.s[0] = -6;
	prog_encode(prog, &instr);

	instr.op = OP_SET_ROW;
	prog_encode(prog, &instr);

	instr.op = OP_CALL;
	instr.l[0] = entry_point;
	prog_encode(prog, &instr);

	instr.op = OP_BRANCH_TRUE;
	instr.l[0] = lc_loop;
	instr.b[0] = 1;
	prog_encode(prog, &instr);

	instr.op = OP_GETSP;
	instr.s[0] = -3;
	prog_encode(prog, &instr);

	instr.op = OP_GETSP;
	instr.s[0] = -2;
	prog_encode(prog, &instr);

	instr.op = OP_APPEND;
	prog_encode(prog, &instr);

	instr.op = OP_PUTSP;
	instr.s[0] = -4;
	prog_encode(prog, &instr);

	instr.op = OP_POP;
	prog_encode(prog, &instr);

	instr.op = OP_BRANCH;
	instr.l[0] = lc_next;
	prog_encode(prog, &instr);

/* OUT: */
	instr.op = OP_GETSP;
	instr.s[0] = -2;
	lc_out = prog_encode(prog, &instr);

	instr.op = OP_PUTSP;
	instr.s[0] = -6;
	prog_encode(prog, &instr);

	instr.op = OP_POP;
	prog_encode(prog, &instr);
	prog_encode(prog, &instr);
	prog_encode(prog, &instr);
	prog_encode(prog, &instr);
	
/* ---- patch instructions ------ */

	instr.op = OP_BRANCH_ZERO;
	instr.l[0] = lc_out;
	instr.b[0] = 0;
	prog_patch_instruction(prog, lc1, &instr);

	instr.op = OP_BRANCH_FALSE;
	instr.l[0] = lc_skip;
	instr.b[0] = 1;
	prog_patch_instruction(prog, lc2, &instr);

	instr.op = OP_BRANCH;
	instr.l[0] = lc_out;
	prog_patch_instruction(prog, lc3, &instr);

	BRE_FREE(expr->right);
	BRE_FREE(arg->right);
}

/*----------------------------------------------------------------------
 * Generate code for the SUM(), MIN(), and MAX() functions.
 * This function is called to generate code for stand-alone
 * calls to the group functions.
 *
 *	GROUP_FUNC(expr)
 *
 * 'expr' can be one of the following:
 *	a. A list of integer.
 *	b. A list of float.
 *	c. A E_DOT expression for a table, with a field that is an integer, string,
 *	   or float type.
 *	d. An E_DOT expression for a GROUP name and field that is an integer, string,
 *	   or float type.
 *
 */
static void gencode_grpfunc(GENCODE_MODE mode, PROGRAM *prog, EXPR *expr,
							OPCODE ops, OPCODE opi, OPCODE opf)
{
	PROGRAM_INSTRUCTION instr;
	EXPR *arg;
	int lc1, lc2, lc3;

	arg = expr->right;

	/*
	 * Check for more complex case involving E_GROUP object.
	 */
	if( arg->op == E_DOT && arg->left->op == E_GROUP ) {
		gencode_grpfunc_group(mode, prog, expr, ops, opi, opf);
		return;
	}

	if( arg->op == E_DOT ) {
		gencode(mode, prog, arg->left);
	} else {
		gencode(mode, prog, arg);
	}

	instr.op = OP_PUSH_I;
	instr.l[0] = 0;
	prog_encode(prog, &instr);

	if( expr->type.primary == T_STRING ) {
		instr.op = OP_PUSH_S;
		instr.str[0] = group_start_string(expr->op);
		instr.s[0] = 1;
	} else if( expr->type.primary == T_INTEGER ) {
		instr.op = OP_PUSH_I;
		instr.l[0] = group_start_integer(expr->op);
	} else if( expr->type.primary == T_FLOAT ) {
		instr.op = OP_PUSH_F;
		instr.d[0] = group_start_float(expr->op);
	} else {
		BRE_ASSERT(0);
	}

	prog_encode(prog, &instr);

/* LOOP BEGINS */
	instr.op = OP_GETSP;
	instr.s[0] = -3;
	lc1 = prog_encode(prog, &instr);

	if( arg->op == E_DOT ) {
		instr.op = OP_COUNT_TR;
		prog_encode(prog, &instr);
	} else {
		instr.op = OP_COUNT_L;
		prog_encode(prog, &instr);
	}

	instr.op = OP_GETSP;
	instr.s[0] = -3;
	prog_encode(prog, &instr);

	instr.op = OP_SUB_I;
	prog_encode(prog, &instr);

	instr.op = OP_BRANCH_ZERO;
	instr.l[0] = 0;
	instr.b[0] = 1;
	lc2 = prog_encode(prog, &instr);

	/*
	 * Compute group function
	 */

	instr.op = OP_GETSP;
	instr.s[0] = -3;
	prog_encode(prog, &instr);

	instr.op = OP_GETSP;
	instr.s[0] = -3;
	prog_encode(prog, &instr);

	if( arg->op == E_DOT ) {
		instr.op = OP_SET_ROW;
		prog_encode(prog, &instr);

		instr.op = OP_FETCH_FIELD;
		instr.ptr[0] = arg->right->symbol;
		instr.ptr[1] = (void*)BRE()->fieldquery_cb;
		instr.b[0] = 1; /* NEW */
		if( arg->right->type.primary == T_BOOLEAN
					|| arg->right->type.primary == T_INTEGER )
			instr.b[1] = 'I';
		else if( arg->right->type.primary == T_FLOAT )
			instr.b[1] = 'F';
		else if( arg->right->type.primary == T_STRING )
			instr.b[1] = 'S';
		else
			BRE_ASSERT(0);

		prog_encode(prog, &instr);
	} else {
		if( expr->type.primary == T_STRING )
			instr.op = OP_FETCH_LIST_S;
		else if( expr->type.primary == T_INTEGER )
			instr.op = OP_FETCH_LIST_I;
		else if( expr->type.primary == T_FLOAT )
			instr.op = OP_FETCH_LIST_F;
		else
			BRE_ASSERT(0);

		prog_encode(prog, &instr);
	}

	instr.op = OP_GETSP;
	instr.s[0] = -2;
	prog_encode(prog, &instr);

	if( expr->type.primary == T_STRING )
		instr.op = ops;
	else if( expr->type.primary == T_INTEGER )
		instr.op = opi;
	else if( expr->type.primary == T_FLOAT )
		instr.op = opf;
	else
		BRE_ASSERT(0);

	prog_encode(prog, &instr);

	instr.op = OP_PUTSP;
	instr.s[0] = -2;
	prog_encode(prog, &instr);

	instr.op = OP_INCSP;
	instr.s[0] = -2;
	prog_encode(prog, &instr);

	instr.op = OP_BRANCH;
	instr.l[0] = lc1;
	prog_encode(prog, &instr);

/* END LOOP */

	instr.op = OP_PUTSP;
	instr.s[0] = -3;
	lc3 = prog_encode(prog, &instr);

	instr.op = OP_POP;
	prog_encode(prog, &instr);

	instr.op = OP_BRANCH_ZERO;
	instr.l[0] = lc3;
	instr.b[0] = 1;
	prog_patch_instruction(prog, lc2, &instr);

	if( arg->op == E_DOT ) {
		BRE_FREE(expr->left);
		BRE_FREE(expr->right);
	}
}

/*----------------------------------------------------------------------
 * Generate byte codes for the '.' operator.
 * There are two types of objects that the '.' operator acts on.
 *
 * 1) A table row and a table field
 * 2) A lookup record and a lookup field.
 *
 * This function handles both cases.
 *
 *
 */
static void gencode_dot(GENCODE_MODE mode, PROGRAM *prog, EXPR *expr)
{
	PROGRAM_INSTRUCTION instr;
	EXPR *left, *right;
	SYMBOL *sym;
	OPCODE opcode;

	left	= expr->left;
	right	= expr->right;

	if( left->type.primary == T_ROW ) {
		gencode(mode, prog, left);

		instr.op = OP_FETCH_FIELD;
		instr.ptr[0] = (void*)right->symbol;
		instr.ptr[1] = (void*)BRE()->fieldquery_cb;
		if( right->op == E_OLDFIELD )
			instr.b[0] = 0;
		else
			instr.b[0] = 1;

		if( right->type.primary == T_BOOLEAN
					|| right->type.primary == T_INTEGER )
			instr.b[1] = 'I';
		else if( right->type.primary == T_FLOAT )
			instr.b[1] = 'F';
		else if( right->type.primary == T_STRING )
			instr.b[1] = 'S';
		else
			BRE_ASSERT(0);

		prog_encode(prog, &instr);

	} else if( left->type.primary == T_REC ) {
		gencode(mode, prog, left);

		sym = right->symbol;

		instr.op	= OP_PUSH_I;
		instr.l[0]	= sym->u.luf.offset;
		prog_encode(prog, &instr);

		if( right->type.primary == T_BOOLEAN
					|| right->type.primary == T_INTEGER )
			opcode = OP_FETCH_LIST_I;
		else if( right->type.primary == T_FLOAT )
			opcode = OP_FETCH_LIST_F;
		else if( right->type.primary == T_STRING )
			opcode = OP_FETCH_LIST_S;
		else
			BRE_ASSERT(0);

		instr.op = opcode;
		prog_encode(prog, &instr);

	} else {
		BRE_ASSERT(0);
	}

	BRE_FREE(expr->right);
}

/*----------------------------------------------------------------------
 * Generate code for a lookup table access.
 * A lookup access is very similar to an external function call.
 *
 */
static void gencode_lookup_index(GENCODE_MODE mode, PROGRAM *prog, EXPR *expr)
{
	PROGRAM_INSTRUCTION instr;
	SYMBOL *sym;
	int nargs_in, nargs_out;
	BRE_EXTERNAL_CB *callback;
	void *external_cookie;

	sym = expr->symbol;

	gencode_external_args(mode, prog, sym->u.ext.args_in, expr->right);

	nargs_in	= strlen(sym->u.ext.args_in);
	nargs_out	= strlen(sym->u.ext.args_out);
	callback	= sym->u.ext.func;
	external_cookie	= sym->u.ext.external_cookie;

	instr.op	= OP_EXT_LOOKUP;
	instr.s[0]	= nargs_in;
	instr.s[1]	= nargs_out;
	instr.str[0]	= sym->u.ext.args_in;
	instr.str[1]	= sym->u.ext.args_out;
	instr.str[2]	= sym->name;
	instr.ptr[0]	= (void*)callback;
	instr.ptr[1]	= external_cookie;
	prog_encode(prog, &instr);
}

static OPCODE opcode_type_select(SYM_TYPE type, OPCODE opb, OPCODE opi, OPCODE opf, OPCODE ops)
{
	switch( type.primary ) {
	case T_BOOLEAN:		return opb;
	case T_INTEGER:		return opi;
	case T_FLOAT:		return opf;
	case T_STRING:		return ops;
	default:
		BRE_ASSERT(0);
	}
}

/*----------------------------------------------------------------------
 * Generate code for a forall and forsome loop (using a list)
 *
 */
static void gencode_for_list(GENCODE_MODE mode, PROGRAM *prog, EXPR *expr)
{
	EXPR *body, *iterator, *vars;
	PROGRAM_INSTRUCTION instr;
	EXPR *v, *nxt;
	SYMBOL *sym;
	EXPR_OPERATOR type;
	int lc1, lc2, lc3, lc4, lc5;

	type	= expr->op;
	vars	= expr->left->left;
	iterator= expr->left->right;
	body	= expr->right;

	gencode(mode, prog, iterator);

	instr.op = OP_PUSH_I;
	instr.l[0] = 0;
	prog_encode(prog, &instr);

/* LOOP BEGINS */
	instr.op = OP_GETSP;
	instr.s[0] = -2;
	lc1 = prog_encode(prog, &instr);

	instr.op = OP_COUNT_L;
	prog_encode(prog, &instr);

	instr.op = OP_GETSP;
	instr.s[0] = -2;
	prog_encode(prog, &instr);

	instr.op = OP_SUB_I;
	prog_encode(prog, &instr);

	instr.op = OP_BRANCH_LT1;
	instr.l[0] = 0;
	instr.b[0] = 1;
	lc2 = prog_encode(prog, &instr);

	/*
	 * Assign the next element from
	 * the list to each variable.
	 */
	for(v=vars; v; v=nxt) {
		sym = v->symbol;

		instr.op = OP_GETSP;
		instr.s[0] = -2;
		prog_encode(prog, &instr);

		instr.op = OP_GETSP;
		instr.s[0] = -2;
		prog_encode(prog, &instr);

		instr.op = opcode_type_select(sym->u.var.type, OP_FETCH_LIST_I, OP_FETCH_LIST_I,
					OP_FETCH_LIST_F, OP_FETCH_LIST_S);

		prog_encode(prog, &instr);

		instr.op = OP_ASSIGN;
		instr.b[0] = 1;
		instr.s[0] = calculate_offset(sym->kind, sym->u.var.fp);
		prog_encode(prog, &instr);

		instr.op = OP_INCSP;
		instr.s[0] = -1;
		prog_encode(prog, &instr);

		nxt = v->right;
		BRE_FREE(v);
	
	}

	gencode(mode, prog, body);

	if( type == E_FORALL )
		instr.op = OP_BRANCH_TRUE;
	else
		instr.op = OP_BRANCH_FALSE;
	instr.l[0] = lc1;
	instr.b[0] = 1;
	prog_encode(prog, &instr);

	instr.op = OP_POP;
	prog_encode(prog, &instr);
	prog_encode(prog, &instr);

	instr.op = OP_PUSH_I;
	if( type == E_FORALL )
		instr.l[0] = 0;
	else
		instr.l[0] = 1;
	prog_encode(prog, &instr);

	instr.op = OP_BRANCH;
	instr.l[0] = 0;
	lc3 = prog_encode(prog, &instr);

	instr.op = OP_POP;
	lc4 = prog_encode(prog, &instr);
	prog_encode(prog, &instr);

	instr.op = OP_PUSH_I;
	if( type == E_FORALL )
		instr.l[0] = 1;
	else
		instr.l[0] = 0;
	prog_encode(prog, &instr);

	lc5 = prog_get_lc(prog);

	instr.op = OP_BRANCH_LT1;
	instr.l[0] = lc4;
	instr.b[0] = 1;
	prog_patch_instruction(prog, lc2, &instr);

	instr.op = OP_BRANCH;
	instr.l[0] = lc5;
	prog_patch_instruction(prog, lc3, &instr);

	BRE_FREE(expr->left);
}


/*----------------------------------------------------------------------
 * Generate code for a forall and forsome loop (using a string)
 *
 */
static void gencode_for_string(GENCODE_MODE mode, PROGRAM *prog, EXPR *expr)
{
	EXPR_OPERATOR type;
	EXPR *body, *iterator;
	SYMBOL *sym;
	PROGRAM_INSTRUCTION instr;
	int lc1, lc2, lc3, lc4, lc5;

	type	= expr->op;
	sym	= expr->left->left->symbol;
	iterator= expr->left->right;
	body	= expr->right;

	gencode(mode, prog, iterator);

	instr.op = OP_PUSH_I;
	instr.l[0] = 0;
	prog_encode(prog, &instr);

	/*
	 * LOOP BEGINS
	 */
	instr.op = OP_GETSP;
	instr.s[0] = -2;
	lc1 = prog_encode(prog, &instr);

	instr.op = OP_COUNT_S;
	prog_encode(prog, &instr);

	instr.op = OP_GETSP;
	instr.s[0] = -2;
	prog_encode(prog, &instr);

	instr.op = OP_SUB_I;
	prog_encode(prog, &instr);

	instr.op = OP_BRANCH_ZERO;
	instr.l[0] = 0;
	instr.b[0] = 1;
	lc2 = prog_encode(prog, &instr);

	/*
	 * ASSIGN NEW CHARACTER TO VARIABLE
	 */
	instr.op = OP_GETSP;
	instr.s[0] = -2;
	prog_encode(prog, &instr);

	instr.op = OP_GETSP;
	instr.s[0] = -2;
	prog_encode(prog, &instr);

	instr.op = OP_FETCH_CHAR;
	prog_encode(prog, &instr);

	instr.op = OP_ASSIGN;
	instr.b[0] = 1;
	instr.s[0] = calculate_offset(sym->kind, sym->u.var.fp);
	prog_encode(prog, &instr);

	instr.op = OP_INCSP;
	instr.s[0] = -1;
	prog_encode(prog, &instr);

	/*
	 * EXECUTE BODY OF FOR-LOOP
	 */
	gencode(mode, prog, body);

	if( type == E_FORALL )
		instr.op = OP_BRANCH_TRUE;
	else
		instr.op = OP_BRANCH_FALSE;
	instr.l[0] = lc1;
	instr.b[0] = 1;
	prog_encode(prog, &instr);

	instr.op = OP_POP;
	prog_encode(prog, &instr);
	prog_encode(prog, &instr);

	instr.op = OP_PUSH_I;
	if( type == E_FORALL )
		instr.l[0] = 0;
	else
		instr.l[0] = 1;
	prog_encode(prog, &instr);

	instr.op = OP_BRANCH;
	instr.l[0] = 0;
	lc3 = prog_encode(prog, &instr);

	instr.op = OP_POP;
	lc4 = prog_encode(prog, &instr);
	prog_encode(prog, &instr);

	instr.op = OP_PUSH_I;
	if( type == E_FORALL )
		instr.l[0] = 1;
	else
		instr.l[0] = 0;
	prog_encode(prog, &instr);

	lc5 = prog_get_lc(prog);

	instr.op = OP_BRANCH_ZERO;
	instr.l[0] = lc4;
	instr.b[0] = 1;
	prog_patch_instruction(prog, lc2, &instr);

	instr.op = OP_BRANCH;
	instr.l[0] = lc5;
	prog_patch_instruction(prog, lc3, &instr);

	BRE_FREE(expr->left->left);
	BRE_FREE(expr->left);
}

#if 0
/*----------------------------------------------------------------------
 * Generate code for a 'for all' 'for some' loop, when
 * the iterator expression is of the form:
 *	TABLE
 *
 */
static void gencode_for_table(GENCODE_MODE mode, PROGRAM *prog, EXPR *expr)
{
	EXPR_OPERATOR type;
	EXPR *body, *iterator;
	SYMBOL *sym;
	PROGRAM_INSTRUCTION instr;
	int lc1, lc2, lc3, lc4, lc5;

	type	= expr->op;
	sym	= expr->left->left->symbol;
	iterator= expr->left->right;
	body	= expr->right;

	gencode(mode, prog, iterator);

	instr.op = OP_PUSH_I;
	instr.l[0] = 0;
	prog_encode(prog, &instr);

	/*
	 * LOOP BEGINS
	 */
	instr.op = OP_GETSP;
	instr.s[0] = -2;
	lc1 = prog_encode(prog, &instr);

	instr.op = OP_COUNT_TR;
	prog_encode(prog, &instr);

	instr.op = OP_GETSP;
	instr.s[0] = -2;
	prog_encode(prog, &instr);

	instr.op = OP_SUB_I;
	prog_encode(prog, &instr);

	instr.op = OP_BRANCH_ZERO;
	instr.l[0] = 0;
	instr.b[0] = 1;
	lc2 = prog_encode(prog, &instr);

	/*
	 * ASSIGN NEW CHARACTER TO VARIABLE
	 */
	instr.op = OP_GETSP;
	instr.s[0] = -2;
	prog_encode(prog, &instr);

	instr.op = OP_GETSP;
	instr.s[0] = -2;
	prog_encode(prog, &instr);

	instr.op = OP_SET_ROW;
	prog_encode(prog, &instr);

	instr.op = OP_ASSIGN;
	instr.b[0] = 1;
	instr.s[0] = calculate_offset(sym->kind, sym->u.var.fp);
	prog_encode(prog, &instr);

	instr.op = OP_INCSP;
	instr.s[0] = -1;
	prog_encode(prog, &instr);

	/*
	 * EXECUTE BODY OF FOR-LOOP
	 */
	gencode(mode, prog, body);

	if( type == E_FORALL )
		instr.op = OP_BRANCH_TRUE;
	else
		instr.op = OP_BRANCH_FALSE;
	instr.l[0] = lc1;
	instr.b[0] = 1;
	prog_encode(prog, &instr);

	instr.op = OP_POP;
	prog_encode(prog, &instr);
	prog_encode(prog, &instr);

	instr.op = OP_PUSH_I;
	if( type == E_FORALL )
		instr.l[0] = 0;
	else
		instr.l[0] = 1;
	prog_encode(prog, &instr);

	instr.op = OP_BRANCH;
	instr.l[0] = 0;
	lc3 = prog_encode(prog, &instr);

	instr.op = OP_POP;
	lc4 = prog_encode(prog, &instr);
	prog_encode(prog, &instr);

	instr.op = OP_PUSH_I;
	if( type == E_FORALL )
		instr.l[0] = 1;
	else
		instr.l[0] = 0;
	prog_encode(prog, &instr);

	lc5 = prog_get_lc(prog);

	instr.op = OP_BRANCH_ZERO;
	instr.l[0] = lc4;
	instr.b[0] = 1;
	prog_patch_instruction(prog, lc2, &instr);

	instr.op = OP_BRANCH;
	instr.l[0] = lc5;
	prog_patch_instruction(prog, lc3, &instr);

	BRE_FREE(expr->left->left);
	BRE_FREE(expr->left);
}
#endif

/*----------------------------------------------------------------------
 * Generate code for a 'for all' 'for some' loop, when
 * the iterator expression is of the form:
 *	TABLE
 *
 * The OP_COUNT_TR instruction, has been moved outside the loop.
 *
 */
static void gencode_for_table(GENCODE_MODE mode, PROGRAM *prog, EXPR *expr)
{
	EXPR_OPERATOR type;
	EXPR *body, *iterator;
	SYMBOL *sym;
	PROGRAM_INSTRUCTION instr;
	int lc1, lc2, lc3, lc4, lc5;

	type	= expr->op;
	sym	= expr->left->left->symbol;
	iterator= expr->left->right;
	body	= expr->right;

	gencode(mode, prog, iterator);

	instr.op = OP_GETSP;
	instr.s[0] = -1;
	prog_encode(prog, &instr);

	instr.op = OP_COUNT_TR;
	prog_encode(prog, &instr);

	instr.op = OP_PUSH_I;
	instr.l[0] = 0;
	prog_encode(prog, &instr);

/*
 * LOOP BEGINS:
 */
	instr.op = OP_GETSP;
	instr.s[0] = -2;
	lc1 = prog_encode(prog, &instr);

	instr.op = OP_GETSP;
	instr.s[0] = -2;
	prog_encode(prog, &instr);

	instr.op = OP_SUB_I;
	prog_encode(prog, &instr);

	instr.op = OP_BRANCH_ZERO;
	instr.l[0] = 0;
	instr.b[0] = 1;
	lc2 = prog_encode(prog, &instr);

	/*
	 * ASSIGN NEW CHARACTER TO VARIABLE
	 */
	instr.op = OP_GETSP;
	instr.s[0] = -3;
	prog_encode(prog, &instr);

	instr.op = OP_GETSP;
	instr.s[0] = -2;
	prog_encode(prog, &instr);

	instr.op = OP_SET_ROW;
	prog_encode(prog, &instr);

	instr.op = OP_ASSIGN;
	instr.b[0] = 1;
	instr.s[0] = calculate_offset(sym->kind, sym->u.var.fp);
	prog_encode(prog, &instr);

	instr.op = OP_INCSP;
	instr.s[0] = -1;
	prog_encode(prog, &instr);

	/*
	 * EXECUTE BODY OF FOR-LOOP
	 */
	gencode(mode, prog, body);

	if( type == E_FORALL )
		instr.op = OP_BRANCH_TRUE;
	else
		instr.op = OP_BRANCH_FALSE;
	instr.l[0] = lc1;
	instr.b[0] = 1;
	prog_encode(prog, &instr);

	instr.op = OP_POP;
	prog_encode(prog, &instr);
	prog_encode(prog, &instr);
	prog_encode(prog, &instr);

	instr.op = OP_PUSH_I;
	if( type == E_FORALL )
		instr.l[0] = 0;
	else
		instr.l[0] = 1;
	prog_encode(prog, &instr);

	instr.op = OP_BRANCH;
	instr.l[0] = 0;
	lc3 = prog_encode(prog, &instr);

/* DONE: */
	instr.op = OP_POP;
	lc4 = prog_encode(prog, &instr);
	prog_encode(prog, &instr);
	prog_encode(prog, &instr);

	instr.op = OP_PUSH_I;
	if( type == E_FORALL )
		instr.l[0] = 1;
	else
		instr.l[0] = 0;
	prog_encode(prog, &instr);

/* patch instructions */
	lc5 = prog_get_lc(prog);

	instr.op = OP_BRANCH_ZERO;
	instr.l[0] = lc4;
	instr.b[0] = 1;
	prog_patch_instruction(prog, lc2, &instr);

	instr.op = OP_BRANCH;
	instr.l[0] = lc5;
	prog_patch_instruction(prog, lc3, &instr);

	BRE_FREE(expr->left->left);
	BRE_FREE(expr->left);
}

/*----------------------------------------------------------------------
 * Generate code for a 'for all' 'for some' loop, when
 * the iterator expression is of the form:
 *
 *	for all <first_row>, <x> in GROUP_NAME
 *		<body>
 *
 *	for some <first_row>, <x> in GROUP_NAME
 *		<body>
 *
 *
 */
static void gencode_for_group(GENCODE_MODE mode, PROGRAM *prog, EXPR *expr)
{
	EXPR_OPERATOR type;
	PROGRAM_INSTRUCTION instr;
	EXPR *iterator, *body;
	SYMBOL *sym1, *sym2;
	int lc1, lc2, lc3, lc4;
	int lc_next, lc_loop, lc_out;
	int entry_point;

	type	= expr->op;
	iterator= expr->left->right;
	body	= expr->right;
	sym1	= expr->left->left->symbol;
	sym2	= expr->left->left->right->symbol;

	entry_point = iterator->symbol->u.grp.entry_point;

	gencode(mode, prog, iterator);

	instr.op = OP_PUSH_I;
	instr.l[0] = 0;
	prog_encode(prog, &instr);

	instr.op = OP_NULL_I;
	prog_encode(prog, &instr);

	if( type == E_FORALL ) {
		instr.op = OP_PUSH_I;
		instr.l[0] = 1;
	} else if (type == E_FORSOME ) {
		instr.op = OP_PUSH_I;
		instr.l[0] = 0;
	} else
		BRE_ASSERT(0);

	prog_encode(prog, &instr);

	instr.op = OP_GETSP;
	instr.s[0] = -4;
	prog_encode(prog, &instr);

	instr.op = OP_COUNT_TR;
	prog_encode(prog, &instr);

	instr.op = OP_BRANCH_ZERO;
	instr.l[0] = 0;
	instr.b[0] = 0;
	lc1 = prog_encode(prog, &instr);

/* NEXT: */
	instr.op = OP_GETSP;
	instr.s[0] = -5;
	lc_next = prog_encode(prog, &instr);

	instr.op = OP_GETSP;
	instr.s[0] = -5;
	prog_encode(prog, &instr);

	instr.op = OP_SET_ROW;
	prog_encode(prog, &instr);

	instr.op = OP_ASSIGN;
	instr.b[0] = 1;
	instr.s[0] = calculate_offset(sym1->kind, sym1->u.var.fp);
	prog_encode(prog, &instr);

	if( type == E_FORALL ) {
		instr.op = OP_PUSH_I;
		instr.l[0] = 1;
	} else if( type == E_FORSOME ) {
		instr.op = OP_PUSH_I;
		instr.l[0] = 0;
	} else
		BRE_ASSERT(0);

	prog_encode(prog, &instr);

	instr.op = OP_PUTSP;
	instr.s[0] = -3;
	prog_encode(prog, &instr);

/* LOOP */
	instr.op = OP_GETSP;
	instr.s[0] = -5;
	lc_loop = prog_encode(prog, &instr);

	instr.op = OP_GETSP;
	instr.s[0] = -5;
	prog_encode(prog, &instr);

	instr.op = OP_SET_ROW;
	prog_encode(prog, &instr);

	instr.op = OP_ASSIGN;
	instr.b[0] = 1;
	instr.s[0] = calculate_offset(sym2->kind, sym2->u.var.fp);
	prog_encode(prog, &instr);

	gencode(mode, prog, body);

	instr.op = OP_GETSP;
	instr.s[0] = -3;
	prog_encode(prog, &instr);

	if( type == E_FORALL )
		instr.op = OP_AND;
	else if( type == E_FORSOME )
		instr.op = OP_OR;
	else
		BRE_ASSERT(0);

	prog_encode(prog, &instr);

	instr.op = OP_PUTSP;
	instr.s[0] = -3;
	prog_encode(prog, &instr);

	if( type == E_FORALL ) {
		instr.op = OP_GETSP;
		instr.s[0] = -2;
		prog_encode(prog, &instr);

		instr.op = OP_BRANCH_FALSE;
		instr.l[0] = 0;
		instr.b[0] = 1;
		lc3 = prog_encode(prog, &instr);
	}

	instr.op = OP_GETSP;
	instr.s[0] = -4;
	prog_encode(prog, &instr);

	instr.op = OP_PUTSP;
	instr.s[0] = -4;
	prog_encode(prog, &instr);

	instr.op = OP_INCSP;
	instr.s[0] = -4;
	prog_encode(prog, &instr);

	instr.op = OP_GETSP;
	instr.s[0] = -4;
	prog_encode(prog, &instr);

	instr.op = OP_GETSP;
	instr.s[0] = -2;
	prog_encode(prog, &instr);

	instr.op = OP_CMP_I;
	prog_encode(prog, &instr);

	instr.op = OP_GE;
	prog_encode(prog, &instr);

	instr.op = OP_BRANCH_TRUE;
	instr.l[0] = 0;
	instr.b[0] = 1;
	lc2 = prog_encode(prog, &instr);

	instr.op = OP_GETSP;
	instr.s[0] = -5;
	prog_encode(prog, &instr);

	instr.op = OP_GETSP;
	instr.s[0] = -5;
	prog_encode(prog, &instr);

	instr.op = OP_SET_ROW;
	prog_encode(prog, &instr);

	instr.op = OP_GETSP;
	instr.s[0] = -6;
	prog_encode(prog, &instr);

	instr.op = OP_GETSP;
	instr.s[0] = -5;
	prog_encode(prog, &instr);

	instr.op = OP_SET_ROW;
	prog_encode(prog, &instr);

	instr.op = OP_CALL;
	instr.l[0] = entry_point;
	prog_encode(prog, &instr);

	instr.op = OP_BRANCH_TRUE;
	instr.l[0] = lc_loop;
	instr.b[0] = 1;
	prog_encode(prog, &instr);

	if( type == E_FORSOME ) {
		instr.op = OP_GETSP;
		instr.s[0] = -2;
		prog_encode(prog, &instr);

		instr.op = OP_BRANCH_FALSE;
		instr.l[0] = 0;
		instr.b[0] = 1;
		lc4 = prog_encode(prog, &instr);
	}

	instr.op = OP_BRANCH;
	instr.l[0] = lc_next;
	prog_encode(prog, &instr);

/* OUT: */
	instr.op = OP_GETSP;
	instr.s[0] = -2;
	lc_out = prog_encode(prog, &instr);

	instr.op = OP_PUTSP;
	instr.s[0] = -6;
	prog_encode(prog, &instr);

	instr.op = OP_POP;
	prog_encode(prog, &instr);
	prog_encode(prog, &instr);
	prog_encode(prog, &instr);
	prog_encode(prog, &instr);
	
/* ---- PATCH INSTRUCTIONS ------ */

	instr.op = OP_BRANCH_ZERO;
	instr.l[0] = lc_out;
	instr.b[0] = 0;
	prog_patch_instruction(prog, lc1, &instr);

	instr.op = OP_BRANCH_TRUE;
	instr.l[0] = lc_out;
	instr.b[0] = 1;
	prog_patch_instruction(prog, lc2, &instr);

	if( type == E_FORALL ) {
		instr.op = OP_BRANCH_FALSE;
		instr.l[0] = lc_out;
		instr.b[0] = 1;
		prog_patch_instruction(prog, lc3, &instr);
	}

	if( type == E_FORSOME ) {
		instr.op = OP_BRANCH_FALSE;
		instr.l[0] = lc_out;
		instr.b[0] = 1;
		prog_patch_instruction(prog, lc4, &instr);
	}
	BRE_FREE(expr->left->left->right);
	BRE_FREE(expr->left->left);
	BRE_FREE(expr->left);
}

/*----------------------------------------------------------------------
 * Generate code for a forall and forsome loop.
 *
 */
static void gencode_for(GENCODE_MODE mode, PROGRAM *prog, EXPR *expr)
{
	if( expr->left->right->op == E_GROUP ) {
		gencode_for_group(mode, prog, expr);

	} else if( expr->left->right->type.primary == T_LIST ) {
		gencode_for_list(mode, prog, expr);

	} else if( expr->left->right->type.primary == T_STRING ) {
		gencode_for_string(mode, prog, expr);

	} else if( expr->left->right->type.primary == T_ROW ) {
		gencode_for_table(mode, prog, expr);

	} else {
		BRE_ASSERT(0);
	}
}

/*----------------------------------------------------------------------
 * Generate byte codes for access to tables.
 * The language construct compiled by this function are:
 *		TABLE
 *		TABLE[x]
 *
 */
static void gencode_table_index(GENCODE_MODE mode, PROGRAM *prog, EXPR *expr)
{
	PROGRAM_INSTRUCTION instr;
	EXPR *index_expr;

	instr.op	= OP_PUSH_TR;
	instr.l[0]	= 0;
	instr.ptr[0]	= expr->symbol;
	prog_encode(prog, &instr);

	if( expr->right ) {
		index_expr = expr->right->left;
		gencode(mode, prog, index_expr);
		instr.op = OP_SET_ROW_VERIFY;
		prog_encode(prog, &instr);
	}
}

/*----------------------------------------------------------------------
 * Generate byte codes for indexing of a list.
 * The language construct compiled by this function are:
 *
 *		LIST[x]
 *
 */
static void gencode_list_index(GENCODE_MODE mode, PROGRAM *prog, EXPR *expr)
{
	PROGRAM_INSTRUCTION instr;
	EXPR *index_expr;

	gencode(mode, prog, expr->left);
	gencode(mode, prog, expr->right->left);

	if( expr->type.primary == T_BOOLEAN || expr->type.primary == T_INTEGER )
		instr.op = OP_FETCH_LIST_I;
	else if( expr->type.primary == T_FLOAT )
		instr.op = OP_FETCH_LIST_F;
	else if( expr->type.primary == T_STRING )
		instr.op = OP_FETCH_LIST_S;
	else
		BRE_ASSERT(0);

	prog_encode(prog, &instr);

	BRE_FREE(expr->right);
}

/*----------------------------------------------------------------------
 * Generate byte codes for when a group object appears
 * in the source code. The generated byte codes
 * push's a table row pointer to the 0th row on the
 * stack.
 *
 * The language construct compiled by this function are:
 *		GROUP-NAME
 *
 */
static void gencode_group_name(GENCODE_MODE mode, PROGRAM *prog, EXPR *expr)
{
	PROGRAM_INSTRUCTION instr;

	instr.op	= OP_PUSH_TR;
	instr.l[0]	= 0;
	instr.ptr[0]	= expr->symbol->u.grp.using;
	prog_encode(prog, &instr);
}

/*----------------------------------------------------------------------
 * Generate byte codes for a compute loop when the iterator
 * consists of a GROUP object.
 *
 * for x, y in group-name compute <grp-expr>
 *	where <body-expr>
 *
 */
static void gencode_compute_group(GENCODE_MODE mode, PROGRAM *prog, EXPR *expr)
{
	PROGRAM_INSTRUCTION instr;
	EXPR *grpfunc, *arg, *vars, *iterator, *body;
	SYMBOL *sym1, *sym2;
	int lc1, lc2, lc3, lc4;
	int lc_next, lc_loop, lc_false, lc_skip, lc_out;
	int entry_point;

	grpfunc		= expr->left;
	arg		= expr->left->right;
	vars		= expr->right->left->left;
	iterator	= expr->right->left->right;
	body		= expr->right->right;

	entry_point = iterator->symbol->u.grp.entry_point;

	sym1 = vars->symbol;
	sym2 = vars->right->symbol;

	gencode(mode, prog, iterator);

	instr.op = OP_PUSH_I;
	instr.l[0] = 0;
	prog_encode(prog, &instr);

	instr.op = OP_NULL_I;
	prog_encode(prog, &instr);

	instr.op = OP_NULL_L;
	prog_encode(prog, &instr);

	instr.op = OP_GETSP;
	instr.s[0] = -4;
	prog_encode(prog, &instr);

	instr.op = OP_COUNT_TR;
	prog_encode(prog, &instr);

	instr.op = OP_BRANCH_ZERO;
	instr.l[0] = 0;
	instr.b[0] = 0;
	lc1 = prog_encode(prog, &instr);

/* NEXT: */
	instr.op = OP_GETSP;
	instr.s[0] = -5;
	lc_next = prog_encode(prog, &instr);

	instr.op = OP_GETSP;
	instr.s[0] = -5;
	prog_encode(prog, &instr);

	instr.op = OP_SET_ROW;
	prog_encode(prog, &instr);

	instr.op = OP_ASSIGN;
	instr.b[0] = 1;
	instr.s[0] = calculate_offset(sym1->kind, sym1->u.var.fp);
	prog_encode(prog, &instr);

	if( expr->type.u.subtype == T_STRING ) {
		instr.op = OP_PUSH_S;
		instr.str[0] = group_start_string(grpfunc->op);
		instr.s[0] = 1;
	} else if( expr->type.u.subtype == T_INTEGER ) {
		instr.op = OP_PUSH_I;
		instr.l[0] = group_start_integer(grpfunc->op);
	} else if( expr->type.u.subtype == T_FLOAT ) {
		instr.op = OP_PUSH_F;
		instr.d[0] = group_start_float(grpfunc->op);
	} else {
		BRE_ASSERT(0);
	}
	prog_encode(prog, &instr);

/* LOOP */
	instr.op = OP_GETSP;
	instr.s[0] = -6;
	lc_loop = prog_encode(prog, &instr);

	instr.op = OP_GETSP;
	instr.s[0] = -6;
	prog_encode(prog, &instr);

	instr.op = OP_SET_ROW;
	prog_encode(prog, &instr);

	instr.op = OP_ASSIGN;
	instr.b[0] = 1;
	instr.s[0] = calculate_offset(sym2->kind, sym2->u.var.fp);
	prog_encode(prog, &instr);

	gencode(mode, prog, body);

	instr.op = OP_BRANCH_FALSE;
	instr.l[0] = 0;
	instr.b[0] = 1;
	lc4 = prog_encode(prog, &instr);

	if( arg ) {
		gencode(mode, prog, arg);
	} else {
		/* the COUNT function will just push 1 on stack */
		instr.op = OP_PUSH_I;
		instr.l[0] = 1;
		prog_encode(prog, &instr);
	}

	instr.op = OP_GETSP;
	instr.s[0] = -2;
	prog_encode(prog, &instr);

	if( grpfunc->op == E_MIN )
		instr.op = opcode_type_select(grpfunc->type, 0, OP_MIN_I, OP_MIN_F, OP_MIN_S);
	else if( grpfunc->op == E_MAX )
		instr.op = opcode_type_select(grpfunc->type, 0, OP_MAX_I, OP_MAX_F, OP_MAX_S);
	else if( grpfunc->op == E_SUM )
		instr.op = opcode_type_select(grpfunc->type, 0, OP_ADD_I, OP_ADD_F, 0);
	else if( grpfunc->op == E_COUNT )
		instr.op = opcode_type_select(grpfunc->type, 0, OP_ADD_I, OP_ADD_F, 0);
	else
		BRE_ASSERT(0);

	prog_encode(prog, &instr);

	instr.op = OP_PUTSP;
	instr.s[0] = -2;
	prog_encode(prog, &instr);

	instr.op = OP_GETSP;
	instr.s[0] = -5;
	lc_false = prog_encode(prog, &instr);

	instr.op = OP_PUTSP;
	instr.s[0] = -5;
	prog_encode(prog, &instr);

	instr.op = OP_INCSP;
	instr.s[0] = -5;
	prog_encode(prog, &instr);

	instr.op = OP_GETSP;
	instr.s[0] = -5;
	prog_encode(prog, &instr);

	instr.op = OP_GETSP;
	instr.s[0] = -3;
	prog_encode(prog, &instr);

	instr.op = OP_CMP_I;
	prog_encode(prog, &instr);

	instr.op = OP_GE;
	prog_encode(prog, &instr);

	instr.op = OP_BRANCH_FALSE;
	instr.l[0] = 0;
	instr.b[0] = 1;
	lc2 = prog_encode(prog, &instr);

	instr.op = OP_GETSP;
	instr.s[0] = -3;
	prog_encode(prog, &instr);

	instr.op = OP_GETSP;
	instr.s[0] = -2;
	prog_encode(prog, &instr);

	instr.op = OP_APPEND;
	prog_encode(prog, &instr);

	instr.op = OP_PUTSP;
	instr.s[0] = -4;
	prog_encode(prog, &instr);

	instr.op = OP_POP;
	prog_encode(prog, &instr);

	instr.op = OP_BRANCH;
	instr.l[0] = 0;
	lc3 = prog_encode(prog, &instr);

/* skip: */
	instr.op = OP_GETSP;
	instr.s[0] = -6;
	lc_skip = prog_encode(prog, &instr);

	instr.op = OP_GETSP;
	instr.s[0] = -6;
	prog_encode(prog, &instr);

	instr.op = OP_SET_ROW;
	prog_encode(prog, &instr);

	instr.op = OP_GETSP;
	instr.s[0] = -7;
	prog_encode(prog, &instr);

	instr.op = OP_GETSP;
	instr.s[0] = -6;
	prog_encode(prog, &instr);

	instr.op = OP_SET_ROW;
	prog_encode(prog, &instr);

	instr.op = OP_CALL;
	instr.l[0] = entry_point;
	prog_encode(prog, &instr);

	instr.op = OP_BRANCH_TRUE;
	instr.l[0] = lc_loop;
	instr.b[0] = 1;
	prog_encode(prog, &instr);

	instr.op = OP_GETSP;
	instr.s[0] = -3;
	prog_encode(prog, &instr);

	instr.op = OP_GETSP;
	instr.s[0] = -2;
	prog_encode(prog, &instr);

	instr.op = OP_APPEND;
	prog_encode(prog, &instr);

	instr.op = OP_PUTSP;
	instr.s[0] = -4;
	prog_encode(prog, &instr);

	instr.op = OP_POP;
	prog_encode(prog, &instr);

	instr.op = OP_BRANCH;
	instr.l[0] = lc_next;
	prog_encode(prog, &instr);

/* OUT: */
	instr.op = OP_GETSP;
	instr.s[0] = -2;
	lc_out = prog_encode(prog, &instr);

	instr.op = OP_PUTSP;
	instr.s[0] = -6;
	prog_encode(prog, &instr);

	instr.op = OP_POP;
	prog_encode(prog, &instr);
	prog_encode(prog, &instr);
	prog_encode(prog, &instr);
	prog_encode(prog, &instr);
	
/* ---- patch instructions ------ */

	instr.op = OP_BRANCH_ZERO;
	instr.l[0] = lc_out;
	instr.b[0] = 0;
	prog_patch_instruction(prog, lc1, &instr);

	instr.op = OP_BRANCH_FALSE;
	instr.l[0] = lc_skip;
	instr.b[0] = 1;
	prog_patch_instruction(prog, lc2, &instr);

	instr.op = OP_BRANCH;
	instr.l[0] = lc_out;
	prog_patch_instruction(prog, lc3, &instr);

	instr.op = OP_BRANCH_FALSE;
	instr.l[0] = lc_false;
	instr.b[0] = 1;
	prog_patch_instruction(prog, lc4, &instr);

	BRE_FREE(expr->left);
	BRE_FREE(expr->right->left->right);
	BRE_FREE(expr->right->left->left->right);
	BRE_FREE(expr->right->left->left);
	BRE_FREE(expr->right->left);
	BRE_FREE(expr->right);
}

/*----------------------------------------------------------------------
 * Generate byte codes for a compute loop.
 *
 * for x, y, z in <iterator> compute <grp-expr>
 *	where <body-expr>
 *
 *
 */
static void gencode_compute(GENCODE_MODE mode, PROGRAM *prog, EXPR *expr)
{
	EXPR *grpfunc, *vars, *iterator, *body;
	EXPR *v, *nxt;
	PROGRAM_INSTRUCTION instr;
	SYMBOL *sym;
	EXPR_OPERATOR type;
	int lc1, lc2, lc3;

	grpfunc		= expr->left;
	vars		= expr->right->left->left;
	iterator	= expr->right->left->right;
	body		= expr->right->right;

	/*
	 * Handle more complex case involving a GROUP object.
	 */
	if( iterator->op == E_GROUP ) {
		gencode_compute_group(mode, prog, expr);
		return;
	}

	gencode(mode, prog, iterator);

	instr.op = OP_PUSH_I;
	instr.l[0] = 0;
	prog_encode(prog, &instr);


	if( grpfunc->type.primary == T_STRING ) {
		instr.op = OP_PUSH_S;
		instr.str[0] = group_start_string(grpfunc->op);
		instr.s[0] = 1;
	} else if( grpfunc->type.primary == T_INTEGER ) {
		instr.op = OP_PUSH_I;
		instr.l[0] = group_start_integer(grpfunc->op);
	} else if( grpfunc->type.primary == T_FLOAT ) {
		instr.op = OP_PUSH_F;
		instr.d[0] = group_start_float(grpfunc->op);
	} else {
		BRE_ASSERT(0);
	}
	prog_encode(prog, &instr);

/* LOOP BEGINS */
	instr.op = OP_GETSP;
	instr.s[0] = -3;
	lc1 = prog_encode(prog, &instr);

	if( iterator->type.primary == T_LIST )
		instr.op = OP_COUNT_L;
	else if( iterator->type.primary == T_STRING )
		instr.op = OP_COUNT_S;
	else if( iterator->type.primary == T_ROW )
		instr.op = OP_COUNT_TR;
	else
		BRE_ASSERT(0);

	prog_encode(prog, &instr);

	instr.op = OP_GETSP;
	instr.s[0] = -3;
	prog_encode(prog, &instr);

	instr.op = OP_SUB_I;
	prog_encode(prog, &instr);

	instr.op = OP_BRANCH_LT1;
	instr.l[0] = 0;
	instr.b[0] = 1;
	lc2 = prog_encode(prog, &instr);

	/*
	 * Assign the next element from
	 * the list to each variable.
	 */
	for(v=vars; v; v=nxt) {
		sym = v->symbol;

		instr.op = OP_GETSP;
		instr.s[0] = -3;
		prog_encode(prog, &instr);

		instr.op = OP_GETSP;
		instr.s[0] = -3;
		prog_encode(prog, &instr);

		if( iterator->type.primary == T_LIST ) {
			instr.op = opcode_type_select(sym->u.var.type, OP_FETCH_LIST_I, OP_FETCH_LIST_I,
					OP_FETCH_LIST_F, OP_FETCH_LIST_S);
			prog_encode(prog, &instr);
		} else if( iterator->type.primary == T_STRING ) {
			instr.op = OP_FETCH_CHAR;
			prog_encode(prog, &instr);
		} else if( iterator->type.primary == T_ROW ) {
			instr.op = OP_SET_ROW;
			prog_encode(prog, &instr);
		} else {
			BRE_ASSERT(0);
		}

		instr.op = OP_ASSIGN;
		instr.b[0] = 1;
		instr.s[0] = calculate_offset(sym->kind, sym->u.var.fp);
		prog_encode(prog, &instr);

		instr.op = OP_INCSP;
		instr.s[0] = -2;
		prog_encode(prog, &instr);

		nxt = v->right;
		BRE_FREE(v);
	}

	gencode(mode, prog, body);

	instr.op = OP_BRANCH_FALSE;
	instr.l[0] = lc1;
	instr.b[0] = 1;
	prog_encode(prog, &instr);

	if( grpfunc->right ) {
		gencode(mode, prog, grpfunc->right);
	} else {
		/* the COUNT function will just push 1 on stack */
		instr.op = OP_PUSH_I;
		instr.l[0] = 1;
		prog_encode(prog, &instr);
	}

	instr.op = OP_GETSP;
	instr.s[0] = -2;
	prog_encode(prog, &instr);

	if( grpfunc->op == E_MIN )
		instr.op = opcode_type_select(grpfunc->type, 0, OP_MIN_I, OP_MIN_F, OP_MIN_S);
	else if( grpfunc->op == E_MAX )
		instr.op = opcode_type_select(grpfunc->type, 0, OP_MAX_I, OP_MAX_F, OP_MAX_S);
	else if( grpfunc->op == E_SUM )
		instr.op = opcode_type_select(grpfunc->type, 0, OP_ADD_I, OP_ADD_F, 0);
	else if( grpfunc->op == E_COUNT )
		instr.op = opcode_type_select(grpfunc->type, 0, OP_ADD_I, OP_ADD_F, 0);
	else
		BRE_ASSERT(0);

	prog_encode(prog, &instr);

	instr.op = OP_PUTSP;
	instr.s[0] = -2;
	prog_encode(prog, &instr);

	instr.op = OP_BRANCH;
	instr.l[0] = lc1;
	prog_encode(prog, &instr);

/* END LOOP */
	instr.op = OP_PUTSP;
	instr.s[0] = -3;
	lc3 = prog_encode(prog, &instr);

	instr.op = OP_POP;
	prog_encode(prog, &instr);

	instr.op = OP_BRANCH_LT1;
	instr.l[0] = lc3;
	instr.b[0] = 1;
	prog_patch_instruction(prog, lc2, &instr);

	BRE_FREE(expr->left);
	BRE_FREE(expr->right->left);
	BRE_FREE(expr->right);
}

/*----------------------------------------------------------------------
 * Generate byte codes for the like operator.
 *
 */
static void gencode_like(GENCODE_MODE mode, PROGRAM *prog, EXPR *expr)
{
	PROGRAM_INSTRUCTION instr;

	gencode(mode, prog, expr->left);
	gencode(mode, prog, expr->right);

	instr.op = OP_LIKE;
	prog_encode(prog, &instr);
}

/*----------------------------------------------------------------------
 * Generate code for the ROWNUM() function
 *
 */
static void gencode_rownum(GENCODE_MODE mode, PROGRAM *prog, EXPR *expr)
{
	PROGRAM_INSTRUCTION instr;

	gencode(mode, prog, expr->right);

	instr.op = OP_CONV_TRI;
	prog_encode(prog, &instr);
}

/*----------------------------------------------------------------------
 * This function takes an expression parse tree and compiles
 * the byte codes into the program 'prog'.
 *
 * The parse tree 'expr' is free'd.
 *
 *
 */
#define EXPR_IS_LEAF(e)		((e)->left == NULL && (e)->right == NULL)
#define MIXED_LIST(e)		((e)->op == E_LIST && (e)->type.u.subtype == T_MIXED)

void gencode(GENCODE_MODE input_mode, PROGRAM *prog, EXPR *expr)
{
	PROGRAM_INSTRUCTION instr;
	GENCODE_MODE mode;
	int const_lc;
	int constant_folding;
	BRE_DATUM datum;

	/*
	 * See if we can perform constant folding. We can constant
	 * fold if:
	 *	a. The incomming 'expr' is constant.
	 *	b. The input_mode is G_GENCODE.
	 *	c. The parse tree 'expr' is not a leaf.
	 *	d. Not a MIXED list
	 */
	if( expr->constant == 1 && input_mode == G_GENCODE
				&& !EXPR_IS_LEAF(expr) && !MIXED_LIST(expr) ) {
		const_lc = prog->lc;
		mode = G_CONSTFOLD;
		constant_folding = 1;
	} else {
		mode = input_mode;
		constant_folding = 0;
	}

	switch( expr->op ) {
	case E_NONE:
		BRE_ASSERT(0);

	case E_FORALL:
		gencode_for(mode, prog, expr);
		break;

	case E_FORSOME:
		gencode_for(mode, prog, expr);
		break;

	case E_COMPUTE:
		gencode_compute(mode, prog, expr);
		break;

	case E_IF:
		gencode_if(mode, prog, expr);
		break;

	case E_IFELSE:
		gencode_ifelse(mode, prog, expr);
		break;

	case E_OR:
		gencode_logical(mode, prog, expr, OP_BRANCH_TRUE);
		break;

	case E_AND:
		gencode_logical(mode, prog, expr, OP_BRANCH_FALSE);
		break;

	case E_IN:
		gencode_in(mode, prog, expr);
		break;

	case E_BETWEEN:
		gencode_between(mode, prog, expr);
		break;

	case E_LIKE:
		gencode_like(mode, prog, expr);
		break;

	case E_NIN:
		gencode_in(mode, prog, expr);
		instr.op = OP_NOT;
		prog_encode(prog, &instr);
		break;

	case E_NBETWEEN:
		gencode_between(mode, prog, expr);
		instr.op = OP_NOT;
		prog_encode(prog, &instr);
		break;

	case E_NLIKE:
		gencode_like(mode, prog, expr);
		instr.op = OP_NOT;
		prog_encode(prog, &instr);
		break;

	case E_EQ:
		gencode_equality(mode, prog, expr);
		break;

	case E_NE:
		gencode_equality(mode, prog, expr);
		instr.op = OP_NOT;
		prog_encode(prog, &instr);
		break;

	case E_LT:
		gencode_relational(mode, prog, expr, OP_LT);
		break;

	case E_LE:
		gencode_relational(mode, prog, expr, OP_LE);
		break;

	case E_GT:
		gencode_relational(mode, prog, expr, OP_GT);
		break;

	case E_GE:
		gencode_relational(mode, prog, expr, OP_GE);
		break;

	case E_CONCAT:
		gencode(mode, prog, expr->left);
		gencode(mode, prog, expr->right);
		instr.op = OP_ADD_S;
		prog_encode(prog, &instr);
		break;

	case E_ADD:
		gencode_arithmetic(mode, prog, expr, OP_ADD_I, OP_ADD_F);
		break;

	case E_ADD_LIST:
		gencode_add_list(mode, prog, expr);
		break;

	case E_SUB:
		gencode_arithmetic(mode, prog, expr, OP_SUB_I, OP_SUB_F);
		break;

	case E_MUL:
		gencode_arithmetic(mode, prog, expr, OP_MUL_I, OP_MUL_F);
		break;

	case E_DIV:
		gencode_arithmetic(mode, prog, expr, OP_DIV_I, OP_DIV_F);
		break;

	case E_MOD:
		gencode_arithmetic(mode, prog, expr, OP_MOD_I, OP_MOD_I);
		break;

	case E_NOT:
		gencode(mode, prog, expr->right);
		instr.op = OP_NOT;

		prog_encode(prog, &instr);
		break;

	case E_NEG:
		gencode(mode, prog, expr->right);
		if( expr->type.primary == T_FLOAT )
			instr.op = OP_NEG_F;
		else
			instr.op = OP_NEG_I;
		prog_encode(prog, &instr);
		break;

	case E_COUNT:
		gencode_count(mode, prog, expr);
		break;

	case E_SUM:
		gencode_grpfunc(mode, prog, expr, 0, OP_ADD_I, OP_ADD_F);
		break;

	case E_MIN:
		gencode_grpfunc(mode, prog, expr, OP_MIN_S, OP_MIN_I, OP_MIN_F);
		break;

	case E_MAX:
		gencode_grpfunc(mode, prog, expr, OP_MAX_S, OP_MAX_I, OP_MAX_F);
		break;

	case E_CALL:
		gencode_call(mode, prog, expr);
		break;

	case E_INTEGER:
		instr.op = OP_PUSH_I;
		instr.l[0] = expr->value.i.val;
		prog_encode(prog, &instr);
		break;

	case E_FLOAT:
		instr.op = OP_PUSH_F;
		instr.d[0] = expr->value.f.val;
		prog_encode(prog, &instr);
		break;

	case E_STRING:
		instr.op = OP_PUSH_S;
		instr.str[0] = expr->value.s.str;
		instr.s[0] = expr->value.s.len;
		prog_encode(prog, &instr);
		BRE_FREE(expr->value.s.str);
		break;

	case E_BOOLEAN:
		instr.op = OP_PUSH_I;
		instr.l[0] = expr->value.i.val;
		prog_encode(prog, &instr);
		break;

	case E_NULL:
		gencode_null(mode, prog, expr);
		break;

	case E_FIELD:
		BRE_ASSERT(0);

	case E_OLDFIELD:
		BRE_ASSERT(0);

	case E_LUFIELD:
		BRE_ASSERT(0);

	case E_VARIABLE:
		gencode_variable(mode, prog, expr);
		break;

	case E_SUBSTR:
		gencode_substr(mode, prog, expr);
		break;

	case E_SUBLST:
		gencode_sublst(mode, prog, expr);
		break;

	case E_LOOKUP_INDEX:
		gencode_lookup_index(mode, prog, expr);
		break;

	case E_TABLE_INDEX:
		gencode_table_index(mode, prog, expr);
		break;

	case E_LIST_INDEX:
		gencode_list_index(mode, prog, expr);
		break;

	case E_LIST:
		gencode_list(mode, prog, expr);
		break;

	case E_WARN:
		BRE_ASSERT(0);
		break;

	case E_FAIL:
		BRE_ASSERT(0);
		break;

	case E_ASSIGN:
		gencode_assignment(mode, prog, expr);
		break;

	case E_BLOCK:
		gencode_block(mode, prog, expr);
		break;

	case E_CONSTANT:
		gencode_constant(mode, prog, expr);
		break;

	case E_GROUP:
		gencode_group_name(mode, prog, expr);
		break;

	case E_DOT:
		gencode_dot(mode, prog, expr);
		break;

	case E_IMPLICIT:
		instr.op = OP_GETFP;
		instr.s[0] = calculate_offset(S_PARAMETER, 0);
		prog_encode(prog, &instr);
		break;

	case E_ROWNUM:
		gencode_rownum(mode, prog, expr);
		break;

	default:
		BRE_ASSERT(0);
	}

	if( constant_folding ) {
		/*
		 * Evaluate the constant, and then
		 * set the location counter to before
		 * the constant.
		 */
		instr.op = OP_END_CONST;
		prog_encode(prog, &instr);
		run_evaluate_constant(prog, const_lc, &datum);
		prog->lc = const_lc;
		gencode_bre_datum(input_mode, prog, expr, &datum);
	}

	BRE_FREE(expr);
}

/*----------------------------------------------------------------------
 * Generate instruction for the entrance into a
 * function call. Return the location of this instruction.
 *
 */
int gencode_function_entry(PROGRAM *prog)
{
	int lc;
	PROGRAM_INSTRUCTION instr;

	instr.op	= OP_ENTRY;
	instr.s[0]	= 0;

	lc = prog_encode(prog, &instr);

	return lc;
}

/*----------------------------------------------------------------------
 * Generate instruction for a function call return.
 * Also patch the entry_point instruction for the
 * number of local variables in the function.
 *
 */
void gencode_function_exit(PROGRAM *prog, int entry_point, int nlocals, int nparms)
{
	PROGRAM_INSTRUCTION instr;

	instr.op	= OP_ENTRY;
	instr.s[0]	= nlocals;
	prog_patch_instruction(prog, entry_point, &instr);

	instr.op	= OP_RETURN;
	instr.s[0]	= nparms;
	prog_encode(prog, &instr);
}

/*----------------------------------------------------------------------
 * Generate instruction for the entrance into a rule.
 * Return the location of this instruction.
 *
 */
int gencode_rule_entry(PROGRAM *prog)
{
	int lc;
	PROGRAM_INSTRUCTION instr;

	instr.op	= OP_ENTRY;
	instr.s[0]	= 0;

	lc = prog_encode(prog, &instr);

	return lc;
}

/*----------------------------------------------------------------------
 * Generate code for the 'warn with' and 'fail with' clauses.
 *
 */
static void gencode_error_clause(PROGRAM *prog, char *rule_id, EXPR *expr)
{
	PROGRAM_INSTRUCTION instr;
	EXPR *curr, *arg, *nxt;
	EXPR_OPERATOR op;
	OPCODE opcode;
	int lc1, lc2;
	int count, idx;
	char name[ 200 ], prefix[ 200 ];

	instr.op = OP_CHECK_RULE;
	instr.l[0] = 0;
	lc1 = prog_encode(prog, &instr);

	if( expr ) {
		op = expr->op;
		count = 0;
		for(curr=expr->right; curr; curr=nxt) {
			arg = curr->left;

			gencode(G_GENCODE, prog, arg);

			if( arg->type.primary != T_STRING ) {
				if( arg->type.primary == T_BOOLEAN )
					opcode = OP_CONV_BS;
				else if( arg->type.primary == T_INTEGER )
					opcode = OP_CONV_IS;
				else if( arg->type.primary == T_FLOAT )
					opcode = OP_CONV_FS;
				else
					BRE_ASSERT(0);

				instr.op = opcode;
				prog_encode(prog, &instr);
			}
			count += 1;
			nxt = curr->right;
			BRE_FREE(curr);
		}
		BRE_FREE(expr);
	} else {
		op = E_FAIL;
		count = 0;
	}

	instr.op = OP_MKLIST;
	instr.s[0] = count;
	prog_encode(prog, &instr);

	sym_extract_pair(rule_id, prefix, name);
	idx = atoi(prefix);

	instr.op	= OP_ERROR;
	instr.b[0]	= (op == E_WARN) ? BRE_WARN : BRE_FAIL;
	instr.s[0]	= idx;
	instr.str[0]	= name;
	prog_encode(prog, &instr);

	lc2 = prog_get_lc(prog);
	instr.op = OP_CHECK_RULE;
	instr.l[0] = lc2;
	prog_patch_instruction(prog, lc1, &instr);
}

/*----------------------------------------------------------------------
 * Generate instruction for the ending of a rule.
 * Also patch the entry_point instruction for the
 * number of local variables in the function.
 *
 */
void gencode_rule_exit(PROGRAM *prog, SYMBOL *sym, EXPR *error_clause)
{
	PROGRAM_INSTRUCTION instr;

	gencode_error_clause(prog, sym->name, error_clause);

	instr.op	= OP_ENTRY;
	instr.s[0]	= sym->u.rul.nlocals;
	prog_patch_instruction(prog, sym->u.rul.entry_point, &instr);

	instr.op	= OP_END_RULE;
	instr.s[0]	= sym->u.rul.nlocals;
	prog_encode(prog, &instr);
}
