#ifndef _BRE_PRV_H
#define _BRE_PRV_H
/*----------------------------------------------------------------------
 * PRIVATE HEADER file for all source files BRE source files
 *
 */
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <string.h>
#include <assert.h>

#include "msgget.h"

typedef struct bre_external {
	char		*name;
	void		*external_cookie;
	char		*args_in;
	char		*args_out;
	BRE_EXTERNAL_CB	*func;
	struct bre_external *next;
} BRE_EXTERNAL;

#include "bre_api.h"
#include "bre_sym.h"
#include "bre_yac.h"
#include "bre_par.h"
#include "bre_ops.h"
#include "bre_bit.h"
#include "msg060.h"
#include "bre_err.h"

/***********************************************************************
 * Largest size a string or variable can be.
 */
#define MAX_TOKEN_SIZE	1000

#define BRE_ASSERT(e)	assert(e)

/*
 * Memory allocation functions.
 */
#define BRE_MALLOC(x)	malloc(x)
#define BRE_REALLOC(p,s) realloc(p,s)
#define BRE_FREE(x)	free(x)
#define BRE_STRDUP(x)	bre_strdup(x)

/***********************************************************************
 * These global variables are used by various parts of the editengine
 * library. To avoid corrupting the name space they are placed in
 * this structure.
 */
struct bre_globals {
	jmp_buf		jmpbuf;
	char		errorbuf[ 1024 ];
	int		lineno;
	int		lpos;
	int		cpos;

	char		**filenames;
	int		file_count;
	FILE		*fp;
	FILE		*fp_diag;
	char		*input_str;

	int		debug_mode;

	int		eval_time;
	int		nevals;

	int		last_instr_lc;
	PROGRAM		*prog;

	char		*fatal_error;

	BRE_EXTERNAL	*externals;

	BRE_REFERENCED_CB	*referenced_cb;
	BRE_ROWCOUNT_CB		*rowcount_cb;
	BRE_ROWCHANGED_CB	*rowchanged_cb;
	BRE_FIELDQUERY_CB	*fieldquery_cb;
	BRE_RULEDIAG_CB		*rulediag_cb;
};

extern struct bre_globals *BRE(void);

/*
 * Proto-types for general BRE functions
 *
 */
extern char *bre_strdup(char *);
extern void bre_register_builtins_private(int, FILE *);

#endif
