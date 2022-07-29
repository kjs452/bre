#ifndef _BRE_OPS_H
#define _BRE_OPS_H

/*----------------------------------------------------------------------
 * Business Rule Byte Codes
 *
 *
 */

/* ---------------------------------------------------------------------- */

typedef enum {		/* ------stack------		operand			*/
 OP_PUSH_I,		/* (x -- x i)			integer literal		*/
 OP_PUSH_F,		/* (x -- x f)			float literal		*/
 OP_PUSH_S,		/* (x -- x s)			string literal		*/
 OP_PUSH_L,		/* (x -- x l)			list literal		*/
 OP_PUSH_TR,		/* (x -- x tr)			table row literal	*/

 OP_NULL_I,		/* (x -- x i)			integer literal		*/
 OP_NULL_F,		/* (x -- x f)			float literal		*/
 OP_NULL_S,		/* (x -- x s)			string literal		*/
 OP_NULL_TR,		/* (x -- x tr)			table row literal	*/
 OP_NULL_L,		/* (x -- x tr)			table row literal	*/

 OP_POP,		/* (x i -- x)						*/

 OP_ADD_I,		/* (i i -- i)			not used		*/
 OP_ADD_F,		/* (f f -- f)			not used		*/
 OP_ADD_S,		/* (s s -- s)			not used		*/

 OP_PREPEND,		/* (l x -- l)			not used		*/
 OP_APPEND,		/* (l x -- l)			not used		*/

 OP_SUB_I,		/* (i i -- f)			not used		*/
 OP_SUB_F,		/* (f f -- f)			not used		*/

 OP_MUL_I,		/* (i i -- i)			not used		*/
 OP_MUL_F,		/* (f f -- f)			not used		*/

 OP_DIV_I,		/* (i i -- i)			not used		*/
 OP_DIV_F,		/* (f f -- f)			not used		*/

 OP_MOD_I,		/* (i i -- i)			not used		*/

 OP_NEG_I,		/* (i -- i)			not used		*/
 OP_NEG_F,		/* (f -- f)			not used		*/

 OP_CMP_I,		/* (i i -- cv)			not used		*/
 OP_CMP_F,		/* (f f -- cv)			not used		*/
 OP_CMP_S,		/* (s s -- cv)			not used		*/
 OP_CMP_TR,		/* (tr tr -- cv)		not used		*/

 OP_EQ,			/* (cv -- i)			not used		*/
 OP_LT,			/* (cv -- i)			not used		*/
 OP_LE,			/* (cv -- i)			not used		*/
 OP_GT,			/* (cv -- i)			not used		*/
 OP_GE,			/* (cv -- i)			not used		*/

 OP_ISNULL_I,		/* (i -- i)			not used		*/
 OP_ISNULL_F,		/* (f -- i)			not used		*/
 OP_ISNULL_S,		/* (s -- i)			not used		*/
 OP_ISNULL_TR,		/* (tr -- i)			not used		*/
 OP_ISNULL_L,		/* (l -- i)			not used		*/

 OP_NOT,		/* (i -- i)			not used		*/

 OP_IN_IL,		/* (i l -- i)			not used		*/
 OP_IN_FL,		/* (f l -- i)			not used		*/
 OP_IN_SL,		/* (s l -- i)			not used		*/
 OP_IN_SS,		/* (s s -- i)			not used		*/

 OP_BETWEEN_I,		/* (i i i -- i)			not used		*/
 OP_BETWEEN_F,		/* (f f f -- i)			not used		*/
 OP_BETWEEN_S,		/* (s s s -- i)			not used		*/

 OP_LIKE,		/* (x s s -- b)						*/

 OP_BRANCH_TRUE,	/* (x i -- x)			n=location to jump to	*/
 OP_BRANCH_FALSE,	/* (x i -- x)			n=location to jump to	*/
 OP_BRANCH_LT1,		/* (x i -- x)			n=location to jump to	*/
 OP_BRANCH_ZERO,	/* (x i -- x)			n=location to jump to	*/
 OP_BRANCH,		/* (x -- x)			n=location to jump to	*/
 OP_CHECK_RULE,		/* (x i -- x)			n=location to jump to	*/
 OP_CALL,		/* (x -- x i)			n=location to jsr to	*/

 OP_RETURN,		/* (x p p p p i -- x)		n=# of parameters to pop*/

 OP_ENTRY,		/* (x -- x)			n=# of local vars	*/
 OP_END_RULE,		/* (b -- )			not used		*/
 OP_END_CONST,		/* (b -- )			end of constant exprs.  */
 OP_ERROR,		/* (l -- b)			warn/fail, ruleid	*/
 OP_NOMORE,		/* ( -- )			end of byte-code program*/
 OP_EXT_FUNC,		/* (p p p -- r)			bre_extern=function info*/
 OP_EXT_LOOKUP,		/* (p p p -- l)			bre_extern=lookup info.	*/

 OP_MKLIST,		/* (x e e e e -- x l)		# of elements in list	*/

 OP_ASSIGN,		/* (x i -- x i)			n=fp offset to variable	*/

 OP_GETFP,		/* (x -- x i)			n=fp offset to var/param*/

 OP_GETSP,		/* (x d x -- x d x d)		n=offset from current SP*/
 OP_PUTSP,		/* (x d x v -- x v x)		n=offset from current SP*/
 OP_INCSP,		/* (x d x -- x d+1 x)		n=offset from current SP*/
 OP_DECSP,		/* (x d x -- x d+1 x)		n=offset from current SP*/

 OP_CONV_IF,		/* (x i -- x f)			not used		*/
 OP_CONV_FI,		/* (x i -- x f)			not used		*/

 OP_CONV_BS,		/* (x i -- x f)			not used		*/
 OP_CONV_IS,		/* (x i -- x f)			not used		*/
 OP_CONV_FS,		/* (x i -- x f)			not used		*/
 OP_CONV_TRI,		/* (x i -- x f)			convert table-row -> int*/

 OP_SET_ROW,		/* (x tr i -- tr)		assign table row to i	*/
 OP_SET_ROW_VERIFY,	/* (x tr i -- tr)		assign table row to i	*/

 OP_FETCH_LIST_I,	/* (l n -- i)			not used		*/
 OP_FETCH_LIST_F,	/* (l n -- f)			not used		*/
 OP_FETCH_LIST_S,	/* (l n -- s)			not used		*/
 OP_FETCH_FIELD,	/* (x tr -- x fv)		field=field to fetch	*/
 OP_FETCH_CHAR,		/* (x s n -- x s)		not used		*/

 OP_SUBSTR,		/* (x s n m -- x s)		not used		*/
 OP_SUBLST,		/* (x l n m -- x l)		not used		*/

 OP_MAX_I,		/* (x i i -- x i)					*/
 OP_MAX_F,		/* (x f f -- x f)					*/
 OP_MAX_S,		/* (x s s -- x s)					*/

 OP_MIN_I,		/* (x i i -- x i)					*/
 OP_MIN_F,		/* (x f f -- x f)					*/
 OP_MIN_S,		/* (x s s -- x s)					*/

 OP_COUNT_L,		/* (x -- x)						*/
 OP_COUNT_S,		/* (x -- x)						*/
 OP_COUNT_TR,		/* (x -- x)						*/

 OP_AND,		/* (b b -- b)						*/
 OP_OR,			/* (b b -- b)						*/

} OPCODE;

typedef struct {
	char		*opcode_name;
	OPCODE		op;
	unsigned char	b[3];
	short		s[3];
	long		l[3];
	double		d[3];
	char		*str[3];
	void		*ptr[3];
	BRE_DATUM	datum;
} PROGRAM_INSTRUCTION;

#define STACK_SIZE	500

/*
 * Description of a byte-code program.
 */
#define PROGRAM_GROW	2048

#define PAD_SHORT	sizeof(short)
#define PAD_LONG	sizeof(long)
#define PAD_DOUBLE	sizeof(double)
#define PAD_POINTER	sizeof(long)
#define PAD_DATUM	sizeof(long)

typedef struct {
	unsigned int	lc;		/* location counter */
	unsigned int	size;		/* current size of 'program' buffer */
	unsigned char	*program;	/* pointer to program */
} PROGRAM;

/*
 * This mode setting instructs the function 'gencode()' what to
 * do.
 *	FREEONLY	- Don't generate code, just free the parse tree.
 *	GENCODE		- Generate code and free up parse tree.
 *	CONSTFOLD	- instructs the lower levels that we are already
 *			  folding a constant expression, so don't do it again.
 *
 */
typedef enum {
	G_FREEONLY, G_GENCODE, G_CONSTFOLD
} GENCODE_MODE;


/*
 * Proto-types from bre_gen.c
 *
 */
extern void	gencode(GENCODE_MODE, PROGRAM *, EXPR *);

extern void	gencode_pop(GENCODE_MODE, PROGRAM *);
extern int	gencode_constant_entry(PROGRAM *);
extern void	gencode_constant_exit(PROGRAM *);
extern int	gencode_function_entry(PROGRAM *);
extern void	gencode_function_exit(PROGRAM *, int, int, int);
extern int	gencode_rule_entry(PROGRAM *);
extern void	gencode_rule_exit(PROGRAM *, SYMBOL *sym, EXPR *);

/*
 * Proto-types from bre_run.c
 *
 */
extern PROGRAM	*prog_new(void);
extern int	prog_free(PROGRAM *);
extern int	prog_get_lc(PROGRAM *);
extern void	prog_set_lc(PROGRAM *, int);
extern int	prog_encode(PROGRAM *, PROGRAM_INSTRUCTION *);
extern void	prog_patch_instruction(PROGRAM *, int, PROGRAM_INSTRUCTION *);
extern void	prog_decode(PROGRAM *, PROGRAM_INSTRUCTION *);
extern void	prog_print(FILE *, PROGRAM *);

extern void	run_evaluate_constant(PROGRAM *, int, BRE_DATUM *);
extern int	run_evaluate_rule(PROGRAM *, SYMBOL *sym);

extern int	mem_remove_all(void);

#endif
