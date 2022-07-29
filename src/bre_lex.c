/*** $Id$ ***/

/*
 */
#ifndef lint
static char rcsid[] = "@(#)$Id$";
#endif

/***********************************************************************
 * BRE LEXICAL ANALYSER
 */

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <string.h>
#include <assert.h>

#include "bre_api.h"
#include "bre_prv.h"

/*----------------------------------------------------------------------
 * All the keywords for the BRE language have been hashed into
 * this table. This allows for fast determination if an identifier
 * is a keyword or not.
 *
 * These parameters were derived from kwhash.c
 * If you add new keywords, run kwhash.c with the
 * new keywords and rebuild this table.
 *
 */
#define KEYWORD_HASH_SIZE	105
#define KEYWORD_HASH_CODE	0x426b

static struct {
	char	*name;
	int	token_type;
	int	value;
} keywords[KEYWORD_HASH_SIZE] = {
/* 0000: */	NULL,		0,		0,
/* 0001: */	NULL,		0,		0,
/* 0002: */	NULL,		0,		0,
/* 0003: */	"min",		MIN,		0,
/* 0004: */	"between",	BETWEEN,	0,
/* 0005: */	"not",		NOT,		0,
/* 0006: */	NULL,		0,		0,
/* 0007: */	NULL,		0,		0,
/* 0008: */	NULL,		0,		0,
/* 0009: */	NULL,		0,		0,
/* 0010: */	NULL,		0,		0,
/* 0011: */	"string",	KW_STRING,	0,
/* 0012: */	"begin",	BEGIN,		0,
/* 0013: */	NULL,		0,		0,
/* 0014: */	NULL,		0,		0,
/* 0015: */	NULL,		0,		0,
/* 0016: */	NULL,		0,		0,
/* 0017: */	"and",		AND,		0,
/* 0018: */	NULL,		0,		0,
/* 0019: */	"float",	KW_FLOAT,	0,
/* 0020: */	"old",		OLD,		0,
/* 0021: */	NULL,		0,		0,
/* 0022: */	NULL,		0,		0,
/* 0023: */	"else",		ELSE,		0,
/* 0024: */	NULL,		0,		0,
/* 0025: */	NULL,		0,		0,
/* 0026: */	"rule",		RULE,		0,
/* 0027: */	"like",		LIKE,		0,
/* 0028: */	NULL,		0,		0,
/* 0029: */	"lookup",	LOOKUP,		0,
/* 0030: */	NULL,		0,		0,
/* 0031: */	"some",		SOME,		0,
/* 0032: */	NULL,		0,		0,
/* 0033: */	NULL,		0,		0,
/* 0034: */	NULL,		0,		0,
/* 0035: */	NULL,		0,		0,
/* 0036: */	"external",	EXTERNAL,	0,
/* 0037: */	NULL,		0,		0,
/* 0038: */	"false",	BOOLEAN,	0,
/* 0039: */	NULL,		0,		0,
/* 0040: */	NULL,		0,		0,
/* 0041: */	"or",		OR,		0,
/* 0042: */	NULL,		0,		0,
/* 0043: */	"sum",		SUM,		0,
/* 0044: */	NULL,		0,		0,
/* 0045: */	NULL,		0,		0,
/* 0046: */	NULL,		0,		0,
/* 0047: */	NULL,		0,		0,
/* 0048: */	"using",	USING,		0,
/* 0049: */	"end",		END,		0,
/* 0050: */	NULL,		0,		0,
/* 0051: */	"constant",	CONSTANT,	0,
/* 0052: */	"group",	GROUP,		0,
/* 0053: */	NULL,		0,		0,
/* 0054: */	"warn",		WARN,		0,
/* 0055: */	NULL,		0,		0,
/* 0056: */	NULL,		0,		0,
/* 0057: */	"then",		THEN,		0,
/* 0058: */	NULL,		0,		0,
/* 0059: */	"all",		ALL,		0,
/* 0060: */	NULL,		0,		0,
/* 0061: */	"count",	COUNT,		0,
/* 0062: */	NULL,		0,		0,
/* 0063: */	NULL,		0,		0,
/* 0064: */	NULL,		0,		0,
/* 0065: */	NULL,		0,		0,
/* 0066: */	NULL,		0,		0,
/* 0067: */	NULL,		0,		0,
/* 0068: */	NULL,		0,		0,
/* 0069: */	NULL,		0,		0,
/* 0070: */	NULL,		0,		0,
/* 0071: */	NULL,		0,		0,
/* 0072: */	NULL,		0,		0,
/* 0073: */	"integer",	KW_INTEGER,	0,
/* 0074: */	"if",		IF,		0,
/* 0075: */	NULL,		0,		0,
/* 0076: */	"null",		KW_NULL,	0,
/* 0077: */	"fail",		FAIL,		0,
/* 0078: */	NULL,		0,		0,
/* 0079: */	NULL,		0,		0,
/* 0080: */	NULL,		0,		0,
/* 0081: */	NULL,		0,		0,
/* 0082: */	NULL,		0,		0,
/* 0083: */	"boolean",	KW_BOOLEAN,	0,
/* 0084: */	"function",	FUNCTION,	0,
/* 0085: */	NULL,		0,		0,
/* 0086: */	NULL,		0,		0,
/* 0087: */	"for",		FOR,		0,
/* 0088: */	"with",		WITH,		0,
/* 0089: */	NULL,		0,		0,
/* 0090: */	"in",		IN,		0,
/* 0091: */	"true",		BOOLEAN,	1,
/* 0092: */	NULL,		0,		0,
/* 0093: */	"rownum",	ROWNUM,		0,
/* 0094: */	NULL,		0,		0,
/* 0095: */	NULL,		0,		0,
/* 0096: */	NULL,		0,		0,
/* 0097: */	"max",		MAX,		0,
/* 0098: */	NULL,		0,		0,
/* 0099: */	NULL,		0,		0,
/* 0100: */	"is",		IS,		0,
/* 0101: */	"where",	WHERE,		0,
/* 0102: */	"compute",	COMPUTE,	0,
/* 0103: */	NULL,		0,		0,
/* 0104: */	NULL,		0,		0,
};

/*----------------------------------------------------------------------
 * Define the global variable 'yylval'. This variable
 * is used by yylex() to return attributes of tokens to the
 * parser.
 */
union yystack yylval;

/*----------------------------------------------------------------------
 * This function is modeled after the one in kwhash.c. If
 * you change the hash function to add/alter the keywords, then
 * replace this function.
 *
 */
static int keyword_hash(char *str)
{
	char *p, c;
	unsigned h, code, shift;

	h = 0;
	code = KEYWORD_HASH_CODE;
	for(p=str; *p; p++) {
		c = (*p) - 'a';

		shift = (code & 7) * 5;

		if( shift > 32 )
			shift = shift - 32;

		h = h | (c << shift);

		code = code>>3;
	}

	return h % KEYWORD_HASH_SIZE;
}

/*----------------------------------------------------------------------
 * Look for 'word' in the keyword table, return token_type and value.
 * (Keywords are always matched with case distinctions)
 *
 * RETURNS:
 *	0 - not found
 *	1 - found.
 */
static int keyword_lookup(char *word, int *token_type, int *value)
{
	int hv;

	hv = keyword_hash(word);

	if( keywords[hv].name && !strcmp(word, keywords[hv].name) ) {
		*token_type = keywords[hv].token_type;
		*value	    = keywords[hv].value;
		return 1;
	} else
		return 0;
}

/* ----------------------------------------------------------------------
 * Get a character from stdin or character buffer
 */
static int getit(void)
{
	BRE()->cpos += 1;
	BRE()->lpos += 1;
	if( BRE()->input_str )
		return *(BRE()->input_str)++;
	else
		return getc(BRE()->fp);
}

/* ----------------------------------------------------------------------
 * Unget a character and move lineptr back by one.
 */
static void ungetit(int c)
{
	if( BRE()->input_str ) {
		(BRE()->input_str)--;
	} else {
		ungetc(c, BRE()->fp);
	}
	BRE()->cpos -= 1;
	BRE()->lpos -= 1;
}

/*----------------------------------------------------------------------
 * action[] array:
 * --------------
 *	- This array contains a byte for each ASCII character.
 *	- This byte is divided into two parts:
 *		T = TOKEN TYPE
 *		C = Charater class.
 *
 *	- The mask value 'ACTION_MASK' is used to extract the
 *	  different component from this byte.
 *
 *	+-------+-------+------- + -----+-------+-------+-------+-------+
 *	|	|	|	 |	|	|	|	|	|
 *	|   C1	|   C2	|   C3	 |   T	|   T	|   T	|   T	|   T	|
 *	|	|	|	 |	|	|	|	|	|
 *	+-------+-------+------- + -----+-------+-------+-------+-------+
 *
 *	TTTTT =	These bits define an integer enumeration. Which can be
 *		one of: {ER, WS, ST, SF, NM, ID, EF, t1, t2, t3, t4}
 *
 *	CCC =	These 3 bits are used to classify each ascii character.
 *		Each bit has the following meaning:
 *		C1 = 1 if the character is valid inside a number/float token.
 *		C2 = 1 if the character is valid inside a string.
 *		C3 = 1 if the character is valid in an identifier.
 *
 * THE TOKENIZER ALGORITHM:
 * ------------------------
 *	1. Read a character 'c' from the input.
 *	2. Index the 'action[c]' array using the character 'c'.
 *	3. Apply the ACTION_MASK and switch() on the action type.
 *	4. A large switch() statement handles each token type.
 *
 * FIGURING OUT WHAT'S IN THE action[] ARRAY
 * -----------------------------------------
 *	To understand how the action[] array is initialized, look
 *	at the entry for the letter 'A':
 *		ID|SI
 *
 *	'ID' is from the enumeration and 'SI' is a #define for
 *	the bits {C1, C2, C3}
 *
 *	ID = this means that when 'A' is the first character of
 *	     a token, then it is going to be an identifier.
 *
 *	SI = this means that the letter 'A' can ligitimately appear
 *	     inside of a string token or an identifier token.
 */

/*
 * first character of each token
 */
enum {
	ER,	/* error */
	WS,	/* white space */
	ST,	/* string */
	SF,	/* itself */
	NM,	/* number */
	ID,	/* identifier */
	EF,	/* EOF */
	t1,	/* < <= <> */
	t2,	/* > >= */
	t3,	/* : := */
	t4	/* -- - */
};

#define ACTION_MASK	0x1f

/* classes */
#define XX  (00)	/* no class */
#define	II  (001<<5)	/* valid identifier character (after the first char) */
#define SS  (002<<5)	/* valid character inside of a string */
#define NN  (004<<5)	/* valid character inside of a numeric quantity */

/* combonations of the above */
#define SI  (SS|II)	/* valid string and identifier */
#define Si  (SS|II|NN)	/* valid string, identifier and number */
#define SN  (SS|NN)	/* valid string and number */

static unsigned char action[] = {
   /* EOF */
   EF|XX,
/*  nul    soh    stx    etx    eot    enq    ack    bel    */
   ER|XX, ER|XX, ER|XX, ER|XX, ER|XX, ER|XX, ER|XX, ER|XX,

/*  bs     ht     nl     vt     np     cr     so     si     */
   ER|XX, WS|SS, WS|XX, WS|XX, WS|XX,  WS|XX, ER|XX, ER|XX,

/*  dle    dc1    dc2    dc3    dc4    nak    syn    etb    */
   ER|XX, ER|XX, ER|XX, ER|XX, ER|XX, ER|XX, ER|XX, ER|XX,

/*  can    em     sub    esc    fs     gs     rs     us     */
   ER|XX, ER|XX, ER|XX, ER|XX, ER|XX, ER|XX, ER|XX, ER|XX,

/*  sp      !      "      #      $      %      &      '     */
   WS|SS, ER|SS, ST|XX, ER|SS, ER|SS, SF|SS, ER|SS, ER|SS,

/*   (      )      *      +      ,      -      .      /     */
   SF|SS, SF|SS, SF|SS, SF|SS, SF|SS, t4|SI, SF|SN, SF|SS,

/*   0      1      2      3      4      5      6      7     */
   NM|Si, NM|Si, NM|Si, NM|Si, NM|Si, NM|Si, NM|Si, NM|Si,

/*   8      9      :      ;      <      =      >      ?     */
   NM|Si, NM|Si, t3|SS, SF|SS, t1|SS, SF|SS, t2|SS, ER|SS,

/*   @      A      B      C      D      E      F      G     */
   ER|SS, ID|SI, ID|SI, ID|SI, ID|SI, ID|SI, ID|SI, ID|SI,

/*   H      I      J      K      L      M      N      O     */
   ID|SI, ID|SI, ID|SI, ID|SI, ID|SI, ID|SI, ID|SI, ID|SI,

/*   P      Q      R      S      T      U      V      W     */
   ID|SI, ID|SI, ID|SI, ID|SI, ID|SI, ID|SI, ID|SI, ID|SI,

/*   X      Y      Z      [      \      ]      ^      _     */
   ID|SI, ID|SI, ID|SI, SF|SS, ER|SS, SF|SS, ER|SS, ER|SI,

/*   `      a      b      c      d      e      f      g     */
   ER|SS, ID|SI, ID|SI, ID|SI, ID|SI, ID|SI, ID|SI, ID|SI,

/*   h      i      j      k      l      m      n      o     */
   ID|SI, ID|SI, ID|SI, ID|SI, ID|SI, ID|SI, ID|SI, ID|SI,

/*   p      q      r      s      t      u      v      w     */
   ID|SI, ID|SI, ID|SI, ID|SI, ID|SI, ID|SI, ID|SI, ID|SI,

/*   x      y      z      {      |      }      ~     del    */
   ID|SI, ID|SI, ID|SI, SF|SS, ER|SS, SF|SS, ER|SS, ER|XX
};

/* ----------------------------------------------------------------------
 * yylex:
 *	Return a token to the parser.
 *	Tokens set eebrglval as follows:
 *
 *	Token Type		component used
 *	--------------		--------------
 *	INTEGER			yylval.l (long)
 *	FLOAT			yylval.d (double)
 *	BOOLEAN			yylval.l (long)
 *	STRING			yylval.str (char *)
 *	IDENTIFIER		yylval.str (char *)
 *
 *	All Other tokens have no attributes, and do not touch frmlval.
 */
int yylex(void)
{
	char num1[30];
	char buf[MAX_TOKEN_SIZE], *p, *r;
	int c, n, ttype, found;

   for(;;) {
	switch( (action+1)[ c = getit() ] & ACTION_MASK ) {
	case ER:	/* error state */
		if( c == '\0' && BRE()->input_str ) {
			return 0;
		} else {
			buf[0] = c;
			buf[1] = '\0';
			sprintf(num1, "%03o", c);
			Bre_Parse_Error(BREMSG_LEX_INVCHR, num1);
		}

	case WS:	/* white space */
		if( c == '\n' ) {
			BRE()->lineno++;
			BRE()->lpos = 1;
		}
		break;

	case ST:	/* string */
		p = buf;
		while( (action+1)[c=getit()]&SS ) {
			if( p - buf > sizeof(buf) )
				Bre_Parse_Error(BREMSG_LEX_STRBIG);
			*p++ = c;
		}
		*p = '\0';
		if( c == EOF ) Bre_Parse_Error(BREMSG_LEX_EOFSTR);
		if( c != '"' ) Bre_Parse_Error(BREMSG_LEX_INVSTR);
		
		yylval.str = BRE_STRDUP(buf);
		return STRING;

	case SF:	/* itself */
		return c;

	case NM:	/* numeric value */
		p = buf;
		do {
			if( p - buf > sizeof(buf) )
				Bre_Parse_Error(BREMSG_LEX_BIGNUM);
			*p++ = c;
			c = getit();
		} while( (action+1)[c]&NN );
		*p = '\0';
		ungetit(c);

		if( p = strchr(buf, '.') ) {
			r = strrchr(buf, '.');
			if( p != r )
				Bre_Parse_Error(BREMSG_LEX_INVNUM, buf);
			if( strlen(buf) == 1 )
				Bre_Parse_Error(BREMSG_LEX_INVNUM, buf);
			yylval.d = atof(buf);
			return FLOAT;
		} else {
			yylval.l = strtol(buf, NULL, 10);
			return INTEGER;
		}

	case ID:	/* identifier */
		p = buf;
		do {
			if( p - buf < sizeof(buf) )
				*p++ = c;
			c=getit();
		} while( (action+1)[c]&II );
		ungetit(c);
		*p = '\0';

		/*
		 * Check for keyword
		 */
		found = keyword_lookup(buf, &ttype, &n);
		if( found ) {
			yylval.l = n;
			return ttype;
		}

		yylval.str = BRE_STRDUP(buf);
		return IDENTIFIER;

	case EF:	/* End of File */
		return 0;

	case t1:	/* < <= <> */
		c=getit();
		if( c == '=' ) return LE;
		if( c == '>' ) return NE;
		ungetit(c);
		return '<';

	case t2:	/* > >=  */
		c = getit();
		if( c == '=' )
			return GE;
		ungetit(c);
		return '>';

	case t3:	/* : :=  */
		c = getit();
		if( c == '=' )
			return ASSIGN_OP;
		ungetit(c);
		return ':';

	case t4:	/* -- - */
		c = getit();
		if( c != '-' ) {
			ungetit(c);
			return '-';
		} else {
			while( (c = getit()) != '\n' && c != EOF)
				;
			BRE()->lineno += 1;
		}
		if( c == EOF )
			Bre_Parse_Error(BREMSG_LEX_EOFCOM);
		break;

	default:
		BRE_ASSERT(0);
	}
   }
}

