/*** $Id: bre_run.c,v 1.25 1998/01/12 12:27:35 stauffer Exp stauffer $ ***/

/*
 */
#ifndef lint
static char rcsid[] = "@(#)$Id: bre_run.c,v 1.25 1998/01/12 12:27:35 stauffer Exp stauffer $";
#endif

/***********************************************************************
 * BRE Byte-Code execution module
 */

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#include "bre_api.h"
#include "bre_prv.h"

/*
 * A null value for double's is this value here:
 */
#define FLOAT_NIL	(9.9e27)

/*----------------------------------------------------------------------
 *	COMPILE	ROUTINE			EXTRACT ROUTINE
 *	------------------------	--------------------
 *	prog_new()
 *	prog_free()
 *	prog_get_lc()
 *	prog_set_lc()
 *	prog_compile_opcode()		prog_get_opcode()
 *	prog_compile_byte()		prog_get_byte()
 *	prog_compile_short()		prog_get_short()
 *	prog_compile_long()		prog_get_long()
 *	prog_compile_double()		prog_get_double()
 *	prog_compile_string()		prog_get_string()
 *	prog_compile_datum()		prog_get_datum()
 *	prog_compile_voidptr()		prog_get_voidptr()
 *	prog_encode()
 *	prog_decode()
 *	prog_patch_instruction()
 *
 * The COMPILE routines will insert a new data item into the
 * program and position the internal location counter 'lc' to
 * the next available spot. The EXTRACT routines do the opposite.
 * They fetch the next data item out of the program and position
 * the internal location counter 'lc' to the next data item.
 *
 */

/*----------------------------------------------------------------------
 * Create a new program structurre.
 */
PROGRAM *prog_new(void)
{
	PROGRAM *prog;
	unsigned char *program;

	prog = (PROGRAM*)BRE_MALLOC( sizeof(PROGRAM) );
	if( prog == NULL )
		Bre_Error(BREMSG_DIE_NOMEM);

	program = (unsigned char*)BRE_MALLOC( PROGRAM_GROW );
	if( program == NULL )
		Bre_Error(BREMSG_DIE_NOMEM);

	prog->lc	= 0;
	prog->size	= PROGRAM_GROW;
	prog->program	= program;

	return prog;
}

/*----------------------------------------------------------------------
 * Free all memory associated with a program.
 *
 * Go through the byte code program and free all the lists
 * that are part of the OP_PUSH_L (Push List) instructions.
 *
 * RETURNS:
 *	Returns the total number of bytes used by the byte-code
 *	program.
 *
 */
int prog_free(PROGRAM *prog)
{
	PROGRAM_INSTRUCTION instr;
	int end_this, i;
	BRE_LIST list;
	int list_bytes, total_bytes;

	prog_set_lc(prog, 0);

	list_bytes = 0;
	end_this = 0;
	while( !end_this ) {
		prog_decode(prog, &instr);
		if( instr.op == OP_NOMORE )
			break;
		else if( instr.op != OP_PUSH_L )
			continue;

		list = instr.datum.l;
		list_bytes += list.len * sizeof(BRE_DATUM);

		if( instr.b[0] == 'S' ) {
			for(i=0; i<list.len; i++) {
				if( list.list[i].s.str ) {
					list_bytes += list.list[i].s.len;
					BRE_FREE(list.list[i].s.str);
				}
			}
		}
		if( list.list )
			BRE_FREE(list.list);
	}
	total_bytes = list_bytes + prog->lc;

	BRE_FREE(prog->program);
	BRE_FREE(prog);

	return total_bytes;
}

/*----------------------------------------------------------------------
 * This functions checks that 'n' additional bytes
 * will be able to fit into the program buffer.
 *
 * If the buffer size, cannot hold another 'n' bytes then
 * this function will increase the size of the buffer to
 * hold more data.
 *
 * PROGRAM_GROW is the amount to increase the
 * buffer by. If 'n' is larger than PROGRAM_GROW
 * the buffer is increased by '(n + PROGRAM_GROW)'
 *
 */
static void prog_grow(PROGRAM *prog, int n)
{
	unsigned int new_size, grow_amount;
	unsigned char *newp;

	if( prog->lc + n >= prog->size ) {
		if( n < PROGRAM_GROW )
			grow_amount = PROGRAM_GROW;
		else
			grow_amount = n + PROGRAM_GROW;

		new_size = prog->size + grow_amount;

		newp = (unsigned char*)BRE_REALLOC(prog->program, new_size);
		if( newp == NULL ) {
			Bre_Error(BREMSG_DIE_REALLOC);
		}

		prog->program	= newp;
		prog->size	= new_size;
	}
}

/*----------------------------------------------------------------------
 * Return the current location counter.
 */
int prog_get_lc(PROGRAM *prog)
{
	return (int)prog->lc;
}

/*----------------------------------------------------------------------
 * Set the location counter to 'lc'.
 */
void prog_set_lc(PROGRAM *prog, int lc)
{
	prog->lc = (unsigned int)lc;
}

static void prog_compile_byte(PROGRAM *prog, unsigned char c)
{
	prog_grow(prog, 1);
	prog->program[prog->lc] = c;
	prog->lc += 1;
}

static void prog_get_byte(PROGRAM *prog, unsigned char *c)
{
	*c = prog->program[prog->lc];
	prog->lc += 1;
}

static void prog_compile_opcode(PROGRAM *prog, OPCODE opcode)
{
	prog_compile_byte(prog, (unsigned char)opcode);
}

static void prog_get_opcode(PROGRAM *prog, OPCODE *opcode)
{
	unsigned char c;

	prog_get_byte(prog, &c);
	*opcode = (OPCODE)c;
}

static void prog_compile_short(PROGRAM *prog, short val)
{
	int pad, mod;

	mod = prog->lc%PAD_SHORT;
	pad = (mod) ? PAD_SHORT - mod : 0;

	prog_grow(prog, pad + sizeof(short));

	prog->lc += pad;

	*((short*)&prog->program[prog->lc]) = val;

	prog->lc += sizeof(short);
}

static void prog_get_short(PROGRAM *prog, short *val)
{
	int pad, mod;

	mod = prog->lc%PAD_SHORT;
	if( mod ) {
		pad = PAD_SHORT - mod;
		prog->lc += pad;
	}

	*val = *((short*)&prog->program[prog->lc]);

	prog->lc += sizeof(short);
}

static void prog_compile_long(PROGRAM *prog, long val)
{
	int pad, mod;

	mod = prog->lc%PAD_LONG;
	pad = (mod) ? PAD_LONG - mod : 0;

	prog_grow(prog, pad + sizeof(long));

	prog->lc += pad;

	*(long*)(&prog->program[prog->lc]) = val;

	prog->lc += sizeof(long);
}

static void prog_get_long(PROGRAM *prog, long *val)
{
	int pad, mod;

	mod = prog->lc%PAD_LONG;
	if( mod ) {
		pad = PAD_LONG - mod;
		prog->lc += pad;
	}

	*val = *((long*)&prog->program[prog->lc]);

	prog->lc += sizeof(long);
}

static void prog_compile_double(PROGRAM *prog, double val)
{
	int pad, mod;

	mod = prog->lc%PAD_DOUBLE;
	pad = (mod) ? PAD_DOUBLE - mod : 0;

	prog_grow(prog, pad + sizeof(double));

	prog->lc += pad;

	*((double*)&prog->program[prog->lc]) = val;

	prog->lc += sizeof(double);
}

static void prog_get_double(PROGRAM *prog, double *val)
{
	int pad, mod;

	mod = prog->lc%PAD_DOUBLE;
	if( mod ) {
		pad = PAD_DOUBLE - mod;
		prog->lc += pad;
	}

	*val = *((double*)&prog->program[prog->lc]);

	prog->lc += sizeof(double);
}

static void prog_compile_string(PROGRAM *prog, char *str)
{
	int len;

	len = strlen(str)+1;

	prog_grow(prog, len);

	memcpy((char*)&prog->program[prog->lc], str, len);

	prog->lc += len;
}

static void prog_get_string(PROGRAM *prog, char **string)
{
	char *str, *p;
	int pad, mod;

	str = (char*)&prog->program[prog->lc];
	p = str;

	while(*p != '\0') {
		p++;
	}

	prog->lc += (p-str)+1;

	*string = str;
}

static void prog_compile_datum(PROGRAM *prog, BRE_DATUM *datum)
{
	int pad, mod;

	mod = prog->lc%PAD_DATUM;
	pad = (mod) ? PAD_DATUM - mod : 0;

	prog_grow(prog, pad + sizeof(BRE_DATUM));

	prog->lc += pad;

	*((BRE_DATUM*)&prog->program[prog->lc]) = *datum;

	prog->lc += sizeof(BRE_DATUM);
}

static void prog_get_datum(PROGRAM *prog, BRE_DATUM *datum)
{
	BRE_DATUM val;
	int pad, mod;

	mod = prog->lc%PAD_DATUM;
	if( mod ) {
		pad = PAD_DATUM - mod;
		prog->lc += pad;
	}

	*datum = *((BRE_DATUM*)&prog->program[prog->lc]);

	prog->lc += sizeof(BRE_DATUM);
}

static void prog_compile_voidptr(PROGRAM *prog, void *voidptr)
{
	int pad, mod;

	mod = prog->lc%PAD_POINTER;
	pad = (mod) ? PAD_POINTER - mod : 0;

	prog_grow(prog, pad + sizeof(void*));

	prog->lc += pad;

	*(void**)(&prog->program[prog->lc]) = voidptr;

	prog->lc += sizeof(void*);
}

static void prog_get_voidptr(PROGRAM *prog, void **voidptr)
{
	BRE_DATUM val;
	int pad, mod;

	mod = prog->lc%PAD_POINTER;
	if( mod ) {
		pad = PAD_POINTER - mod;
		prog->lc += pad;
	}

	*voidptr = *(void**)(&prog->program[prog->lc]);

	prog->lc += sizeof(void*);
}

/*----------------------------------------------------------------------
 * This table defines each instruction and what operands it accepts.
 * The first column is the name of the instruction. The second column
 * is an array of OPERAND_TYPE's (up to 6 of them). This array
 * describes the operands that this instruction uses.
 *
 * When encoding (decoding) an instruction, the prog_encode (prog_decode)
 * routine will read from (write to) the PROGRAM_INSTRUCTION members.
 *
 */
typedef enum {			/* PROGRAM_INSTRUCTION member */
	OT_NONE,
	OT_BYTE,		/* instr->b[x]		*/
	OT_SHORT,		/* instr->s[x]		*/
	OT_LONG,		/* instr->l[x]		*/
	OT_DOUBLE,		/* instr->d[x]		*/
	OT_STRING,		/* instr->str[x]	*/
	OT_DATUM,		/* instr->datum		*/
	OT_VOIDPTR,		/* instr->ptr[x]	*/
} OPERAND_TYPE;

#define NOPERANDS	8

static struct {
	char		*opcode_name;
	OPERAND_TYPE	t[ NOPERANDS ];
} InstrDecode[] = {
"OP_PUSH_I",			{OT_LONG, OT_NONE},
"OP_PUSH_F",			{OT_DOUBLE, OT_NONE},
"OP_PUSH_S",			{OT_SHORT, OT_STRING, OT_NONE},
"OP_PUSH_L",			{OT_DATUM, OT_BYTE, OT_NONE},
"OP_PUSH_TR",			{OT_LONG, OT_VOIDPTR, OT_NONE},
"OP_NULL_I",			{OT_NONE},
"OP_NULL_F",			{OT_NONE},
"OP_NULL_S",			{OT_NONE},
"OP_NULL_TR",			{OT_VOIDPTR, OT_NONE},
"OP_NULL_L",			{OT_NONE},
"OP_POP",			{OT_NONE},
"OP_ADD_I",			{OT_NONE},
"OP_ADD_F",			{OT_NONE},
"OP_ADD_S",			{OT_NONE},
"OP_PREPEND",			{OT_NONE},
"OP_APPEND",			{OT_NONE},
"OP_SUB_I",			{OT_NONE},
"OP_SUB_F",			{OT_NONE},
"OP_MUL_I",			{OT_NONE},
"OP_MUL_F",			{OT_NONE},
"OP_DIV_I",			{OT_NONE},
"OP_DIV_F",			{OT_NONE},
"OP_MOD_I",			{OT_NONE},
"OP_NEG_I",			{OT_NONE},
"OP_NEG_F",			{OT_NONE},
"OP_CMP_I",			{OT_NONE},
"OP_CMP_F",			{OT_NONE},
"OP_CMP_S",			{OT_NONE},
"OP_CMP_TR",			{OT_NONE},
"OP_EQ",			{OT_NONE},
"OP_LT",			{OT_NONE},
"OP_LE",			{OT_NONE},
"OP_GT",			{OT_NONE},
"OP_GE",			{OT_NONE},
"OP_ISNULL_I",			{OT_NONE},
"OP_ISNULL_F",			{OT_NONE},
"OP_ISNULL_S",			{OT_NONE},
"OP_ISNULL_TR",			{OT_NONE},
"OP_ISNULL_L",			{OT_NONE},
"OP_NOT",			{OT_NONE},
"OP_IN_IL",			{OT_NONE},
"OP_IN_FL",			{OT_NONE},
"OP_IN_SL",			{OT_NONE},
"OP_IN_SS",			{OT_NONE},
"OP_BETWEEN_I",			{OT_NONE},
"OP_BETWEEN_F",			{OT_NONE},
"OP_BETWEEN_S",			{OT_NONE},
"OP_LIKE",			{OT_NONE},
"OP_BRANCH_TRUE",		{OT_BYTE, OT_LONG, OT_NONE},
"OP_BRANCH_FALSE",		{OT_BYTE, OT_LONG, OT_NONE},
"OP_BRANCH_LT1",		{OT_BYTE, OT_LONG, OT_NONE},
"OP_BRANCH_ZERO",		{OT_BYTE, OT_LONG, OT_NONE},
"OP_BRANCH",			{OT_LONG, OT_NONE},
"OP_CHECK_RULE",		{OT_LONG, OT_NONE},
"OP_CALL",			{OT_LONG, OT_NONE},
"OP_RETURN",			{OT_SHORT, OT_NONE},
"OP_ENTRY",			{OT_SHORT, OT_NONE},
"OP_END_RULE",			{OT_SHORT, OT_NONE},
"OP_END_CONST",			{OT_NONE},
"OP_ERROR",			{OT_BYTE, OT_SHORT, OT_STRING, OT_NONE},
"OP_NOMORE",			{OT_NONE},
"OP_EXT_FUNC",			{OT_SHORT, OT_SHORT, OT_STRING, OT_STRING, OT_STRING, OT_VOIDPTR, OT_VOIDPTR, OT_NONE},
"OP_EXT_LOOKUP",		{OT_SHORT, OT_SHORT, OT_STRING, OT_STRING, OT_STRING, OT_VOIDPTR, OT_VOIDPTR, OT_NONE},
"OP_MKLIST",			{OT_SHORT, OT_NONE},
"OP_ASSIGN",			{OT_BYTE, OT_SHORT, OT_NONE},
"OP_GETFP",			{OT_SHORT, OT_NONE},
"OP_GETSP",			{OT_SHORT, OT_NONE},
"OP_PUTSP",			{OT_SHORT, OT_NONE},
"OP_INCSP",			{OT_SHORT, OT_NONE},
"OP_DECSP",			{OT_SHORT, OT_NONE},
"OP_CONV_IF",			{OT_NONE},
"OP_CONV_FI",			{OT_NONE},
"OP_CONV_BS",			{OT_NONE},
"OP_CONV_IS",			{OT_NONE},
"OP_CONV_FS",			{OT_NONE},
"OP_CONV_TRI",			{OT_NONE},
"OP_SET_ROW",			{OT_NONE},
"OP_SET_ROW_VERIFY",		{OT_NONE},
"OP_FETCH_LIST_I",		{OT_NONE},
"OP_FETCH_LIST_F",		{OT_NONE},
"OP_FETCH_LIST_S",		{OT_NONE},
"OP_FETCH_FIELD",		{OT_VOIDPTR, OT_VOIDPTR, OT_BYTE, OT_BYTE, OT_NONE},
"OP_FETCH_CHAR",		{OT_NONE},
"OP_SUBSTR",			{OT_NONE},
"OP_SUBLST",			{OT_NONE},
"OP_MAX_I",			{OT_NONE},
"OP_MAX_F",			{OT_NONE},
"OP_MAX_S",			{OT_NONE},
"OP_MIN_I",			{OT_NONE},
"OP_MIN_F",			{OT_NONE},
"OP_MIN_S",			{OT_NONE},
"OP_COUNT_L",			{OT_NONE},
"OP_COUNT_S",			{OT_NONE},
"OP_COUNT_TR",			{OT_NONE},
"OP_AND",			{OT_NONE},
"OP_OR",			{OT_NONE},
};

/*----------------------------------------------------------------------
 * Decode an instruction and place
 * the data into 'instr'
 *
 */
void prog_decode(PROGRAM *prog, PROGRAM_INSTRUCTION *instr)
{
	OPCODE op;
	OPERAND_TYPE optype;
	int done, i, ib, is, il, id, istr, ip, isk;

	ib = 0; il = 0; id = 0; is = 0; istr = 0; ip = 0; isk = 0;

	prog_get_opcode(prog, &op);
	instr->op = op;
	instr->opcode_name = InstrDecode[op].opcode_name;

	done = 0;
	for(i=0; !done; i++) {
		optype = InstrDecode[op].t[i];

		switch( optype ) {
		case OT_NONE:
			done = 1;
			break;

		case OT_BYTE:
			prog_get_byte(prog, &instr->b[ib++]);
			break;

		case OT_SHORT:
			prog_get_short(prog, &instr->s[is++]);
			break;

		case OT_LONG:
			prog_get_long(prog, &instr->l[il++]);
			break;

		case OT_DOUBLE:
			prog_get_double(prog, &instr->d[id++]);
			break;

		case OT_STRING:
			prog_get_string(prog, &instr->str[istr++]);
			break;

		case OT_DATUM:
			prog_get_datum(prog, &instr->datum);
			break;

		case OT_VOIDPTR:
			prog_get_voidptr(prog, &instr->ptr[ip++]);
			break;

		default:
			BRE_ASSERT(0);
		}
	}
}

/*----------------------------------------------------------------------
 * Encode an instruction into 'prog' by looking
 * at the data in 'instr'.
 *
 * RETURNS:
 *	The location counter where the instruction
 *	was compile into.
 *
 */
int prog_encode(PROGRAM *prog, PROGRAM_INSTRUCTION *instr)
{
	OPCODE op;
	OPERAND_TYPE optype;
	int done, i, ib, is, il, id, istr, ip, isk;
	int ret_lc;

	ib = 0; il = 0; id = 0; is = 0; istr = 0; ip = 0; isk = 0;

	ret_lc = prog->lc;

	op = instr->op;
	prog_compile_opcode(prog, op);

	done = 0;
	for(i=0; !done; i++) {
		BRE_ASSERT( i < NOPERANDS );
		optype = InstrDecode[op].t[i];

		switch( optype ) {
		case OT_NONE:
			done = 1;
			break;

		case OT_BYTE:
			prog_compile_byte(prog, instr->b[ib++]);
			break;

		case OT_SHORT:
			prog_compile_short(prog, instr->s[is++]);
			break;

		case OT_LONG:
			prog_compile_long(prog, instr->l[il++]);
			break;

		case OT_DOUBLE:
			prog_compile_double(prog, instr->d[id++]);
			break;

		case OT_STRING:
			prog_compile_string(prog, instr->str[istr++]);
			break;

		case OT_DATUM:
			prog_compile_datum(prog, &instr->datum);
			break;

		case OT_VOIDPTR:
			prog_compile_voidptr(prog, instr->ptr[ip++]);
			break;

		default:
			BRE_ASSERT(0);
		}
	}

	return ret_lc;
}

void prog_patch_instruction(PROGRAM *prog, int patch_lc, PROGRAM_INSTRUCTION *instr)
{
	int save_lc;

	save_lc = prog->lc;
	prog->lc = patch_lc;
	prog_encode(prog, instr);
	prog->lc = save_lc;
}

/*----------------------------------------------------------------------
 * Used by prog_print() to format each instruction attributes.
 *
 */
static void format_operands(char *operand_str, PROGRAM_INSTRUCTION *instr)
{
	OPCODE op;
	OPERAND_TYPE optype;
	int done, i, ib, is, il, id, istr, ip;
	char buf[ 1024 ];

	operand_str[0] = '\0';

	ib = 0; il = 0; id = 0; is = 0; istr = 0; ip = 0;

	op = instr->op;
	done = 0;
	for(i=0; !done; i++) {
		optype = InstrDecode[op].t[i];

		switch( optype ) {
		case OT_NONE:
			buf[0] = '\0';
			done = 1;
			break;

		case OT_BYTE:
			sprintf(buf, "B%d=%d ", ib, instr->b[ib]);
			ib++;
			break;

		case OT_SHORT:
			sprintf(buf, "S%d=%hd ", is, instr->s[is]);
			is++;
			break;

		case OT_LONG:
			sprintf(buf, "L%d=%ld ", il, instr->l[il]);
			il++;
			break;

		case OT_DOUBLE:
			sprintf(buf, "D%d=%.4f ", id, instr->d[id]);
			id++;
			break;

		case OT_STRING:
			sprintf(buf, "STR%d=\"%s\" ", istr, instr->str[istr]);
			istr++;
			break;

		case OT_DATUM:
			sprintf(buf, "[datum] ");
			break;

		case OT_VOIDPTR:
			sprintf(buf, "PTR%d=%0x ", ip, instr->ptr[ip]);
			ip++;
			break;

		default:
			BRE_ASSERT(0);
		}

		strcat(operand_str, buf);
	}
}

static void format_comment_entry(int lc, PROGRAM_INSTRUCTION *instr, char *comment)
{
	SYMBOL *sym;
	SYMBOL_CURSOR sc;
	char prefix[ 200 ], name[ 200 ];
	int idx;
	char *filename;

	sym = sym_first(&sc);
	while( sym ) {
		if( sym->kind == S_RULE && sym->u.rul.entry_point == lc ) {
			sym_extract_pair(sym->name, prefix, name);
			idx = atoi(prefix);
			filename = BRE()->filenames[ idx-1 ];
			if( sym->u.rul.using )
				sprintf(comment, "----- RULE: %s using %s (file: %s) -----",
							name,
							(sym->u.rul.using)->name, filename);
			else
				sprintf(comment, "----- RULE: %s (file: %s) -----", name, filename);
			return;
		}

		sym = sym_next(&sc);
	}

	sprintf(comment, "----- function or constant -----");
}

static void format_comment_pushtr(int lc, PROGRAM_INSTRUCTION *instr, char *comment)
{
	SYMBOL *tbl;
	int row;

	tbl = instr->ptr[0];
	row = instr->l[0];

	sprintf(comment, "%s[%d]", tbl->name, row);
}

static void format_comment_fetchfield(int lc, PROGRAM_INSTRUCTION *instr, char *comment)
{
	SYMBOL *fld;
	char prefix[ 200 ], name[ 200 ];

	fld = instr->ptr[0];
	sym_extract_pair(fld->name, prefix, name);

	sprintf(comment, "%s%s.%s", (instr->b[0]==0) ? "old " : "", prefix, name);
}

static void format_comment_extlookup(int lc, PROGRAM_INSTRUCTION *instr, char *comment)
{
	BRE_EXTERNAL *curr;
	BRE_EXTERNAL_CB *cb;

	cb = (BRE_EXTERNAL_CB*)instr->ptr[0];

	for(curr=BRE()->externals; curr; curr=curr->next) {
		if( cb == curr->func ) {
			sprintf(comment, "call lookup %s", curr->name);
			return;
		}
	}

	BRE_ASSERT(0);
}

static void format_comment_extfunc(int lc, PROGRAM_INSTRUCTION *instr, char *comment)
{
	BRE_EXTERNAL *curr;
	BRE_EXTERNAL_CB *cb;

	cb = (BRE_EXTERNAL_CB*)instr->ptr[0];

	for(curr=BRE()->externals; curr; curr=curr->next) {
		if( cb == curr->func ) {
			sprintf(comment, "external function = %s", curr->name);
			return;
		}
	}

	BRE_ASSERT(0);
}

static void format_comment_nulltr(int lc, PROGRAM_INSTRUCTION *instr, char *comment)
{
	SYMBOL *table;

	table = instr->ptr[0];
	sprintf(comment, "%s", table->name);
}

static void format_comment_pushl(int lc, PROGRAM_INSTRUCTION *instr, char *comment)
{
	char buf[ 1024 ], type, *comma;
	BRE_LIST l;
	int i;

	type	= instr->b[0];
	l	= instr->datum.l;

	sprintf(comment, "{ ");

	for(i=0; i<l.len && l.list; i++) {
		comma = ((i+1) < l.len) ? "," : "";

		switch( type ) {
		case 'I':
			if( l.list[i].i.nil )
				sprintf(buf, "nil%s", comma);
			else
				sprintf(buf, "%d%s", l.list[i].i.val, comma);
			break;

		case 'F':
			if( l.list[i].f.val == FLOAT_NIL )
				sprintf(buf, "nil%s", comma);
			else
				sprintf(buf, "%.3f%s", l.list[i].f.val, comma);
			break;

		case 'S':
			if( l.list[i].s.str == NULL )
				sprintf(buf, "nil%s", comma);
			else
				sprintf(buf, "\"%s\"%s", l.list[i].s.str, comma);
			break;

		default:
			BRE_ASSERT(0);
		}

		strcat(comment, buf);

	}
	strcat(comment, " }");
}

/*----------------------------------------------------------------------
 * Output the program in an ascii format.
 * For some opcodes, print more information about the operands.
 * This function assumes that the symbols table still contains:
 * 	SYM_RULE, SYM_TABLE, and SYM_FIELD entries.
 *
 */
void prog_print(FILE *fp, PROGRAM *prog)
{
	PROGRAM_INSTRUCTION instr;
	int	lc;
	char	attrs[2000];
	char	comment[5000];
	int	end_this, newline;

	prog_set_lc(prog, 0);

	end_this = 0;
	while( !end_this ) {
		lc = prog_get_lc(prog);

		comment[0]	= '\0';
		newline		= 0;
		prog_decode(prog, &instr);
		switch( instr.op ) {
		case OP_NOMORE:
			end_this = 1;
			break;

		case OP_ENTRY:
			format_comment_entry(lc, &instr, comment);
			break;

		case OP_PUSH_TR:
			format_comment_pushtr(lc, &instr, comment);
			break;

		case OP_FETCH_FIELD:
			format_comment_fetchfield(lc, &instr, comment);
			break;

		case OP_END_RULE:
			newline = 1;
			break;

		case OP_RETURN:
			newline = 1;
			break;

		case OP_EXT_LOOKUP:
			format_comment_extlookup(lc, &instr, comment);
			break;

		case OP_EXT_FUNC:
			format_comment_extfunc(lc, &instr, comment);
			break;

		case OP_NULL_TR:
			format_comment_nulltr(lc, &instr, comment);
			break;

		case OP_PUSH_L:
			format_comment_pushl(lc, &instr, comment);
			break;
		}

		format_operands(attrs, &instr);

		if( comment[0] != '\0' ) {
			fprintf(fp, "%05d: %-15s %s     ; %s\n",
					lc, instr.opcode_name, attrs, comment);
		} else {
			fprintf(fp, "%05d: %-15s %s\n", lc, instr.opcode_name, attrs);
		}

		if( newline )
			fprintf(fp, "\n");
	}
}

/*----------------------------------------------------------------------
 * Execution-time memory manager
 *
 *	mem_reset()
 *		- Resets all of the memory pool pointers. Making
 *		  all memory available.
 *
 *	ptr = mem_alloc(n)
 *		- Allocates n bytes of storage from the memory pool
 *
 *	ptr = mem_strdup(str)
 *		- Duplicates a string. Memory is obtained from the
 *		  memory pool.
 *
 *	ptr = mem_mklist(n)
 *		- Returns storage for 'n' list items.
 *
 *	mem_remove_all()
 *		- Removes all of the memory pool.
 *
 * These routines simplify the execution of the byte codes
 * because none of the instruction need to care about
 * free-ing memory.
 *
 * The byte code machie is always pushing and poping objects
 * onto the stack. When new strings and lists are created,
 * the mem_XXXXXXX() routines are used.
 *
 */
#define MEM_ALIGNTO		sizeof(char*)
#define MEM_BLOCKSIZE		2048
typedef struct mem_list {
	int	size;
	char	*free;
	char	*data;
	struct mem_list *next;
} MEM_LIST;

static MEM_LIST *mem_head = NULL;

/*----------------------------------------------------------------------
 * Reset all of the free list pointers to
 * make all memory in the memory pool available.
 */
static void mem_reset(void)
{
	MEM_LIST *curr;

	for(curr=mem_head; curr; curr=curr->next)
		curr->free = curr->data;
}

/*----------------------------------------------------------------------
 * Remove the free list completely.
 * RETURNS:
 *	Total Number of bytes allocated for temporary
 *	storage.
 *
 */
int mem_remove_all(void)
{
	MEM_LIST *curr, *nxt;
	int tmpspace;

	tmpspace = 0;
	curr=mem_head;
	while(curr) {
		tmpspace += curr->size;
		nxt = curr->next;
		BRE_FREE(curr);
		curr=nxt;
	}
	mem_head = NULL;

	return tmpspace;
}

/*----------------------------------------------------------------------
 * Allocate 'size' bytes of memory from the memory pool.
 * This functions makes sure the returned pointer is aligned
 * correctly.
 */
static char *mem_alloc(int size)
{
	MEM_LIST *curr, *prev;
	char *ptr;
	int max, mod, pad;

	prev = NULL;
	for(curr=mem_head; curr; curr=curr->next) {
		if( size <= curr->size - (curr->free - curr->data) )
			break;
		prev = curr;
	}

	if( curr == NULL ) {
		/*
		 * No memory block was found. Allocate a new block
		 * and place in BEGINNING of list. 
		 */
		max = (size > MEM_BLOCKSIZE) ? size : MEM_BLOCKSIZE;

		ptr = (char*)BRE_MALLOC( sizeof(MEM_LIST) + max );
		if( ptr == NULL ) {
			Bre_Error(BREMSG_DIE_NOMEM);
		}
		curr = (MEM_LIST*)ptr;
		curr->size = max;
		curr->data = ptr + sizeof(MEM_LIST);
		curr->free = curr->data;
		curr->next = NULL;

		if( mem_head == NULL )
			mem_head = curr;
		else
			prev->next = curr;
	}

	ptr = curr->free;
	curr->free += size;

	/*
	 * Adjust the 'free' pointer to be aligned
	 * to a MEM_ALIGNTO boundary.
	 */
	mod = (curr->free - curr->data) % MEM_ALIGNTO;
	pad = (mod) ? MEM_ALIGNTO - mod : 0;
	curr->free += pad;

	return ptr;
}

static char *mem_strdup(char *str)
{
	int len;
	char *ptr;

	len = strlen(str);

	ptr = mem_alloc(len+1);

	strcpy(ptr, str);
	return ptr;
}

static BRE_DATUM *mem_mklist(int n)
{
	BRE_DATUM *ptr;

	ptr = (BRE_DATUM*)mem_alloc( sizeof(BRE_DATUM)*n );

	return ptr;
}

/*----------------------------------------------------------------------
 * Instruction functions. The following functions implement
 * the more complex byte code instructions: 
 *
 *	op_cmp_f()
 *	op_cmp_s()
 *	op_isnull_s()
 *	op_add_s()
 *	op_prepend()
 *	op_append()
 *	op_substr()
 *	op_sublst()
 *	op_in_il()
 *	op_in_fl()
 *	op_in_sl()
 *	op_in_ss()
 *	op_between_i()
 *	op_between_f()
 *	op_between_s()
 *	op_like()
 *	op_ext_func()
 *	op_ext_lookup()
 *	op_fetch_list_i()
 *	op_fetch_list_f()
 *	op_fetch_list_s()
 *	op_fetch_char()
 *	op_fetch_field()
 *	op_count_tr()
 *	op_conv_bs()
 *	op_conv_is()
 *	op_conv_fs()
 *	op_error()
 *	op_min_i()
 *	op_min_f()
 *	op_min_s()
 *	op_max_i()
 *	op_max_f()
 *	op_max_s()
 *	op_set_row_verify()
 */

/*----------------------------------------------------------------------
 * Compare two float objects.
 *
 * RETURNS:
 *	nil	- If d1 or d2 are nil.
 *	-1	- d1 < d2
 *	0	- d1 == d2
 *	+1	- d1 > d2
 *
 */
#define FLOAT_CMP_MIN	-0.000001
#define FLOAT_CMP_MAX	 0.000001
static BRE_INTEGER op_cmp_f(BRE_FLOAT f1, BRE_FLOAT f2)
{
	double diff;
	BRE_INTEGER i;

	if( f1.val == FLOAT_NIL || f2.val == FLOAT_NIL) {
		i.nil = 1;
		i.val = 0;
		return i;
	}

	i.nil = 0;

	diff = f1.val - f2.val;
	if( diff < FLOAT_CMP_MIN )
		i.val = -1;
	else if( diff > FLOAT_CMP_MAX )
		i.val = 1;
	else
		i.val = 0;

	return i;
}

/*----------------------------------------------------------------------
 * Compare two string.
 * RETURNS:
 *	-1	- s1 < s2
 *	0	- s1 == s2
 *	+1	- s1 > s2
 *	nil	- one of the strings is null, but not both.
 *
 */
static BRE_INTEGER op_cmp_s(BRE_STRING s1, BRE_STRING s2)
{
	BRE_INTEGER ni;

	if( s1.str == NULL || s2.str == NULL ) {
		ni.val = 0;
		ni.nil = 1;
	} else {
		ni.val = strcmp(s1.str, s2.str);
		ni.nil = 0;
	}

	return ni;
}

/*----------------------------------------------------------------------
 * Check is a string is NULL.
 * A null string is defined as:
 *	- a string of all spaces.
 *	- an empty string
 *	- a string that points to NULL.
 *
 * RETURNS:
 *	0 - String is NOT null.
 *	1 - string is null.
 *
 */
static int op_isnull_s(BRE_STRING s)
{
	char *p;

	if( s.str == NULL || s.len == 0 )
		return 1;

	for(p=s.str; *p; p++) {
		if( *p != ' ' )
			return 0;
	}
	return 1;
}

/*----------------------------------------------------------------------
 * Concatenate two string.
 *
 */
static BRE_STRING op_add_s(BRE_STRING s1, BRE_STRING s2)
{
	BRE_STRING ns;

	ns.str = NULL;
	ns.len = 0;

	if( s1.str == NULL || s2.str == NULL )
		return ns;

	ns.len = s1.len + s2.len;
	ns.str = mem_alloc( ns.len+1 );

	strcpy(ns.str, s1.str);
	strcat(ns.str, s2.str);
	return ns;
}

/*----------------------------------------------------------------------
 * Prepend a value to the beginning of a list.
 * Return the new list.
 * If the oroginal list is NULL, create a new one.
 *
 */
static BRE_LIST op_prepend(BRE_LIST lst, BRE_DATUM val)
{
	BRE_LIST newlst;
	int i, len;

	if( lst.list )
		len = lst.len;
	else
		len = 0;

	newlst.len	= len + 1;
	newlst.list	= mem_mklist( newlst.len );

	newlst.list[0] = val;
	for(i=0; i<len; i++) {
		newlst.list[i+1] = lst.list[i];
	}

	return newlst;
}

/*----------------------------------------------------------------------
 * Append a value to the end of a list.
 * Return the new list.
 * If the oroginal list is NULL, create a new one.
 *
 */
static BRE_LIST op_append(BRE_LIST lst, BRE_DATUM val)
{
	BRE_LIST newlst;
	int i, len;

	if( lst.list )
		len = lst.len;
	else
		len = 0;

	newlst.len	= len + 1;
	newlst.list	= mem_mklist( newlst.len );

	for(i=0; i<len; i++) {
		newlst.list[i] = lst.list[i];
	}
	newlst.list[i] = val;

	return newlst;
}

/*----------------------------------------------------------------------
 * Create a substring
 *
 */
static BRE_STRING op_substr(BRE_STRING s, int n, int m)
{
	BRE_STRING ns;

	ns.str = NULL;
	ns.len = 0;

	if( s.str == NULL )
		return ns;

	if( n < 0 || n >= s.len || m < 0 )
		return ns;

	ns.len = (m < s.len-n) ? (m) : (s.len-n);

	ns.str = mem_alloc( ns.len+1 );

	strncpy(ns.str, s.str+n, ns.len);
	ns.str[ns.len] = '\0';

	return ns;
}

/*----------------------------------------------------------------------
 * Create a sub list
 *
 */
static BRE_LIST op_sublst(BRE_LIST l, int n, int m)
{
	BRE_LIST nl;
	int i;

	nl.len = 0;
	nl.list = NULL;

	if( l.list == NULL )
		return nl;

	if( n < 0 || n >= l.len || m < 0 )
		return nl;

	nl.len = (m < l.len-n) ? (m) : (l.len-n);

	nl.list = mem_mklist( nl.len );

	for(i=0; i<nl.len; i++) {
		nl.list[i] = l.list[n+i];
	}

	return nl;
}

/*----------------------------------------------------------------------
 * op_in_il:	Check to see if integer 'v' is in the list 'l'
 * op_in_fl:	Check to see if float 'v' is in the list 'l'
 *
 */
static int op_in_il(BRE_INTEGER v, BRE_LIST l)
{
	int found, i;

	if( v.nil == 1 )
		return 0;

	found = 0;
	for(i=0; i<l.len; i++) {
		if( l.list[i].i.nil != 0 )
			continue;

		if( v.val == l.list[i].i.val ) {
			found = 1;
			break;
		}
	}
	return found;
}

static int op_in_fl(BRE_FLOAT v, BRE_LIST l)
{
	int found, i;

	if( v.val == FLOAT_NIL )
		return 0;

	found = 0;
	for(i=0; i<l.len; i++) {
		if( v.val == l.list[i].f.val ) {
			found = 1;
			break;
		}
	}
	return found;
}

static int op_in_sl(BRE_STRING v, BRE_LIST l)
{
	int found, i;
	BRE_INTEGER n;

	if( v.str == NULL )
		return 0;

	found = 0;
	for(i=0; i<l.len; i++) {
		n = op_cmp_s(v, l.list[i].s);
		if( n.nil == 0 && n.val == 0 ) {
			found = 1;
			break;
		}
	}
	return found;
}

static int op_in_ss(BRE_STRING v, BRE_STRING string)
{
	char *p;
	int n, found;

	if( v.str == NULL || string.str == NULL )
		return 0;

	found = 0;
	for(p=string.str; *p; p++) {
		n = strncmp(v.str, p, v.len);
		if( n == 0 ) {
			found = 1;
			break;
		}
	}
	return found;
}

static int op_between_i(BRE_INTEGER v, BRE_LIST l)
{
	int low, high, val;

	if( l.len != 2 || v.nil )
		return 0;

	if( l.list[0].i.nil || l.list[1].i.nil )
		return 0;

	low	= l.list[0].i.val;
	high	= l.list[1].i.val;
	val	= v.val;

	return (val >= low && val <= high);
}

static int op_between_f(BRE_FLOAT v, BRE_LIST l)
{
	double low, high, val;

	if( l.len != 2 || v.val == FLOAT_NIL )
		return 0;

	if( l.list[0].f.val == FLOAT_NIL
				|| l.list[1].f.val == FLOAT_NIL )
		return 0;

	low	= l.list[0].f.val;
	high	= l.list[1].f.val;
	val	= v.val;

	return (val >= low && val <= high);
}

/*----------------------------------------------------------------------
 * Compute the 'BETWEEN' operator (for strings)
 *
 * RETURNS:
 *	1 - If 's' alphabetically falls between the low and high
 *	    values in the list.
 *	0 - else.
 *
 */
static int op_between_s(BRE_LIST l, BRE_STRING s)
{
	BRE_STRING low, high;
	int cmp1, cmp2;

	low = l.list[0].s;
	high = l.list[1].s;

	if( l.len != 2 || s.str == NULL || low.str == NULL || high.str == NULL )
		return 0;

	cmp1 = strcmp(s.str, low.str);
	cmp2 = strcmp(s.str, high.str);

	if( cmp1 >= 0 && cmp2 <= 0 )
		return 1;
	else
		return 0;
}

/*----------------------------------------------------------------------
 * do_like()
 * do_star()
 *	These functions perform the LIKE operator string matching
 * algorithm.
 *
 */
static int do_like(char *, char *);
static int do_star(char *str, char *pat)
{
	char *p, *t;
	char c, c1;

	t = str;
	p = pat;

	while( (c = *p++) == '_' || c == '%' ) {
		if (c == '_' && *t++ == 0)
			return 0;
	}

	if( c == 0 )
		return 1;

	if( c == '\\' )
		c1 = *p;
	else
		c1 = c;

	for(;;) {
		if( (c == '[' || *t == c1) && do_like(t, p-1) )
			return 1;
		if( *t++ == 0 )
			return 0;
	}
}

/*----------------------------------------------------------------------
 * Compare the string 'str' to the globbing pattern 'pat'. The
 * special characters in pattern follow the SQL specification for
 * the LIKE operator:
 *
 *	'-'	- Matches any character.
 *	'%'	- Matches one or more characters.
 *
 * As an extension, the following can also be included inside of
 * pattern:
 *
 *	[set]	- Match any character in set.
 *	[^set]	- Match any character not in set.
 *
 * Use the backslash "\" before a character to remove any
 * special meaning it might have as a match operator.
 *
 * A set consists of single characters, to represent a range of
 * characters use '-'. For example,
 *	[0-9]		- Matches all digits.
 *	[a-zA-Z]	- Matches all letters.
 *
 * RETURNS:
 *	1 - string 'str' matches pattern 'pat'.
 *	0 - no match.
 */
static int do_like(char *str, char *pat)
{
	char *p, *t;
	char c;

	t = str;
	p = pat;

	while( c = *p++ ) {
		switch( c ) {
		case '_':
			if( *t == 0 )
				return 0;
			else
				t++;
		break;

		case '\\':
			if( *p++ != *t++ )
				return 0;
			break;

		case '%':
			return do_star(t, p);

		case '[':
		{
			char c1 = *t++;
			int invert = (*p == '^');

			if( invert )
				p++;
			c = *p++;
			while(1) {
				char cstart = c, cend = c;

				if( c == '\\' ) {
					cstart	= *p++;
					cend	= cstart;
				}
				c = *p++;
				if( c == '-' ) {
					cend = *p++;
					if( cend == '\\' )
						cend = *p++;
					c = *p++;
				}

				if( c1 >= cstart && c1 <= cend )
					goto match;
				if( c == ']' )
					break;
			}
			if( !invert )
				return 0;
			break;

		        /*
			 * Skip the rest of the [...] construct that already matched.
			 */
		match:
			while (c != ']') {
				c = *p++;
				if( c == '\\' )
					p++;
			}
			if( invert )
				return 0;
			break;
		}

		default:
			if( c != *t++ )
				return 0;
		}
	}

	if( *t )
		return 0;
	return 1;
}

/*----------------------------------------------------------------------
 * Compute the 'LIKE' operator.
 * 'str' is the string to check against the pattern 'pat'
 *
 * 'pat' - can contain '%' and '_' just like in SQL.
 *
 * RETURNS:
 *	0 - string 'str' does not match pattern 'pat'
 *	1 - String does match!
 *
 */
static int op_like(BRE_STRING str, BRE_STRING pat)
{
	if( str.str == NULL || pat.str == NULL )
		return 0;

	return do_like(str.str, pat.str);
}

/*----------------------------------------------------------------------
 * Invoke the external function callback.
 *	instr.s[0]	<- number of arguments passed to this
 *			   function.
 *	instr.s[1]	<- number of arguments to be returned. (Always 1)
 *	instr.str[0]	<- type of each input argument.
 *			   (ie. "SSIF" - means (string, string, integer, float) )
 *	instr.str[1]	<- type of the return argument.
 *			   (ie. "B" - means boolean)
 *	instr.str[2]	<- name of function
 *	instr.ptr[0]	<- a pointer to the function to invoke.
 *	instr.ptr[1]	<- external cookie
 *
 * Any strings returned by an external function will
 * be copied using mem_strdup(). This means that
 * the external function can return a static string.
 *
 * The return value from the callback is ignored.
 *
 */
static BRE_DATUM op_ext_func(PROGRAM_INSTRUCTION *instr, BRE_DATUM *args)
{
	BRE_DATUM ret_val;
	BRE_EXTERNAL_CB *callback;
	char *ret_type;
	char *name;
	void *external_cookie;

	callback	= (BRE_EXTERNAL_CB*)instr->ptr[0];
	external_cookie = instr->ptr[1];
	ret_type	= instr->str[1];
	name		= instr->str[2];

	callback(name, external_cookie, args, &ret_val);

	if( ret_type[0] == 'S' ) {
		ret_val.s.str = mem_strdup(ret_val.s.str);
	}

	return ret_val;
}

/*----------------------------------------------------------------------
 * Invoke the external lookup callback.
 * This function is similar to op_ext_func, except the
 * return list can be multiple values.
 *
 * The return values are collected into a single BRE_LIST object
 * and returned. Memory for the list, and strings is obtained
 * from mem_alloc().
 *
 *	instr.s[0]	<- number of arguments passed to this
 *			   function.
 *	instr.s[1]	<- number of arguments to be returned. (Always 1)
 *	instr.str[0]	<- type of each input argument.
 *			   (ie. "SSIF" - means (string, string, integer, float) )
 *	instr.str[1]	<- type of the return argument.
 *			   (ie. "BIS" - means: boolean, integer, string)
 *	instr.str[2]	<- name of lookup table.
 *	instr.ptr[0]	<- a pointer to the function to invoke.
 *	instr.ptr[1]	<- external lookup.
 *
 * Handling the callback return value is as follows:
 * 	- A return value of 0, indicates record successfully obtained.
 *	- A non-zero return indicates that the lookup failed to find
 *	  a record.
 */
static BRE_DATUM op_ext_lookup(PROGRAM_INSTRUCTION *instr, BRE_DATUM *args)
{
	BRE_DATUM *ret_list;
	BRE_DATUM ret_val;
	BRE_EXTERNAL_CB *callback;
	char *args_out, *p;
	int i, n, len;
	char *name;
	void *external_cookie;

	callback	= (BRE_EXTERNAL_CB*)instr->ptr[0];
	external_cookie = instr->ptr[1];
	len		= instr->s[1];
	args_out	= instr->str[1];
	name		= instr->str[2];

	ret_list = mem_mklist( len );

	n = callback(name, external_cookie, args, ret_list);

	if( n == 0 ) {
		i = 0;
		for(p=args_out; *p; p++) {
			if( *p == 'S' ) {
				ret_list[i].s.str = mem_strdup(ret_list[i].s.str);
			}

			i++;
		}
		ret_val.l.len	= len;
		ret_val.l.list	= ret_list;
	} else {
		ret_val.l.len = 0;
		ret_val.l.list = NULL;
	}

	return ret_val;
}

/*----------------------------------------------------------------------
 * op_fetch_list_i(), op_fetch_list_f(), op_fetch_list_s()
 *
 * Fetch a data value from a list. The list is indexed
 * using the integer argument 'i'.
 *
 * A null value will be placed on this stack if,
 *	a. the list being indexed is null.
 *	b. The index is out of range.
 *
 */
static BRE_INTEGER op_fetch_list_i(BRE_LIST l, BRE_INTEGER i)
{
	BRE_INTEGER ret;

	if( i.nil || l.list == NULL || i.val < 0 || i.val >= l.len ) {
		ret.val = 0;
		ret.nil = 1;
		return ret;
	}

	ret = l.list[ i.val ].i;

	return ret;
}

static BRE_FLOAT op_fetch_list_f(BRE_LIST l, BRE_INTEGER i)
{
	BRE_FLOAT ret;

	if( i.nil || l.list == NULL || i.val < 0 || i.val >= l.len ) {
		ret.val = FLOAT_NIL;
		return ret;
	}

	ret = l.list[ i.val ].f;

	return ret;
}

static BRE_STRING op_fetch_list_s(BRE_LIST l, BRE_INTEGER i)
{
	BRE_STRING ret;

	if( i.nil || l.list == NULL || i.val < 0 || i.val >= l.len ) {
		ret.len = 0;
		ret.str = NULL;
		return ret;
	}

	ret = l.list[ i.val ].s;

	return ret;
}

/*----------------------------------------------------------------------
 * Fetch character located at position 'i'
 *
 */
static BRE_STRING op_fetch_char(BRE_STRING s, BRE_INTEGER i)
{
	BRE_STRING ret_val;

	if( s.str == NULL || i.nil || i.val >= s.len || i.val < 0 ) {
		ret_val.str	= NULL;
		ret_val.len	= 0;
		return ret_val;
	}

	ret_val.len = 1;
	ret_val.str = mem_alloc(2);
	ret_val.str[0] = s.str[ i.val ];
	ret_val.str[1] = '\0';

	return ret_val;
}

/*----------------------------------------------------------------------
 * Call the client to fetch a field value.
 * If the table row pointer 'tr' is null, then return
 * a null value.
 */
static BRE_DATUM op_fetch_field(PROGRAM_INSTRUCTION *instr, BRE_TABLEROW tr)
{
	void *table_cookie;
	void *field_cookie;
	SYMBOL *table, *field;
	BRE_FIELDQUERY_CB *cb;
	BRE_DATUM datum;
	int new_value;
	char tbuf[200], fbuf[200];
	char type;
	char *newstr;

	field		= (SYMBOL*)instr->ptr[0];
	table		= tr.table;
	table_cookie	= table->u.tab.table_cookie;
	field_cookie	= field->u.fld.field_cookie;
	cb =		(BRE_FIELDQUERY_CB*)instr->ptr[1];
	new_value	= instr->b[0];
	type		= instr->b[1];

	if( tr.nil ) {
		switch( type ) {
		case 'I':
			datum.i.nil = 1;
			break;
		case 'F':
			datum.f.val = FLOAT_NIL;
			break;
		case 'S':
			datum.s.len = 0;
			datum.s.str = NULL;
			break;
		default:
			BRE_ASSERT(0);
		}
		return datum;
	}

	sym_extract_pair(field->name, tbuf, fbuf);

	cb(table->name, table_cookie, fbuf, field_cookie, tr.row, new_value, &datum);

	/*
	 * Make a copy of the string in 'datum'
	 */
	if( type == 'S' ) {
		newstr = mem_alloc( datum.s.len + 1 );
		strncpy(newstr, datum.s.str, datum.s.len);
		newstr[ datum.s.len ] = '\0';
		datum.s.str = newstr;
	}

	return datum;
}

static BRE_INTEGER op_count_tr(BRE_TABLEROW tr)
{
	BRE_ROWCOUNT_CB *cb;
	BRE_INTEGER count;

	cb = BRE()->rowcount_cb;

	count.nil = 0;
	count.val = cb(tr.table->name, tr.table->u.tab.table_cookie);

	return count;
}

static BRE_STRING op_conv_bs(BRE_INTEGER b)
{
	BRE_STRING s;
	char *tf;

	tf = (b.val) ? "true" : "false";

	s.str = mem_strdup(tf);
	s.len = strlen(s.str);
	return s;
}

static BRE_STRING op_conv_is(BRE_INTEGER i)
{
	BRE_STRING s;
	char buf[ 100 ];

	if( i.nil )
		sprintf(buf, "nil");
	else
		sprintf(buf, "%d", i.val);

	s.str = mem_strdup(buf);
	s.len = strlen(s.str);
	return s;
}

static BRE_STRING op_conv_fs(BRE_FLOAT f)
{
	BRE_STRING s;
	char buf[ 100 ];

	if( f.val == FLOAT_NIL )
		sprintf(buf, "nil");
	else
		sprintf(buf, "%.3f", f.val);

	s.str = mem_strdup(buf);
	s.len = strlen(s.str);
	return s;
}

/*----------------------------------------------------------------------
 * Construct a list of message strings and invoke the
 * rule diagnostic callback.
 *
 * If the 'fatal_error' string is set, then return this as the
 * error message, and an error code of BRE_FATA.
 *
 */
static void op_error(PROGRAM_INSTRUCTION *instr, BRE_LIST l)
{
	BRE_RULEDIAG_CB *diagnostic;
	char *rule_id;
	BRE_DIAGNOSTIC diag;
	char **message_tokens;
	char *filename;
	int idx, i;

	diagnostic = BRE()->rulediag_cb;

	if( diagnostic == NULL )
		return;

	diag	= (BRE_DIAGNOSTIC)instr->b[0];
	idx	= instr->s[0];
	rule_id	= instr->str[0];
	filename = BRE()->filenames[ idx-1 ];

	if( BRE()->fatal_error == NULL ) {
		message_tokens = (char**)mem_alloc( sizeof(char*) * (l.len+1) );
		for(i=0; i<l.len; i++) {
			if( l.list[i].s.str == NULL ) {
				message_tokens[i] = "nil";
			} else {
				message_tokens[i] = l.list[i].s.str;
			}
		}
		message_tokens[i] = NULL;
	} else {
		/* report a FATAL error */
		diag = BRE_FATAL;
		message_tokens = (char**)mem_alloc( sizeof(char*) * 2 );
		message_tokens[0] = BRE()->fatal_error;
		message_tokens[1] = NULL;
	}

	diagnostic(filename, rule_id, diag, message_tokens);
}

static BRE_INTEGER op_min_i(BRE_INTEGER i1, BRE_INTEGER i2)
{
	BRE_INTEGER ret_val;

	if( i1.nil || i2.nil ) {
		ret_val.val = 0;
		ret_val.nil = 1;
		return ret_val;
	}

	ret_val.nil = 0;
	if( i1.val < i2.val )
		ret_val.val = i1.val;
	else
		ret_val.val = i2.val;

	return ret_val;
}

static BRE_FLOAT op_min_f(BRE_FLOAT f1, BRE_FLOAT f2)
{
	BRE_FLOAT ret_val;

	if( f1.val == FLOAT_NIL || f2.val == FLOAT_NIL ) {
		ret_val.val = FLOAT_NIL;
		return ret_val;
	}

	if( f1.val < f2.val )
		ret_val.val = f1.val;
	else
		ret_val.val = f2.val;

	return ret_val;
}

static BRE_STRING op_min_s(BRE_STRING s1, BRE_STRING s2)
{
	BRE_STRING ret_val;
	BRE_INTEGER i;

	if( s1.str == NULL || s2.str == NULL ) {
		ret_val.len = 0;
		ret_val.str = NULL;
		return ret_val;
	}

	i = op_cmp_s(s1, s2);

	if( i.nil ) {
		ret_val.len = 0;
		ret_val.str = NULL;
		return ret_val;
	}

	if( i.val < 0 )
		ret_val = s1;
	else
		ret_val = s2;

	return ret_val;
}

static BRE_INTEGER op_max_i(BRE_INTEGER i1, BRE_INTEGER i2)
{
	BRE_INTEGER ret_val;

	if( i1.nil || i2.nil ) {
		ret_val.val = 0;
		ret_val.nil = 1;
		return ret_val;
	}

	ret_val.nil = 0;
	if( i1.val > i2.val )
		ret_val.val = i1.val;
	else
		ret_val.val = i2.val;

	return ret_val;
}

static BRE_FLOAT op_max_f(BRE_FLOAT f1, BRE_FLOAT f2)
{
	BRE_FLOAT ret_val;

	if( f1.val == FLOAT_NIL || f2.val == FLOAT_NIL ) {
		ret_val.val = FLOAT_NIL;
		return ret_val;
	}

	if( f1.val > f2.val )
		ret_val.val = f1.val;
	else
		ret_val.val = f2.val;

	return ret_val;
}

static BRE_STRING op_max_s(BRE_STRING s1, BRE_STRING s2)
{
	BRE_STRING ret_val;
	BRE_INTEGER i;

	if( s1.str == NULL || s2.str == NULL ) {
		ret_val.len = 0;
		ret_val.str = NULL;
		return ret_val;
	}

	i = op_cmp_s(s1, s2);

	if( i.nil ) {
		ret_val.len = 0;
		ret_val.str = NULL;
		return ret_val;
	}

	if( i.val > 0 )
		ret_val = s1;
	else
		ret_val = s2;

	return ret_val;
}

/*----------------------------------------------------------------------
 * Set the table row 'tr' to index 'i'. But first, verify that
 * the table 'tr' has the correct number of rows.
 *
 */
static BRE_TABLEROW op_set_row_verify(BRE_TABLEROW tr, BRE_INTEGER i)
{
	BRE_ROWCOUNT_CB *cb;
	BRE_TABLEROW ret_val;
	unsigned int count;

	if( tr.nil || i.nil || i.val < 0 || i.val >= count ) {
		ret_val.nil = 1;
		ret_val.row = tr.row;
		ret_val.table = tr.table;
		return ret_val;
	}

	cb = BRE()->rowcount_cb;
	count = cb(tr.table->name, tr.table->u.tab.table_cookie);
	ret_val.nil = 0;
	ret_val.row = i.val;
	ret_val.table = tr.table;

	return ret_val;
}

/***********************************************************************
 ***********************************************************************
 *			BYTE CODE EXECUTION FUNCTION
 *
 * Execute the bytes codes starting at 'entry_point'
 *
 * Assumes SP and FP are initialized appropriately.
 *
 */
static BRE_DATUM Stack[ STACK_SIZE ];
static int SP;
static int FP;

static void run_evaluate(PROGRAM *prog, int entry_point)
{
	PROGRAM_INSTRUCTION instr;
	int end;
#if 0
	int prev_lc;
#endif

	mem_reset();

	prog->lc = entry_point;

	end = 0;
	while( !end ) {
#if 0
		prev_lc = prog->lc;
#endif

		prog_decode(prog, &instr);

#if 0
   printf("%05d: %-15s (SP=%03d, FP=%03d)\n", prev_lc, instr.opcode_name, SP, FP);
   fflush(stdout);
#endif

		switch( instr.op ) {
		case OP_PUSH_I:
			Stack[SP].i.nil		= 0;
			Stack[SP++].i.val	= instr.l[0];
			break;

		case OP_PUSH_F:
			Stack[SP++].f.val	= instr.d[0];
			break;

		case OP_PUSH_S:
			Stack[SP].s.len		= instr.s[0];
			Stack[SP++].s.str	= instr.str[0];
			break;

		case OP_PUSH_L:
			Stack[SP].l.len		= instr.datum.l.len;
			Stack[SP++].l.list	= instr.datum.l.list;
			break;

		case OP_PUSH_TR:
			Stack[SP].tr.nil	= 0;
			Stack[SP].tr.row	= instr.l[0];
			Stack[SP++].tr.table	= (SYMBOL*)instr.ptr[0];
			break;

		case OP_NULL_I:
			Stack[SP++].i.nil = 1;
			break;

		case OP_NULL_F:
			Stack[SP++].f.val = FLOAT_NIL;
			break;

		case OP_NULL_S:
			Stack[SP].s.len = 0;
			Stack[SP++].s.str = NULL;
			break;

		case OP_NULL_TR:
			Stack[SP].tr.row = 0;
			Stack[SP].tr.table = (SYMBOL*)instr.ptr[0];
			Stack[SP++].tr.nil = 1;
			break;

		case OP_NULL_L:
			Stack[SP].l.len = 0;
			Stack[SP++].l.list = NULL;
			break;

		case OP_POP:
			SP--;
			break;

		case OP_ADD_I:
			Stack[SP-2].i.nil = Stack[SP-1].i.nil || Stack[SP-2].i.nil;
			Stack[SP-2].i.val = Stack[SP-2].i.val + Stack[SP-1].i.val;
			SP--;
			break;

		case OP_ADD_F:
			if( Stack[SP-2].f.val == FLOAT_NIL || Stack[SP-1].f.val == FLOAT_NIL )
				Stack[SP-2].f.val = FLOAT_NIL;
			else
				Stack[SP-2].f.val = Stack[SP-2].f.val + Stack[SP-1].f.val;
			SP--;
			break;

		case OP_ADD_S:
			Stack[SP-2].s = op_add_s(Stack[SP-2].s, Stack[SP-1].s);
			SP--;
			break;

		case OP_PREPEND:
			Stack[SP-2].l = op_prepend(Stack[SP-2].l, Stack[SP-1]);
			SP--;
			break;

		case OP_APPEND:
			Stack[SP-2].l = op_append(Stack[SP-2].l, Stack[SP-1]);
			SP--;
			break;

		case OP_SUB_I:
			Stack[SP-2].i.nil = Stack[SP-1].i.nil || Stack[SP-2].i.nil;
			Stack[SP-2].i.val = Stack[SP-2].i.val - Stack[SP-1].i.val;
			SP--;
			break;

		case OP_SUB_F:
			if( Stack[SP-2].f.val == FLOAT_NIL || Stack[SP-1].f.val == FLOAT_NIL )
				Stack[SP-2].f.val = FLOAT_NIL;
			else
				Stack[SP-2].f.val = Stack[SP-2].f.val - Stack[SP-1].f.val;
			SP--;
			break;

		case OP_MUL_I:
			Stack[SP-2].i.nil = Stack[SP-1].i.nil || Stack[SP-2].i.nil;
			Stack[SP-2].i.val = Stack[SP-2].i.val * Stack[SP-1].i.val;
			SP--;
			break;

		case OP_MUL_F:
			if( Stack[SP-2].f.val == FLOAT_NIL || Stack[SP-1].f.val == FLOAT_NIL )
				Stack[SP-2].f.val = FLOAT_NIL;
			else
				Stack[SP-2].f.val = Stack[SP-2].f.val * Stack[SP-1].f.val;
			SP--;
			break;

		case OP_DIV_I:
			Stack[SP-2].i.nil = Stack[SP-1].i.nil || Stack[SP-2].i.nil;
			Stack[SP-2].i.val = Stack[SP-2].i.val / Stack[SP-1].i.val;
			SP--;
			break;

		case OP_DIV_F:
			if( Stack[SP-2].f.val == FLOAT_NIL || Stack[SP-1].f.val == FLOAT_NIL )
				Stack[SP-2].f.val = FLOAT_NIL;
			else
				Stack[SP-2].f.val = Stack[SP-2].f.val / Stack[SP-1].f.val;
			SP--;
			break;

		case OP_MOD_I:
			Stack[SP-2].i.nil = Stack[SP-1].i.nil || Stack[SP-2].i.nil;
			Stack[SP-2].i.val = Stack[SP-2].i.val % Stack[SP-1].i.val;
			SP--;
			break;

		case OP_NEG_I:
			Stack[SP-2].i.val = - Stack[SP-2].i.val;
			break;

		case OP_NEG_F:
			if( Stack[SP-2].f.val != FLOAT_NIL )
				Stack[SP-2].f.val = - Stack[SP-2].f.val;
			break;

		case OP_CMP_I:
			if( Stack[SP-2].i.nil || Stack[SP-1].i.nil )
				Stack[SP-2].i.nil = 1;
			else {
				Stack[SP-2].i.nil = 0;
				Stack[SP-2].i.val = (Stack[SP-2].i.val - Stack[SP-1].i.val);
			}
			SP--;
			break;

		case OP_CMP_F:
			Stack[SP-2].i = op_cmp_f(Stack[SP-2].f, Stack[SP-1].f);
			SP--;
			break;

		case OP_CMP_S:
			Stack[SP-2].i = op_cmp_s(Stack[SP-2].s, Stack[SP-1].s);
			SP--;
			break;

		case OP_CMP_TR:
			if( Stack[SP-2].tr.nil || Stack[SP-2].tr.nil ) {
				Stack[SP-2].i.nil = 1;
			} else {
				Stack[SP-2].i.nil = 0;
				Stack[SP-2].i.val = (Stack[SP-2].tr.row - Stack[SP-1].tr.row);
			}
			SP--;
			break;

		case OP_EQ:
			if( Stack[SP-1].i.nil )
				Stack[SP-1].i.val = 0;
			else
				Stack[SP-1].i.val = (Stack[SP-1].i.val == 0);
			break;

		case OP_LT:
			if( Stack[SP-1].i.nil )
				Stack[SP-1].i.val = 0;
			else
				Stack[SP-1].i.val = (Stack[SP-1].i.val < 0);
			break;

		case OP_LE:
			if( Stack[SP-1].i.nil )
				Stack[SP-1].i.val = 0;
			else
				Stack[SP-1].i.val = (Stack[SP-1].i.val <= 0);
			break;

		case OP_GT:
			if( Stack[SP-1].i.nil )
				Stack[SP-1].i.val = 0;
			else
				Stack[SP-1].i.val = (Stack[SP-1].i.val > 0);
			break;

		case OP_GE:
			if( Stack[SP-1].i.nil )
				Stack[SP-1].i.val = 0;
			else
				Stack[SP-1].i.val = (Stack[SP-1].i.val >= 0);
			break;

		case OP_ISNULL_I:
			if( Stack[SP-1].i.nil )
				Stack[SP-1].i.val = 1;
			else
				Stack[SP-1].i.val = 0;
			break;

		case OP_ISNULL_F:
			if( Stack[SP-1].f.val == FLOAT_NIL )
				Stack[SP-1].i.val = 1;
			else
				Stack[SP-1].i.val = 0;
			break;

		case OP_ISNULL_S:
			if( op_isnull_s(Stack[SP-1].s) )
				Stack[SP-1].i.val = 1;
			else
				Stack[SP-1].i.val = 0;
			break;

		case OP_ISNULL_TR:
			if( Stack[SP-1].tr.nil )
				Stack[SP-1].i.val = 1;
			else
				Stack[SP-1].i.val = 0;
			break;

		case OP_ISNULL_L:
			if( Stack[SP-1].l.len == 0 || Stack[SP-1].l.list == NULL )
				Stack[SP-1].i.val = 1;
			else
				Stack[SP-1].i.val = 0;
			break;

		case OP_NOT:
			Stack[SP-1].i.val = ! Stack[SP-1].i.val;
			break;

		case OP_IN_IL:
			Stack[SP-2].i.val = op_in_il(Stack[SP-2].i, Stack[SP-1].l);
			SP--;
			break;

		case OP_IN_FL:
			Stack[SP-2].i.val = op_in_fl(Stack[SP-2].f, Stack[SP-1].l);
			SP--;
			break;

		case OP_IN_SL:
			Stack[SP-2].i.val = op_in_sl(Stack[SP-2].s, Stack[SP-1].l);
			SP--;
			break;

		case OP_IN_SS:
			Stack[SP-2].i.val = op_in_ss(Stack[SP-2].s, Stack[SP-1].s);
			SP--;
			break;

		case OP_BETWEEN_I:
			Stack[SP-2].i.val = op_between_i(Stack[SP-2].i, Stack[SP-1].l);
			SP--;
			break;

		case OP_BETWEEN_F:
			Stack[SP-2].i.val = op_between_f(Stack[SP-2].f, Stack[SP-1].l);
			SP--;
			break;

		case OP_BETWEEN_S:
			Stack[SP-2].i.val = op_between_s(Stack[SP-1].l, Stack[SP-2].s);
			SP--;
			break;

		case OP_LIKE:
			Stack[SP-2].i.val = op_like(Stack[SP-2].s, Stack[SP-1].s);
			SP--;
			break;

		case OP_BRANCH_TRUE:
			if( Stack[SP-1].i.val )
				prog->lc = instr.l[0];
			if( instr.b[0] )
				SP--;
			break;

		case OP_BRANCH_FALSE:
			if( Stack[SP-1].i.val == 0 )
				prog->lc = instr.l[0];
			if( instr.b[0] )
				SP--;
			break;

		case OP_BRANCH_LT1:
			if( Stack[SP-1].i.val < 1 )
				prog->lc = instr.l[0];
			if( instr.b[0] )
				SP--;
			break;

		case OP_BRANCH_ZERO:
			if( Stack[SP-1].i.val == 0 )
				prog->lc = instr.l[0];
			if( instr.b[0] )
				SP--;
			break;

		case OP_BRANCH:
			prog->lc = instr.l[0];
			break;

		case OP_CHECK_RULE:
			if( Stack[SP-1].i.val && BRE()->fatal_error == NULL )
				prog->lc = instr.l[0];
			break;

		case OP_CALL:
			Stack[SP++].i.val = prog->lc;
			prog->lc = instr.l[0];
			break;

		case OP_RETURN:
			prog->lc = Stack[FP-1].i.val;
			Stack[ FP - (instr.s[0]+1) ] = Stack[SP-1];
			SP = FP - instr.s[0];
			FP = Stack[FP].i.val;
			break;

		case OP_ENTRY:
			Stack[SP++].i.val = FP;
			FP = SP-1;
			SP += instr.s[0];
			break;

		case OP_END_RULE:
			BRE_ASSERT( SP - instr.s[0] - 3 == 1 );
			Stack[ FP-2 ] = Stack[SP-1];
			SP = FP-2;
			end = 1;
			break;

		case OP_END_CONST:
			end = 1;
			break;

		case OP_ERROR:
			op_error(&instr, Stack[SP-1].l);
			SP--;
			break;

		case OP_NOMORE:
			BRE_ASSERT(0);

		case OP_EXT_FUNC:
			Stack[SP - instr.s[0]] = op_ext_func(&instr, &Stack[SP - instr.s[0]]);
			SP -= instr.s[0]-1;
			break;

		case OP_EXT_LOOKUP:
			Stack[SP - instr.s[0]] = op_ext_lookup(&instr, &Stack[SP - instr.s[0]]);
			SP -= instr.s[0]-1;
			break;

		case OP_MKLIST:
			{
				int i;
				BRE_LIST bl;

				bl.len = instr.s[0];
				bl.list = mem_mklist( bl.len );

				for(i=0; i<bl.len; i++) {
					bl.list[i] = Stack[SP-bl.len+i];
				}

				SP -= bl.len;
				Stack[SP++].l = bl;
			}
			break;

		case OP_ASSIGN:
			Stack[FP + instr.s[0] ] = Stack[SP-1];
			if( instr.b[0] )
				SP--;
			break;

		case OP_GETFP:
			Stack[SP++] = Stack[FP + instr.s[0] ];
			break;

		case OP_GETSP:
			Stack[SP] = Stack[SP + instr.s[0] ];
			SP++;
			break;

		case OP_PUTSP:
			Stack[SP + instr.s[0] ] = Stack[SP-1];
			SP--;
			break;

		case OP_INCSP:
			Stack[SP + instr.s[0] ].i.val += 1;
			break;

		case OP_DECSP:
			Stack[SP + instr.s[0] ].i.val -= 1;
			break;

		case OP_CONV_IF:
			if( Stack[SP-1].i.nil )
				Stack[SP-1].f.val = FLOAT_NIL;
			else
				Stack[SP-1].f.val = Stack[SP-1].i.val;
			break;

		case OP_CONV_FI:
			Stack[SP-1].i.val = Stack[SP-1].f.val;
			Stack[SP-1].i.nil = 0;
			break;

		case OP_CONV_BS:
			Stack[SP-1].s = op_conv_bs(Stack[SP-1].i);
			break;

		case OP_CONV_IS:
			Stack[SP-1].s = op_conv_is(Stack[SP-1].i);
			break;

		case OP_CONV_FS:
			Stack[SP-1].s = op_conv_fs(Stack[SP-1].f);
			break;

		case OP_CONV_TRI:
			{	short row;

				if( Stack[SP-1].tr.nil ) {
					Stack[SP-1].i.nil = 1;
				} else {
					row = Stack[SP-1].tr.row;
					Stack[SP-1].i.val = row;
					Stack[SP-1].i.nil = 0;
				}
			}
			break;

		case OP_SET_ROW:
			Stack[SP-2].tr.row = Stack[SP-1].i.val;
			SP--;
			break;

		case OP_SET_ROW_VERIFY:
			Stack[SP-2].tr = op_set_row_verify(Stack[SP-2].tr, Stack[SP-1].i);
			SP--;
			break;

		case OP_FETCH_LIST_I:
			Stack[SP-2].i = op_fetch_list_i(Stack[SP-2].l, Stack[SP-1].i);
			SP--;
			break;

		case OP_FETCH_LIST_F:
			Stack[SP-2].f = op_fetch_list_f(Stack[SP-2].l, Stack[SP-1].i);
			SP--;
			break;

		case OP_FETCH_LIST_S:
			Stack[SP-2].s = op_fetch_list_s(Stack[SP-2].l, Stack[SP-1].i);
			SP--;
			break;

		case OP_FETCH_FIELD:
			Stack[SP-1] = op_fetch_field(&instr, Stack[SP-1].tr);
			break;

		case OP_FETCH_CHAR:
			Stack[SP-2].s = op_fetch_char(Stack[SP-2].s, Stack[SP-1].i);
			SP--;
			break;

		case OP_SUBSTR:
			Stack[SP-3].s = op_substr(Stack[SP-3].s, Stack[SP-2].i.val,
									Stack[SP-1].i.val);
			SP -= 2;
			break;

		case OP_SUBLST:
			Stack[SP-3].l = op_sublst(Stack[SP-3].l, Stack[SP-2].i.val,
									Stack[SP-1].i.val);
			SP -= 2;
			break;

		case OP_MAX_I:
			Stack[SP-2].i = op_max_i(Stack[SP-2].i, Stack[SP-1].i);
			SP--;
			break;

		case OP_MAX_F:
			Stack[SP-2].f = op_max_f(Stack[SP-2].f, Stack[SP-1].f);
			SP--;
			break;

		case OP_MAX_S:
			Stack[SP-2].s = op_max_s(Stack[SP-2].s, Stack[SP-1].s);
			SP--;
			break;

		case OP_MIN_I:
			Stack[SP-2].i = op_min_i(Stack[SP-2].i, Stack[SP-1].i);
			SP--;
			break;

		case OP_MIN_F:
			Stack[SP-2].f = op_min_f(Stack[SP-2].f, Stack[SP-1].f);
			SP--;
			break;

		case OP_MIN_S:
			Stack[SP-2].s = op_min_s(Stack[SP-2].s, Stack[SP-1].s);
			SP--;
			break;

		case OP_COUNT_L:
			Stack[SP-1].i.val = Stack[SP-1].l.len;
			Stack[SP-1].i.nil = 0;
			break;

		case OP_COUNT_S:
			Stack[SP-1].i.val = Stack[SP-1].s.len;
			Stack[SP-1].i.nil = 0;
			break;

		case OP_COUNT_TR:
			Stack[SP-1].i = op_count_tr(Stack[SP-1].tr);
			break;

		case OP_AND:
			Stack[SP-2].i.val = Stack[SP-2].i.val && Stack[SP-1].i.val;
			SP--;
			break;

		case OP_OR:
			Stack[SP-2].i.val = Stack[SP-2].i.val || Stack[SP-1].i.val;
			SP--;
			break;

		default:
			BRE_ASSERT(0);
		}

		/*
		 * Check for stack overflow.
		 */
		BRE_ASSERT( SP < STACK_SIZE );
	}
}

/*----------------------------------------------------------------------
 * This function is called from the code generator, when
 * a constant is being compiled. This evalutes the constant
 * code and reduces it to a single value.
 *
 */
void run_evaluate_constant(PROGRAM *prog, int entry_point, BRE_DATUM *datum)
{
	SP = 0;
	run_evaluate(prog, entry_point);
	*datum = Stack[0];
}

/*----------------------------------------------------------------------
 * This function performs the basic rule evaluation.
 * 'prog' is the byte-code program to evaluate.
 * 'sym' must be a symbol table entry for a S_RULE type.
 *
 * This function will evaluate the rule for all rows
 * of the 'using' table. If the rule does not have a 'using'
 * table, then we evaluate the rule just once.
 *
 * Returns:
 *	1 - During one of the evaluations of the rule, a FALSE
 *	    was returned.
 *	0 - Else.
 */
int run_evaluate_rule(PROGRAM *prog, SYMBOL *sym)
{
	BRE_ROWCOUNT_CB	*rowcount;
	SYMBOL *using;
	int count, i;

	BRE_ASSERT(sym->kind == S_RULE);

	rowcount = BRE()->rowcount_cb;
	using	 = sym->u.rul.using;

	if( using ) {
		count = rowcount(using->name, using->u.tab.table_cookie);
		for(i=0; i<count; i++) {
			Stack[0].tr.nil = 0;
			Stack[0].tr.row = i;
			Stack[0].tr.table = using;
			Stack[1].i.val = 0;
			Stack[1].i.nil = 0;
			SP = 2;

			run_evaluate(prog, sym->u.rul.entry_point);
			if( Stack[0].i.val == 0 )
				return 1;
		}
		return 0;
	} else {
		Stack[0].i.val = 0;
		Stack[0].i.nil = 0;
		Stack[1].i.val = 0;
		Stack[1].i.nil = 0;
		SP = 2;

		run_evaluate(prog, sym->u.rul.entry_point);
		if( Stack[0].i.val )
			return 0;
		else
			return 1;
	}
}
