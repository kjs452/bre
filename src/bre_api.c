/***********************************************************************
 * BUSINESS RULE ENGINE - API INTERFACE.
 *
 */
#include <sys/types.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <setjmp.h>
#include <stdarg.h>
#include <assert.h>
#include <ctype.h>
#include <time.h>

#include "bre_api.h"
#include "bre_prv.h"

/*
 * This function is used to access the global variables.
 * (Needed mainly for the MPE environment)
 */
static struct bre_globals BRE_GLOBALS;
struct bre_globals *BRE(void)
{
	return &BRE_GLOBALS;
}

/*
 * Perform the strdup() function. MPE does not
 * have strdup(), so we implement it here.
 */
char *bre_strdup(char *str)
{
	char *new_str;

	new_str = (char*)BRE_MALLOC( strlen(str)+1 );
	return strcpy(new_str, str);
}

/*
 * Call bre_marktime() to note the current time.
 * Then call bre_difftime() to return the elapsed
 * number of seconds.
 *
 */
static time_t Start;
static void bre_marktime(void)
{
	Start = time(NULL);
}

static int bre_difftime(void)
{
	time_t end;

	end = time(NULL);

	return end - Start;
}

/*-----------------------------------------------------------------------------
| Purpose:  To tell the business rule engine that the given name corresponds
|           to a 'table'.
*-----------------------------------------------------------------------------*/
   void
bre_register_table(
	const char     *table_name,		/* I */
	const void     *table_cookie)	/* I Optional client-defined value */
{
	SYMBOL *sym;

	BRE_ASSERT(table_name != NULL);

	sym = sym_lookup((char*)table_name);
	if( sym == NULL ) {
		sym = sym_insert((char*)table_name, S_TABLE);
	}

	sym->u.tab.refcount	= 0;
	sym->u.tab.table_cookie	= (void*)table_cookie;
}

/*-----------------------------------------------------------------------------
| Purpose:  To tell the business rule engine that the given field name
|           is associated with the given table name.
*-----------------------------------------------------------------------------*/
   void
bre_register_table_field(
	const char     *table_name,		/* I */
	const char     *field_name,		/* I */
	char     field_type,			/* I (B, I, F, S) */
	unsigned int   field_length,		/* I Meaningful for S only */
	const void     *field_cookie)		/* I Optional client-defined value */
{
	SYMBOL *sym;

	BRE_ASSERT(table_name != NULL);
	BRE_ASSERT(field_name != NULL);

	sym = sym_lookup_pair((char*)table_name, (char*)field_name);
	if( sym == NULL ) {
		sym = sym_insert_pair((char*)table_name, (char*)field_name, S_FIELD);
	}

	sym->u.fld.refcount	= 0;
	sym->u.fld.field_cookie	= (void*)field_cookie;
	switch( field_type ) {
	case 'B':
		sym->u.fld.type.primary = T_BOOLEAN;
		break;
	case 'I':
		sym->u.fld.type.primary = T_INTEGER;
		break;
	case 'F':
		sym->u.fld.type.primary = T_FLOAT;
		break;
	case 'S':
		sym->u.fld.type.primary = T_STRING;
		break;
	default:
		BRE_ASSERT(0);
	}

	sym->u.fld.length = field_length;
}

/*-----------------------------------------------------------------------------
| Purpose:  To register a callback procedure with th business rule engine. This
|	    callback defines a new function or lookup table that can be accessed
|	    from the business rule language.
|
|	    The caller must register all functions/lookup tables before
|	    compiling a rule file.
|
| name      - name of the function or lookup table.
| external_cookie - Client-defined value.
| args_in   - A string describing the # of parameters and types
|             of each parameter.
| args_out  - A string the specifies the return value(s). A function
|             has only 1 return value, a lookuptable has 1 or more.
|
|  To specify 'args_in' or 'args_out' use this key
|     'B'   - Boolean      BRE_INTEGER
|     'I'   - Integer      BRE_INTEGER
|     'F'   - Float        BRE_FLOAT
|     'S'   - String       BRE_STRING
*---------------------------------------------------------------------------*/
   void
bre_register_external(
   const char     *name,            /* I */
   const void     *external_cookie,  /* I - client-defined value */
   const char     *args_in,         /* I */
   const char     *args_out,        /* I */
   BRE_EXTERNAL_CB *function)      /* I callback function */
{
	BRE_EXTERNAL *newext, *curr, *prev;
	const char *p;
	char *type_string;

	BRE_ASSERT(name != NULL);
	BRE_ASSERT(args_in != NULL);
	BRE_ASSERT(args_out != NULL);
	BRE_ASSERT(function != NULL);

	type_string = "BIFS";
	for(p=args_in; *p; p++)
		BRE_ASSERT(strchr(type_string, *p) != NULL);

	for(p=args_out; *p; p++)
		BRE_ASSERT(strchr(type_string, *p) != NULL);

	newext = (BRE_EXTERNAL*)BRE_MALLOC( sizeof(BRE_EXTERNAL) );
	if( newext == NULL ) {
		Bre_Error(BREMSG_DIE_NOMEM);
	}

	newext->name		= BRE_STRDUP((char*)name);
	newext->external_cookie	= (void*)external_cookie;
	newext->args_in		= BRE_STRDUP((char*)args_in);
	newext->args_out	= BRE_STRDUP((char*)args_out);
	newext->func		= function;
	newext->next		= NULL;

	prev = NULL;
	for(curr=BRE()->externals; curr; curr=curr->next)
		prev=curr;

	if( prev == NULL )
		BRE()->externals = newext;
	else
		prev->next = newext;
}

/*----------------------------------------------------------------------
 * Remove all extraneous symbols from
 * the symbol table.
 *	- External function declarations
 *	- External lookup declarations
 *	- Constants
 *	- Functions.
 *	- Groups.
 *
 */
static void parse_cleanup(void)
{
	SYMBOL_LIST *head, *curr, *nxt, *curr2, *nxt2;
	SYMBOL *sym, *sym2;

	head = sym_detach_kind(S_EXTFUNC);
	for(curr=head; curr; curr=nxt) {
		sym = curr->sym;
		BRE_FREE(sym->name);
		BRE_FREE(sym->u.ext.args_in);
		BRE_FREE(sym->u.ext.args_out);
		BRE_FREE(sym);
		nxt=curr->next;
		BRE_FREE(curr);
	}

	head = sym_detach_kind(S_EXTLOOKUP);
	for(curr=head; curr; curr=nxt) {
		sym = curr->sym;
		BRE_FREE(sym->name);
		BRE_FREE(sym->u.ext.args_in);
		BRE_FREE(sym->u.ext.args_out);
		BRE_FREE(sym);
		nxt=curr->next;
		BRE_FREE(curr);
	}

	head = sym_detach_kind(S_LUFIELD);
	for(curr=head; curr; curr=nxt) {
		sym = curr->sym;
		BRE_FREE(sym->name);
		BRE_FREE(sym);
		nxt=curr->next;
		BRE_FREE(curr);
	}

	head = sym_detach_kind(S_CONSTANT);
	for(curr=head; curr; curr=nxt) {
		sym = curr->sym;
		BRE_FREE(sym->name);
		BRE_FREE(sym);
		nxt=curr->next;
		BRE_FREE(curr);
	}

	head = sym_detach_kind(S_FUNCTION);
	for(curr=head; curr; curr=nxt) {
		sym = curr->sym;

		/*
		 * Free the parameters of this function.
		 */
		for(curr2=sym->u.fun.parms; curr2; curr2=nxt2) {
			sym2 = curr2->sym;
			BRE_FREE(sym2->name);
			BRE_FREE(sym2);
			nxt2 = curr2->next;
			BRE_FREE(curr2);
		}

		BRE_FREE(sym->name);
		BRE_FREE(sym);
		nxt=curr->next;
		BRE_FREE(curr);
	}

	head = sym_detach_kind(S_GROUP);
	for(curr=head; curr; curr=nxt) {
		sym = curr->sym;
		BRE_FREE(sym->name);
		BRE_FREE(sym);
		nxt=curr->next;
		BRE_FREE(curr);
	}
}

/*
 * Add a filename to the BRE()->filenames[] array.
 *
 */
static void add_filename(char *filename)
{
	char *filename_dup;
	char **array;
	int cnt;

	filename_dup = BRE_STRDUP(filename);
	if( filename_dup == NULL )
		Bre_Error(BREMSG_DIE_NOMEM);

	cnt	= BRE()->file_count + 1;
	array	= BRE()->filenames;

	if( array == NULL )
		array = (char**)BRE_MALLOC( sizeof(char*) * cnt );
	else
		array = BRE_REALLOC(array, sizeof(char*)*cnt);

	if( array == NULL )
		Bre_Error(BREMSG_DIE_NOMEM);

	array[cnt-1] = filename_dup;

	BRE()->file_count	= cnt;
	BRE()->filenames	= array;
}

/*
 * Free the array of parsed filenames.
 */
static void free_filenames(void)
{
	char **array;
	int cnt, i;

	cnt	= BRE()->file_count;
	array	= BRE()->filenames;

	for(i=0; i<cnt; i++) {
		BRE_FREE(array[i]);
	}

	if( array != NULL ) {
		BRE_FREE(array);
	}
}

/*-----------------------------------------------------------------------------
| Purpose:  To tell the business rule engine to compile the rules found
|           in the given rule_file. Diagnostic messages will be written
|           to the diagnostics_file in the event that errors are encountered.
| Returns:  BRE_SUCCESS - Success, BRE_FAIL - Failure
*-----------------------------------------------------------------------------*/
   BRE_DIAGNOSTIC
bre_compile_rules(
	const char  *rule_file,		/* I */
	const char  *diagnostics_file)	/* I */
{
	int n, lc;
	FILE *fp, *fp_diag;
	char tbuf[200], fbuf[200];
	SYMBOL *sym, *sym2;
	PROGRAM_INSTRUCTION instr;
	SYMBOL_CURSOR sc;

	BRE_ASSERT(rule_file != NULL);
	BRE_ASSERT(diagnostics_file != NULL);

	fp_diag = fopen(diagnostics_file, "w");
	if( fp_diag == NULL ) {
		sprintf(BRE()->errorbuf, "%s: %s", diagnostics_file, strerror(errno));
		return BRE_FAIL;
	}

	fp = fopen(rule_file, "r");
	if( fp == NULL ) {
		fprintf(fp_diag, "%s: %s\n", rule_file, strerror(errno));
		return BRE_FAIL;
	}

	/*
	 * Setup the BRE globals for parsing/compiling.
	 */
	add_filename((char*)rule_file);
	BRE()->fp_diag	= fp_diag;
	BRE()->fp	= fp;
	BRE()->input_str = NULL;
	BRE()->lineno	= 1;
	BRE()->lpos	= 0;
	BRE()->cpos	= 0;
	BRE()->errorbuf[0] = '\0';

	if( getenv("BRE_DEBUG") != NULL )
		BRE()->debug_mode = 1;
	else
		BRE()->debug_mode = 0;

	/*
	 * Set location counter for the byte code compiler.
	 */
	if( BRE()->prog == NULL ) {
		BRE()->prog = prog_new();
	} else {
		lc = BRE()->last_instr_lc;
		prog_set_lc(BRE()->prog, lc);
	}

	if( setjmp(BRE()->jmpbuf) ) {
		fclose(fp);
		fclose(fp_diag);
		BRE()->fp	= NULL;
		BRE()->fp_diag	= NULL;
		parse_cleanup();
		return BRE_FAIL;
	}

	bre_clear_errors();

	yyparse();

	/*
	 * Terminate the program with a 'OP_NOMORE' instruction.
	 */
	instr.op = OP_NOMORE;
	lc = prog_encode(BRE()->prog, &instr);
	BRE()->last_instr_lc = lc;

	fclose(fp);
	fclose(fp_diag);
	BRE()->fp	= NULL;
	BRE()->fp_diag	= NULL;

	parse_cleanup();

	if( bre_error_count() > 0 )
		return BRE_FAIL;

	/*
	 * Invoke the BRE_REFERENCED_CB for all fields that were used.
	 */
	if( BRE()->referenced_cb ) {
		for(sym=sym_first(&sc); sym; sym=sym_next(&sc)) {
			if( sym->kind != S_FIELD )
				continue;

			if( sym->u.fld.refcount == 0 )
				continue;

			sym_extract_pair(sym->name, tbuf, fbuf);

			sym2 = sym_lookup(tbuf);
			BRE_ASSERT(sym2 != NULL);

			BRE()->referenced_cb(tbuf, sym2->u.tab.table_cookie,
					fbuf, sym->u.fld.field_cookie);

			sym->u.fld.refcount = 0;
		}
	}

	return BRE_SUCCESS;
}

/*-----------------------------------------------------------------------------
| Following are prototypes for the functions that allow 'registration'
| of the callback functions listed below.
*-----------------------------------------------------------------------------*/

   void
bre_register_tell_field_referenced(
	BRE_REFERENCED_CB	*tell_field_reference)
{
	BRE()->referenced_cb = tell_field_reference;
}

   void
bre_register_query_table_row_count(
	BRE_ROWCOUNT_CB		*query_table_row_count)
{
	BRE()->rowcount_cb = query_table_row_count;
}

   void
bre_register_query_row_change(
	BRE_ROWCHANGED_CB	*query_row_change)
{
	BRE()->rowchanged_cb = query_row_change;
}

   void
bre_register_query_field_data(
	BRE_FIELDQUERY_CB	*query_field_data)
{
	BRE()->fieldquery_cb = query_field_data;
}

   void
bre_register_emit_rule_diagnostic(
	BRE_RULEDIAG_CB		*emit_rule_diagnostic)
{
	BRE()->rulediag_cb = emit_rule_diagnostic;
}

static void clear_fatal_error(void)
{
	if( BRE()->fatal_error != NULL ) {
		BRE_FREE( BRE()->fatal_error );
		BRE()->fatal_error = NULL;
	}
}

/*-----------------------------------------------------------------------------
| Purpose:  To be called from inside of external callback functions to propogate
|	    a fatal error back to the caller (see the BRE_FATAL return code)
|
|           When this function is used, it will cause the rule to fail with the
|           diagnostics error code of BRE_FATAL. The token list will consists of
|           a single string.
|
*-----------------------------------------------------------------------------*/
   void
bre_fatal(
	const char *fatal_error)	/* I - fatal error message string */
{

	BRE_ASSERT( fatal_error != NULL );

	clear_fatal_error();
	BRE()->fatal_error = BRE_STRDUP((char*)fatal_error);
}

/*-----------------------------------------------------------------------------
| Purpose:  To tell apply the 'currently defined' business rules against
|           the 'current' table data.
| Returns:  BRE_SUCCESS - Success, BRE_WARN - some warnings, BRE_FAIL - some failures
|           BRE_FATAL - a serious error occured with an external function or lookup
*-----------------------------------------------------------------------------*/
   BRE_DIAGNOSTIC
bre_evaluate_rules(void)
{
	SYMBOL *sym;
	SYMBOL_CURSOR sc;
	PROGRAM_INSTRUCTION instr;
	int n;
	int warnings, failures, fatals;
	int diff;

	BRE_ASSERT( BRE()->prog != NULL );
	BRE_ASSERT( BRE()->rowcount_cb != NULL );
	BRE_ASSERT( BRE()->fieldquery_cb != NULL );

	bre_marktime();

	warnings = 0;
	failures = 0;
	fatals	= 0;
	for(sym = sym_first(&sc); sym; sym=sym_next(&sc)) {
		if( sym->kind != S_RULE )
			continue;

		n = run_evaluate_rule(BRE()->prog, sym);

		if( BRE()->fatal_error ) {
			fatals += 1;
			clear_fatal_error();
		} else if( n ) {
			if( sym->u.rul.diag == BRE_FAIL )
				failures += 1;
			else
				warnings += 1;
		}
	}

	diff = bre_difftime();

	BRE()->nevals		+= 1;
	BRE()->eval_time	+= diff;

	if( BRE()->debug_mode ) {
		printf("BRE evaluation #%d: %d sec., total: %d sec.\n",
					BRE()->nevals, diff, BRE()->eval_time);
	}

	if( fatals )
		return BRE_FATAL;
	else if( failures )
		return BRE_FAIL;
	else if( warnings )
		return BRE_WARN;
	else
		return BRE_SUCCESS;
}

/*-----------------------------------------------------------------------------
| Purpose:  Free's all data structures, byte-code programs, symbols etc..
|           After calling this function all memory should have have been free'd
|	    If 'debug_file' is non-NULL, many useful statistics are written to
|	    the debug_file.
|
*-----------------------------------------------------------------------------*/
   void
bre_shutdown(
	const char *debug_file)
{
	int nsymbols, nslots, nchains, min_chain, max_chain;
	int ntables, nfields, nrules, nexternals, tmpspace, end_lc;
	int prog_size, mem_total, symtab_size, ext_size;
	double avg_seconds;
	SYMBOL_LIST *head, *curr, *nxt;
	SYMBOL *sym;
	SYMBOL_CURSOR sc;
	BRE_EXTERNAL *ecurr, *enxt;
	FILE *fp;

	sym_stats(&nsymbols, &nslots, &nchains, &min_chain, &max_chain);

	if( debug_file )
		fp = fopen(debug_file, "w");
	else
		fp = NULL;

	if( fp ) {
		/*
		 * Print the byte-code program.
		 */
		if( BRE()->prog ) {
			prog_print(fp, BRE()->prog);
			end_lc = prog_get_lc(BRE()->prog);
		} else {
			fprintf(fp, "\nNo compiled byte-code program.\n");
			end_lc = 0;
		}

		fprintf(fp, "\nAvailable BRE Builtin Functions/Lookups:\n");
		bre_register_builtins_private(0, fp);
		fprintf(fp, "\n");
	}

	/***********************************************************************
	 * The following stuff needs to be free'd:
	 *
	 *	1) Symbol Table entries.
	 *	2) Byte-Code program.
	 *	3) List of external objects.
	 *	4) Temporary storage used by the byte-code interpreter.
	 *	5) List of parsed filenames.
	 *
	 */

	/* ----------------------------------------------------------------------
	 * 1) Free All Symbol Table entires:
	 *	parse_cleanup()	- Takes care of all symbols except TABLE's,
	 *			FIELD's and RULE's
	 */
	parse_cleanup();

	symtab_size = 0;
	/*
	 * Eliminate S_TABLE entries.
	 */
	ntables = 0;
	head = sym_detach_kind(S_TABLE);
	for(curr=head; curr; curr=nxt) {
		sym = curr->sym;
		symtab_size += sizeof(SYMBOL) + strlen(sym->name);
		BRE_FREE(sym->name);
		BRE_FREE(sym);
		nxt=curr->next;
		BRE_FREE(curr);
		ntables += 1;
	}

	/*
	 * Eliminate S_FIELD entries.
	 */
	nfields = 0;
	head = sym_detach_kind(S_FIELD);
	for(curr=head; curr; curr=nxt) {
		sym = curr->sym;
		symtab_size += sizeof(SYMBOL) + strlen(sym->name);
		BRE_FREE(sym->name);
		BRE_FREE(sym);
		nxt=curr->next;
		BRE_FREE(curr);
		nfields += 1;
	}

	/*
	 * Eliminate S_RULE entries.
	 */
	nrules = 0;
	head = sym_detach_kind(S_RULE);
	for(curr=head; curr; curr=nxt) {
		sym = curr->sym;
		symtab_size += sizeof(SYMBOL) + strlen(sym->name);
		BRE_FREE(sym->name);
		BRE_FREE(sym);
		nxt=curr->next;
		BRE_FREE(curr);
		nrules += 1;
	}

	sym = sym_first(&sc);
	BRE_ASSERT( sym == NULL );

	/* ----------------------------------------------------------------------
	 * 2) Free the Byte-code program:
	 *
	 */
	if( BRE()->prog != NULL ) {
		prog_size = prog_free(BRE()->prog);
		BRE()->prog = NULL;
	}

	/*----------------------------------------------------------------------
	 * 3) Free the list of external objects.
	 *
	 */
	ext_size = 0;
	nexternals = 0;
	for(ecurr=BRE()->externals; ecurr; ecurr=enxt) {
		ext_size += sizeof(BRE_EXTERNAL) + strlen(ecurr->name)
						 + strlen(ecurr->args_in)
						 + strlen(ecurr->args_out);
		BRE_FREE(ecurr->name);
		BRE_FREE(ecurr->args_in);
		BRE_FREE(ecurr->args_out);
		enxt = ecurr->next;
		BRE_FREE(ecurr);
		nexternals += 1;
	}
	BRE()->externals = NULL;

	/*----------------------------------------------------------------------
	 * 4) Free the temporary storage used by the byte-code interpreter.
	 *    
	 */
	tmpspace = mem_remove_all();

	/*----------------------------------------------------------------------
	 * 5) Free list of parsed filenames.
	 */
	free_filenames();

	/*----------------------------------------------------------------------
	 * 6) Free any fatal error message string.
	 */
	clear_fatal_error();

	/*
	 * Print some statistics about the BRE library.
	 */
	if( fp ) {
		fprintf(fp, "General Statistics:\n");
		fprintf(fp, "# of symbols: %d, # of slots: %d, # of chains: %d\n",
				nsymbols, nslots, nchains);
		fprintf(fp, "Smallest chain: %d, Longest chain: %d, Average chain: %.3f\n",
				min_chain, max_chain, (double)nsymbols/(double)nchains );

		fprintf(fp, "%12d business rules.\n", nrules);
		fprintf(fp, "%12d tables.\n", ntables);
		fprintf(fp, "%12d fields.\n", nfields);
		fprintf(fp, "%12d external functions & lookups.\n", nexternals);
		fprintf(fp, "%12d bytes needed for temporary storage.\n", tmpspace);
		fprintf(fp, "%12d Ending Location Counter for Byte-Code program\n\n", end_lc);

		fprintf(fp, "Approximate Memory Requirements:\n");

		mem_total = symtab_size;
		fprintf(fp, "%12d Symbol table data\n", symtab_size);
		mem_total += ext_size;
		fprintf(fp, "%12d External functions & lookups\n", ext_size);
		mem_total += tmpspace;
		fprintf(fp, "%12d Temporary storage\n", tmpspace);
		mem_total += prog_size;
		fprintf(fp, "%12d Byte-code program\n", prog_size);

		fprintf(fp, "%12d TOTAL BYTES\n\n", mem_total);

		if( BRE()->nevals )
			avg_seconds =  (double)BRE()->eval_time / (double)BRE()->nevals;
		else
			avg_seconds = 0.0;

		fprintf(fp, "Performance:\n");
		fprintf(fp, "%12d Elapsed time (sec.)\n", BRE()->eval_time);
		fprintf(fp, "%12d # of evaluations\n", BRE()->nevals);
		fprintf(fp, "%8.4f seconds/evaluation\n", avg_seconds);

		fclose(fp);
	}
}

