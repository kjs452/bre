#ifndef _BRE_SYM_H
#define _BRE_SYM_H
/*----------------------------------------------------------------------
 * Symbol table structures.
 *
 *
 *
 */
typedef enum {
	T_UNKNOWN,
	T_MIXED,
	T_BOOLEAN,
	T_INTEGER,
	T_FLOAT,
	T_STRING,
	T_LIST,
	T_ROW,
	T_REC
} T_FLAGS;

typedef struct {
	T_FLAGS	primary;
	union {
		T_FLAGS		subtype;	/* used when primary==T_LIST */
		struct symbol	*symbol;	/* used when primary==T_ROW,T_REC */
	} u;
} SYM_TYPE;

typedef struct sym_external {
	int	refcount;
	void	*external_cookie;
	char	*args_in;
	char	*args_out;
	BRE_EXTERNAL_CB *func;
} SYM_EXTERNAL;

typedef struct {
	int		refcount;
	SYM_TYPE	ret_type;
	int		entry_point;
	int		nlocals;
	int		nargs;
	struct sym_list	*parms;
	struct sym_list	*vars;
} SYM_FUNCTION;

typedef struct {
	int		refcount;
	struct symbol	*using;
	int		entry_point;
} SYM_GROUP;

typedef struct {
	int		refcount;
	int		entry_point;
	SYM_TYPE	type;
} SYM_CONSTANT;

typedef struct {
	int		entry_point;
	int		nlocals;
	BRE_DIAGNOSTIC	diag;
	SYM_TYPE	ret_type;
	struct symbol	*using;
	struct sym_list	*vars;
} SYM_RULE;

typedef struct sym_table {
	int		table_id;
	int		refcount;
	void		*table_cookie;
} SYM_TABLE;

typedef struct {
	int		refcount;
	int		length;
	SYM_TYPE	type;
	void		*field_cookie;
} SYM_FIELD;

typedef struct {
	int		refcount;
	int		offset;
	SYM_TYPE	type;
} SYM_LUFIELD;

typedef struct {
	int		refcount;
	SYM_TYPE	type;
	int		fp;
} SYM_PARAMETER;

typedef struct {
	int		refcount;
	int		assigned;
	SYM_TYPE	type;
	int		fp;
} SYM_VARIABLE;

typedef enum {
	S_EXTFUNC,
	S_EXTLOOKUP,
	S_CONSTANT,
	S_FUNCTION,
	S_GROUP,
	S_RULE,
	S_TABLE,
	S_FIELD,
	S_LUFIELD,
	S_PARAMETER,
	S_VARIABLE
} SYM_KIND;

typedef struct symbol {
	char		*name;
	SYM_KIND	kind;
	union {
		SYM_EXTERNAL	ext;
		SYM_CONSTANT	con;
		SYM_FUNCTION	fun;
		SYM_GROUP	grp;
		SYM_RULE	rul;
		SYM_TABLE	tab;
		SYM_FIELD	fld;
		SYM_PARAMETER	par;
		SYM_VARIABLE	var;
		SYM_LUFIELD	luf;
	} u;
	struct symbol *next;
} SYMBOL;

typedef struct sym_list {
	SYMBOL		*sym;
	struct sym_list	*next;
} SYMBOL_LIST;

/*
 * Proto-types
 *
 */
extern SYMBOL	*sym_lookup(char*);
extern SYMBOL	*sym_insert(char*, SYM_KIND);

extern SYMBOL	*sym_lookup_pair(char*, char*);
extern SYMBOL	*sym_insert_pair(char *, char*, SYM_KIND);

extern void	sym_remove(SYMBOL *);
extern SYMBOL_LIST *sym_detach_kind(SYM_KIND);
extern void	sym_detach(SYMBOL *);
extern void	sym_reattach(SYMBOL *);

extern void	sym_stats(int *, int *, int *, int *, int *);

extern void	sym_extract_pair(char *, char *, char *);

typedef struct {
	int	curr_hv;
	SYMBOL	*curr_sym;
} SYMBOL_CURSOR;

extern SYMBOL	*sym_first(SYMBOL_CURSOR *);
extern SYMBOL	*sym_next(SYMBOL_CURSOR *);

extern void	symlist_add(SYMBOL_LIST **, SYMBOL*);
extern void	symlist_detach_all(SYMBOL_LIST *);
extern void	symlist_remove_all(SYMBOL_LIST **);

extern int	symbol_match(char *, char *);

extern BRE_EXTERNAL *sym_find_external(char *);

#endif
