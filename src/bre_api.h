#ifndef _BRE_API_H
#define _BRE_API_H

/* CUSTOMIZATIONS BELOW */
/*-----------------------------------------------------------------------------
| Business Rule Engine (BRE)
|
| This is the public header file for the BRE library.
|
|
|
*----------------------------------------------------------------------------*/

/*
 * The basic data types
 *
 */
union bre_type;
struct sym_table;

typedef struct {
   short    nil;
   long     val;
} BRE_INTEGER;

typedef struct {
   double      val;
} BRE_FLOAT;

typedef struct {
   short    len;
   char     *str;
} BRE_STRING;

typedef struct {
   short          len;
   union bre_type *list;
} BRE_LIST;

typedef struct {
   short          nil;
   short          row;
   struct symbol  *table;
} BRE_TABLEROW;

typedef union bre_type {
   BRE_INTEGER    i;
   BRE_FLOAT      f;
   BRE_STRING     s;
   BRE_LIST       l;
   BRE_TABLEROW   tr;
} BRE_DATUM;

typedef enum { BRE_SUCCESS, BRE_WARN, BRE_FAIL, BRE_FATAL } BRE_DIAGNOSTIC;

/*-----------------------------------------------------------------------------
| Callback procedures needed by the BRE library.
|
|
*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
| Callback prototype
| Purpose:  For each external function or lookup table added to the bre
|      language will be accessed using a callback procedure of this
|      type. These callback procedures are added to the bre_externals()
|      array.
|
| Calls to this function are made during evaluation of a rule in which
| the external function was referenced in the rule.
|
*-----------------------------------------------------------------------------*/
typedef int (BRE_EXTERNAL_CB)(
   const char *name,			/* I - name of external object */
   const void *external_cookie,		/* I - client-defined value */
   BRE_DATUM *in,       /* I - input paramters */
   BRE_DATUM *out);     /* O - output parameters */

/*-----------------------------------------------------------------------------
| Callback prototype - Optional
| Purpose:  To let the client know which fields are referenced by the
|           current business rules. This information may be needed by
|           the client so that it can ensure that values for these fields
|           will always be available at rule evaluation time.
|
| Calls to this function are made as a result of a successful call to
| 'bre_compile_rules()'.
*-----------------------------------------------------------------------------*/
typedef void (BRE_REFERENCED_CB)(
   const char     *table_name,      /* I */
   const void     *table_cookie,    /* I client-defined value */
   const char     *field_name,      /* I */
   const void     *field_cookie);   /* I client-defined value */

/*-----------------------------------------------------------------------------
| Callback prototype - Required
| Purpose:  To indicate (to bre) the number of rows of data that exist for
|           the given table.
|
| This function is called (by bre) for each known table early from within
| 'bre_evaluate_rules()'.
*-----------------------------------------------------------------------------*/
typedef unsigned int (BRE_ROWCOUNT_CB)(
   const char     *table_name,      /* I */
   const void     *table_cookie);   /* I client-defined value */

/*-----------------------------------------------------------------------------
| Callback prototype - Optional
| Purpose:  To determine whether the given table / row was 'changed'.
|           Many business rules will not be applied against table rows that
|           have not changed.
|           If no 'query_row_change()' function is registered, the business
|           rule engine will assume that all table rows have changed.
| Returns:  1 - changed, 0 - unchanged
|
| This function will be called (by bre) for each row in each table, as
| determined by the value returned by 'query_table_row_count()'.
*-----------------------------------------------------------------------------*/
typedef int (BRE_ROWCHANGED_CB)(
   const char     *table_name,      /* I */
   const void     *table_cookie,    /* I client-defined value */
   unsigned int   row_number);      /* I */

/*-----------------------------------------------------------------------------
| Callback prototype - Required
| Purpose:  To provide (to bre) the value of a field for a given table / row.
|
| This function is called for each field in each (changed) table / row that
| is referred to by a business rule.
*-----------------------------------------------------------------------------*/
typedef void (BRE_FIELDQUERY_CB)(
   const char     *table_name,      /* I */
   const void     *table_cookie,    /* I client-defined value */
   const char     *field_name,      /* I */
   const void     *field_cookie,    /* I client-defined value */
   unsigned int   row_number,       /* I */
   int            new_value,        /* I 1 - new, 0 - old */
   BRE_DATUM      *field_value);    /* O */

/*-----------------------------------------------------------------------------
| Callback prototype - Optional
| Purpose:  To communicate diagnostic information to the client.
|           If no 'emit_rule_diagnostic()' function has been registered,
|           messages are are printed to <stderr>.
*-----------------------------------------------------------------------------*/
typedef void (BRE_RULEDIAG_CB)(
   const char        *filename,  /* I */
   const char        *rule_id,   /* I */
   BRE_DIAGNOSTIC failure,       /* I BRE_FAIL-Failure, BRE_WARN-Warning, BRE_FATAL-Fatal */
   char     **message_tokens);   /* I Null terminated list of message tokens */

/*-----------------------------------------------------------------------------
| BRE Library proto-types follow:
*----------------------------------------------------------------------------*/

/* CUSTOMIZATIONS ABOVE */

/*-----------------------------------------------------------------------------
| Purpose:  To tell the business rule engine that the given name corresponds
|           to a 'table'.
*-----------------------------------------------------------------------------*/
   void
bre_register_table(
   const char     *table_name,      /* I */
   const void     *table_cookie);   /* I Optional client-defined value */

/*-----------------------------------------------------------------------------
| Purpose:  To tell the business rule engine that the given field name
|           is associated with the given table name.
*-----------------------------------------------------------------------------*/
   void
bre_register_table_field(
   const char     *table_name,      /* I */
   const char     *field_name,      /* I */
   char     field_type,       /* I (B, I, F, S) */
   unsigned int   field_length,  /* I Meaningful for S only */
   const void     *field_cookie);   /* I Optional client-defined value */

/*-----------------------------------------------------------------------------
| Purpose:  To register a callback procedure with th business rule engine. This
|	    callback defines a new function or lookup table that can be accessed
|	    from the business rule language.
|
|	    The caller must register all functions/lookup tables before
|	    compiling a rule file.
|
| name      - name of the function or lookup table.
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
   const void     *external_cookie, /* I - client-defined value */
   const char     *args_in,         /* I */
   const char     *args_out,        /* I */
   BRE_EXTERNAL_CB *function);      /* I callback function */

/*-----------------------------------------------------------------------------
| Purpose:  To tell the business rule engine to compile the rules found
|           in the given rule_file. Diagnostic messages will be written
|           to the diagnostics_file in the event that errors are encountered.
| Returns:	BRE_SUCCESS - Success.
|		BRE_FAIL - Some rules Failed.
*-----------------------------------------------------------------------------*/
   BRE_DIAGNOSTIC
bre_compile_rules(
   const char  *rule_file,    /* I */
   const char  *diagnostics_file);  /* I */

/*-----------------------------------------------------------------------------
| Following are prototypes for the functions that allow 'registration'
| of the callback functions listed below.
*-----------------------------------------------------------------------------*/

   void
bre_register_tell_field_referenced(
   BRE_REFERENCED_CB *tell_field_reference);

   void
bre_register_query_table_row_count(
   BRE_ROWCOUNT_CB      *query_table_row_count);

   void
bre_register_query_row_change(
   BRE_ROWCHANGED_CB *query_row_change);

   void
bre_register_query_field_data(
   BRE_FIELDQUERY_CB *query_field_data);

   void
bre_register_emit_rule_diagnostic(
   BRE_RULEDIAG_CB      *emit_rule_diagnostic);

/*----------------------------------------------------------------------
 | Purpose:   To register the bre builtin functions.
 |
 | fp         - If FP is non-NULL, then write the proto-type information out
 |              to this file descriptor.
 |
 *----------------------------------------------------------------------*/
   void
bre_register_builtins(
	FILE *fp);

/*-----------------------------------------------------------------------------
| Purpose:  To tell apply the 'currently defined' business rules against
|           the 'current' table data.
| Returns:	BRE_SUCCESS - Success.
|		BRE_WARN - success with some warnings.
|		BRE_FAIL - some rules failed.
|		BRE_FATAL - fatal error in an external function or external lookup.
*-----------------------------------------------------------------------------*/
   BRE_DIAGNOSTIC
bre_evaluate_rules(void);

/*-----------------------------------------------------------------------------
| Purpose:  Free's all data structures, byte-code programs, symbols etc..
|           After calling this function all memory should have have been free'd
|
|           If 'debug_file' is not NULL, debug information and statistics will
|           be written into the file.
|
*-----------------------------------------------------------------------------*/
   void
bre_shutdown(
	const char *debug_file);	/* I - name of a file to contain debug info */

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
	const char *fatal_message);	/* I - error message string */

#endif
