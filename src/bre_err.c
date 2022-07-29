/*** $Id$ ***/

/*
 */
#ifndef lint
static char rcsid[] = "@(#)$Id$";
#endif

/***********************************************************************
 * BRE ERROR HANDLING ROUTINES
 */
/* pintfdef .h */

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <setjmp.h>
#include <stdarg.h>

#include "bre_api.h"
#include "bre_prv.h"

#define ERROR_THRESHOLD	10

/*
 * Error context.
 */
static struct {
	int	err_cnt;
} EC;

/*
 * NOTE: This is gross. BRE should not need to
 * be aware of how the msg routines encode the
 * tokens. (ie. I shouldn't be aware that '!' is used)
 * Maybe a routine like this should be provided in
 * msg_get.c
 *
 * (I need to know how many tokens are being passed to my
 * varargs error function, and the only way to know this is
 * by counting the number of tokens used in the error message
 * string)
 *
 */
static int count_tokens(char *str)
{
	char *p;
	int ntoks;

	ntoks = 0;
	for(p=str; *p; p++) {
		if( *p == '!' && isdigit(*(p+1)) )
			ntoks += 1;
	}
	return ntoks;
}

   void
bre_clear_errors(void)
{
	EC.err_cnt = 0;
}

   int
bre_error_count(void)
{
	return EC.err_cnt;
}

/*----------------------------------------------------------------------
 *
 * Print an error message.
 * This causes the parse to be aborted immediately.
 *
 */
   void
Bre_Parse_Error(
	MSG_SET_060 msgcode,
	...)
{
	va_list ap;
	char msg_str[ 2000 ];
	char *tokens[ 50 ];
	int i, ntoks;

	msg_read(BRE_MSGSET, msgcode, msg_str, sizeof(msg_str));

	ntoks = count_tokens(msg_str);

	va_start(ap, msgcode);
	for(i=0; i<ntoks; i++) {
		tokens[i] = va_arg(ap, char*);
	}
	tokens[i] = NULL;
	va_end(ap);

	msg_expand(msg_str, sizeof(msg_str), tokens);

	if( BRE()->input_str )
		fprintf(BRE()->fp_diag, "%s\n", msg_str);
	else
		fprintf(BRE()->fp_diag, "%s: line: %d, column %d %s\n",
				BRE()->filenames[ BRE()->file_count-1 ],
				BRE()->lineno, BRE()->lpos, msg_str);

	/* return to the caller -- immediately */
	longjmp(BRE()->jmpbuf, 1);
}

/*----------------------------------------------------------------------
 * Print an parse warning message.
 * Warning message still prevents success, but
 * a warning does not immediately halt
 *
 */
   void
Bre_Parse_Warn(
	MSG_SET_060 msgcode,
	...)
{
	va_list ap;
	char msg_str[ 2000 ];
	char *tokens[ 50 ];
	int i, ntoks;

	msg_read(BRE_MSGSET, msgcode, msg_str, sizeof(msg_str));

	ntoks = count_tokens(msg_str);

	va_start(ap, msgcode);
	for(i=0; i<ntoks; i++) {
		tokens[i] = va_arg(ap, char*);
	}
	tokens[i] = NULL;
	va_end(ap);

	msg_expand(msg_str, sizeof(msg_str), tokens);

	if( BRE()->input_str )
		fprintf(BRE()->fp_diag, "%s\n", msg_str);
	else
		fprintf(BRE()->fp_diag, "%s: line: %d, column %d %s\n",
				BRE()->filenames[ BRE()->file_count-1 ],
				BRE()->lineno,
				BRE()->lpos,
				msg_str);

	EC.err_cnt += 1;

	if( EC.err_cnt > ERROR_THRESHOLD )
		longjmp(BRE()->jmpbuf, 1);
}

/*----------------------------------------------------------------------
 * Print a message.
 * Use this function to warn the user. (This function is currently not
 * being called)
 *
 */
   void
Bre_Parse_Message(
	MSG_SET_060 msgcode,
	...)
{
	va_list ap;
	char msg_str[ 2000 ];
	char *tokens[ 50 ];
	int i, ntoks;

	msg_read(BRE_MSGSET, msgcode, msg_str, sizeof(msg_str));

	ntoks = count_tokens(msg_str);

	va_start(ap, msgcode);
	for(i=0; i<ntoks; i++) {
		tokens[i] = va_arg(ap, char*);
	}
	tokens[i] = NULL;
	va_end(ap);

	if( BRE()->input_str )
		fprintf(BRE()->fp_diag, "%s\n", msg_str);
	else
		fprintf(BRE()->fp_diag, "%s: line: %d, column %d %s\n",
				BRE()->filenames[ BRE()->file_count-1 ],
				BRE()->lineno, BRE()->lpos, msg_str);
}

/*----------------------------------------------------------------------
 * Print an error message.
 * This error function is for internal, or serious errors
 * not related to parsing. It will immediately exit() the
 * current program.
 *
 * This function is for low-level errors like out of memory
 * conditions.
 *
 */
   void
Bre_Error(
	MSG_SET_060 msgcode,
	...)
{
	va_list ap;
	char msg_str[ 2000 ];
	char *tokens[ 50 ];
	int i, ntoks;

	msg_read(BRE_MSGSET, msgcode, msg_str, sizeof(msg_str));

	ntoks = count_tokens(msg_str);

	va_start(ap, msgcode);
	for(i=0; i<ntoks; i++) {
		tokens[i] = va_arg(ap, char*);
	}
	tokens[i] = NULL;
	va_end(ap);

	msg_expand(msg_str, sizeof(msg_str), tokens);

	if( BRE()->fp_diag )
		fprintf(BRE()->fp_diag, "%s\n", msg_str);
	else
		fprintf(stderr, "%s\n", msg_str);

	/*
	 * Die...
	 */
	exit(1);
}

