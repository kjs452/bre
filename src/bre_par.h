#ifndef _BRE_PAR_H
#define _BRE_PAR_H
/*----------------------------------------------------------------------
 * Parse Tree Data Structures.
 *
 *
 */
typedef enum {
	E_NONE,
	E_FORALL,
	E_FORSOME,
	E_COMPUTE,
	E_IF,
	E_IFELSE,
	E_OR,
	E_AND,
	E_IN,
	E_BETWEEN,
	E_LIKE,
	E_NIN,
	E_NBETWEEN,
	E_NLIKE,
	E_EQ,
	E_NE,
	E_LT,
	E_LE,
	E_GT,
	E_GE,
	E_CONCAT,
	E_ADD,
	E_ADD_LIST,
	E_SUB,
	E_MUL,
	E_DIV,
	E_MOD,
	E_NOT,
	E_NEG,
	E_COUNT,
	E_SUM,
	E_MIN,
	E_MAX,
	E_CALL,
	E_INTEGER,
	E_FLOAT,
	E_STRING,
	E_BOOLEAN,
	E_NULL,
	E_FIELD,
	E_OLDFIELD,
	E_LUFIELD,
	E_VARIABLE,
	E_SUBSTR,
	E_SUBLST,
	E_LOOKUP_INDEX,
	E_TABLE_INDEX,
	E_LIST_INDEX,
	E_LIST,
	E_WARN,
	E_FAIL,
	E_ASSIGN,
	E_BLOCK,
	E_CONSTANT,
	E_GROUP,
	E_DOT,
	E_IMPLICIT,
	E_ROWNUM
} EXPR_OPERATOR;

typedef struct expr {
	EXPR_OPERATOR	op;

	SYM_TYPE	type;
	int		constant;

	SYMBOL		*symbol;
	BRE_DATUM	value;

	struct expr	*left, *right;
} EXPR;

/*----------------------------------------------------------------------
 * ID_LIST
 *
 */
typedef struct id_list {
	char		*id;
	struct id_list	*next;
} ID_LIST;

/* ----------------------------------------------------------------------
 * The yacc stack.
 *
 */
union yystack {
	long		l;
	double		d;
	char		*str;
	EXPR		*expr;
	ID_LIST		*idlist;
	EXPR_OPERATOR	op;
	SYM_TYPE	type;
};
extern union yystack yylval;

/*
 *
 * Proto-types
 *
 */
extern EXPR	*expr_if(EXPR *, EXPR *);
extern EXPR	*expr_ifelse(EXPR *, EXPR *, EXPR *);
extern EXPR	*expr_logical(EXPR_OPERATOR, EXPR *, EXPR *);
extern EXPR	*expr_in(EXPR_OPERATOR, EXPR *, EXPR *);
extern EXPR	*expr_between(EXPR_OPERATOR, EXPR *, EXPR *);
extern EXPR	*expr_like(EXPR_OPERATOR, EXPR *, EXPR *);
extern EXPR	*expr_equality(EXPR_OPERATOR, EXPR *, EXPR *);
extern EXPR	*expr_relational(EXPR_OPERATOR, EXPR *, EXPR *);
extern EXPR	*expr_add(EXPR *, EXPR *);
extern EXPR	*expr_arithmetic(EXPR_OPERATOR, EXPR *, EXPR *);
extern EXPR	*expr_grpfunc(EXPR_OPERATOR, EXPR *);
extern EXPR	*expr_grpfunc_sa(EXPR_OPERATOR, EXPR *);
extern EXPR	*expr_call(char *, EXPR *);
extern EXPR	*expr_integer(long);
extern EXPR	*expr_float(double);
extern EXPR	*expr_string(char *);
extern EXPR	*expr_boolean(long);
extern EXPR	*expr_null(void);
extern EXPR	*expr_args(EXPR *, EXPR *);
extern EXPR	*expr_list(EXPR *);
extern EXPR	*expr_substr(EXPR *, EXPR *);
extern EXPR	*expr_old(EXPR *);
extern EXPR	*expr_rownum(EXPR *);

extern EXPR	*expr_variable(char *, EXPR *);
extern EXPR	*expr_indexed_table(char *, EXPR *);
extern EXPR	*expr_tablefield(char *, char *, EXPR *);
extern EXPR	*expr_indexed_tablefield(char *, EXPR *, char *, EXPR *);

extern EXPR	*expr_for(EXPR_OPERATOR, EXPR *, EXPR *);
extern EXPR	*expr_compute(EXPR *, EXPR *, EXPR *);
extern EXPR	*expr_for_idlist(ID_LIST *);
extern EXPR	*expr_iterator(EXPR_OPERATOR, EXPR *, EXPR *);

extern EXPR	*expr_error(EXPR_OPERATOR, EXPR *);

extern EXPR	*expr_assign(char *, EXPR *);
extern EXPR	*expr_block(EXPR *);

extern ID_LIST	*idlist(ID_LIST *, char *);
extern char	*make_ruleid(long);

extern SYM_TYPE	basic_type(int);
extern SYM_TYPE	list_type(int);
extern SYM_TYPE	complex_type(char *);
extern SYM_TYPE	complex_list_type(char *);
extern void	declare_type(ID_LIST *, SYM_TYPE);
extern SYM_TYPE	type_from_char(char);

extern void	declare_rule(char *, char *);
extern void	declare_func(char *, ID_LIST *);
extern void	declare_group(char *, char *);
extern void	declare_constant(char *, EXPR *);
extern void	terminate_func(void);
extern void	terminate_rule(EXPR *);
extern void	terminate_group(EXPR *);

extern void	declare_extfunc(char *, ID_LIST *);
extern void	declare_lookup(char *, ID_LIST *, ID_LIST *);

extern void	statement(EXPR *);

#endif

