
# line 2 "bre_yac.y"
/*
 */
#ifndef lint
static char rcsid[] = "@(#)$Id$";
#endif

/***********************************************************************
 * BRE GRAMMAR
 */
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <string.h>
#include <assert.h>

#include "bre_api.h"
#include "bre_prv.h"

#define YYSTYPE		union yystack

#ifdef __cplusplus
#  include <stdio.h>
#  include <yacc.h>
#endif	/* __cplusplus */ 
# define OR 257
# define AND 258
# define LE 259
# define GE 260
# define NE 261
# define IN 262
# define BETWEEN 263
# define ASSIGN_OP 264
# define NOT 265
# define OLD 266
# define LIKE 267
# define EXTERNAL 268
# define CONSTANT 269
# define FUNCTION 270
# define LOOKUP 271
# define RULE 272
# define GROUP 273
# define IS 274
# define USING 275
# define FAIL 276
# define WARN 277
# define WITH 278
# define COMPUTE 279
# define WHERE 280
# define FOR 281
# define ALL 282
# define SOME 283
# define IF 284
# define THEN 285
# define ELSE 286
# define BEGIN 287
# define END 288
# define COUNT 289
# define MAX 290
# define MIN 291
# define SUM 292
# define ROWNUM 293
# define KW_INTEGER 294
# define KW_FLOAT 295
# define KW_BOOLEAN 296
# define KW_STRING 297
# define INTEGER 298
# define FLOAT 299
# define STRING 300
# define BOOLEAN 301
# define KW_NULL 302
# define IDENTIFIER 303
#define yyclearin yychar = -1
#define yyerrok yyerrflag = 0
extern int yychar;
#ifndef YYMAXDEPTH
#define YYMAXDEPTH 150
#endif

/* __YYSCLASS defines the scoping/storage class for global objects
 * that are NOT renamed by the -p option.  By default these names
 * are going to be 'static' so that multi-definition errors
 * will not occur with multiple parsers.
 * If you want (unsupported) access to internal names you need
 * to define this to be null so it implies 'extern' scope.
 * This should not be used in conjunction with -p.
 */
#ifndef __YYSCLASS
# define __YYSCLASS static
#endif
YYSTYPE yylval;
__YYSCLASS YYSTYPE yyval;
typedef int yytabelem;
# define YYERRCODE 256

# line 344 "bre_yac.y"


/* yyerror -
 *	Pass control to the "real" edits-engine error function.
 */
static void yyerror(char *s)
{
	Bre_Parse_Error(BREMSG_YACC_SYNTAX, s);
}
__YYSCLASS yytabelem yyexca[] ={
-1, 1,
	0, -1,
	-2, 0,
-1, 90,
	265, 0,
	-2, 76,
-1, 155,
	44, 21,
	58, 21,
	-2, 109,
	};
# define YYNPROD 125
# define YYLAST 830
__YYSCLASS yytabelem yyact[]={

    39,   199,   142,    82,    83,    37,   213,   215,   216,   214,
    52,    23,    59,   211,   156,   217,    22,   143,   100,    61,
    30,    29,    18,    17,    59,    16,   236,   136,   209,    39,
    53,    56,    55,    54,    37,   206,   205,    28,    64,    13,
    10,    11,    81,    14,    12,    47,    60,    79,    77,    57,
    78,    24,    80,    19,    20,   116,   117,   180,    39,   164,
   118,   163,    98,    37,   133,    73,    71,    75,    97,    50,
   141,   165,   232,   108,    88,    51,   151,   149,   230,   229,
   228,   227,   226,    52,    95,   204,   150,    39,    97,   108,
   171,   174,    37,    63,    99,   108,   180,   220,   219,    81,
   190,    81,    84,   181,    79,    77,    79,    78,   237,    80,
   238,    80,    52,    96,   104,   182,   175,   108,   107,   176,
    85,   108,   102,   101,    26,   188,   208,     3,    27,    25,
    15,   207,    81,    96,   152,   111,   109,    79,    77,   170,
    78,    52,    80,    81,   158,   106,     9,    58,    79,    77,
    31,    78,     8,    80,   231,    73,    71,    75,   148,    62,
     7,     6,     5,     4,     2,     1,    73,    71,    75,    21,
    52,   197,   198,   201,   202,   200,   235,   183,    41,   166,
    87,   203,   139,    81,   110,   131,   132,   242,    79,    77,
   100,    78,   223,    80,    81,    38,   103,    93,   159,    79,
    77,   144,    78,   167,    80,     0,    73,    71,    75,   166,
   166,   169,     0,     0,     0,   172,     0,    73,    71,    75,
     0,     0,     0,     0,     0,    36,    48,   154,   173,     0,
     0,     0,   154,   167,   167,   186,   187,     0,     0,     0,
     0,    33,   191,     0,    34,     0,     0,    35,     0,    53,
    56,    55,    54,    49,    36,    48,   135,   196,    42,    43,
    44,    45,    46,    40,    74,    76,    72,    67,    68,     0,
    33,     0,    70,    34,   185,   184,    35,     0,    53,    56,
    55,    54,    49,    36,    48,   224,    94,    42,    43,    44,
    45,    46,    40,   234,     0,     0,   225,     0,     0,    33,
     0,   154,    34,     0,     0,    35,     0,    53,    56,    55,
    54,    49,    36,    48,     0,     0,    42,    43,    44,    45,
    46,    40,     0,     0,     0,   218,     0,     0,    33,     0,
     0,    34,     0,     0,    35,     0,    53,    56,    55,    54,
    49,     0,     0,     0,     0,    42,    43,    44,    45,    46,
   155,     0,    65,    66,    74,    76,    72,    67,    68,   153,
    69,     0,    70,    65,    66,    74,    76,    72,    67,    68,
     0,    69,     0,    70,     0,     0,     0,     0,     0,     0,
     0,   189,     0,     0,    32,     0,     0,     0,     0,     0,
     0,   134,     0,     0,    86,    89,    90,    91,     0,    92,
     0,     0,     0,    65,    66,    74,    76,    72,    67,    68,
     0,    69,   105,    70,    65,    66,    74,    76,    72,    67,
    68,     0,    69,     0,    70,   112,   113,   114,   115,     0,
   119,   120,   121,   122,   123,   124,   125,   126,   127,   128,
   129,   130,    81,     0,     0,     0,     0,    79,    77,     0,
    78,     0,    80,     0,   138,     0,   140,     0,   145,     0,
     0,   146,   147,     0,     0,    73,    71,    75,     0,   157,
     0,     0,     0,     0,     0,     0,   160,   161,   162,    81,
     0,     0,     0,   178,    79,    77,     0,    78,     0,    80,
     0,     0,     0,     0,   168,     0,    89,     0,   212,     0,
     0,     0,    73,    71,    75,     0,     0,     0,     0,   179,
     0,     0,     0,     0,    81,     0,     0,     0,   177,    79,
    77,     0,    78,     0,    80,     0,     0,     0,     0,     0,
     0,   192,   193,     0,   194,     0,   195,    73,    71,    75,
    81,     0,     0,     0,   137,    79,    77,     0,    78,   210,
    80,    81,     0,     0,     0,     0,    79,    77,     0,    78,
     0,    80,     0,    73,    71,    75,     0,   221,   222,     0,
     0,     0,     0,     0,    73,    71,    75,    81,     0,   233,
   233,     0,    79,    77,     0,    78,     0,    80,     0,     0,
     0,     0,     0,     0,     0,     0,   239,   240,   241,    81,
    73,    71,    75,     0,    79,    77,     0,    78,     0,    80,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,    73,    71,    75,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,    65,    66,    74,    76,    72,    67,    68,     0,
    69,     0,    70,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,    65,
    66,    74,    76,    72,    67,    68,     0,    69,     0,    70,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,    65,    66,    74,    76,    72,    67,
    68,     0,    69,     0,    70,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
    65,    66,    74,    76,    72,    67,    68,     0,    69,     0,
    70,    65,    66,    74,    76,    72,    67,    68,     0,    69,
     0,    70,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,    66,    74,
    76,    72,    67,    68,     0,    69,     0,    70,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,    74,    76,    72,    67,    68,     0,    69,     0,    70 };
__YYSCLASS yytabelem yypact[]={

  -229, -3000,  -229, -3000, -3000, -3000, -3000, -3000, -3000, -3000,
  -278,  -280,  -281,  -217,  -287, -3000,  -223,    84,  -238,  -282,
  -283,  -238, -3000, -3000,    18,  -225,  -291,  -228,  -284,    84,
     2,  -236,   514,  -279,    18,    18,    18,    18, -3000,    18,
    22, -3000, -3000, -3000, -3000, -3000, -3000, -3000,  -285,    83,
 -3000,    82,   -11, -3000, -3000, -3000, -3000, -3000,    77, -3000,
 -3000, -3000, -3000,  -291, -3000,    18,    18,    18,    18,  -207,
    18,    18,    18,    18,    18,    18,    18,    18,    18,    18,
    18,    18,  -291,  -291,  -198,    73,   106,   -32, -3000,   514,
     5, -3000,   503, -3000,    18, -3000,   -40,  -286,    18, -3000,
    42,    18,    18,    33, -3000,   514,    47, -3000,  -289,    18,
    51,    47,   540,   562,    62,    62,    18,    18,    18,    62,
    62,    62,    62,    62,    62,    62,    64,    64, -3000, -3000,
 -3000,  -201,  -203,  -113,    18, -3000,    18, -3000,   514,    46,
   157, -3000, -3000,     0,    75,   514,   477,   442, -3000,    18,
    37, -3000, -3000,   514,    45,    22, -3000,   514,    69,    -2,
    62,    62,    62,  -113,  -113, -3000, -3000, -3000,    95, -3000,
    54,   -40,    18, -3000,    18, -3000,    18, -3000, -3000,   514,
    47,  -122,   -38, -3000,  -242,  -243, -3000, -3000,  -251,    18,
  -290, -3000,   514,   405,   157,   514, -3000, -3000, -3000,  -288,
 -3000, -3000, -3000, -3000,  -291,    40,    39,    18,    18,  -259,
   514,     0, -3000,   -43,   -44,   -45,   -46,   -47,    29,    18,
    18,   514,   514,  -254,    68, -3000, -3000, -3000, -3000, -3000,
 -3000, -3000,    66,   514,    66, -3000,    18,    18,    18,   514,
   146,   514, -3000 };
__YYSCLASS yytabelem yypgo[]={

     0,   201,   197,   196,    69,    45,    84,   195,   192,    71,
   359,   182,    70,    76,   180,    74,   178,    72,   177,   102,
   176,    75,   171,   120,   129,   128,   169,   165,   164,   127,
   163,   162,   161,   160,   152,   146,   145,    86,   136,   135,
   134,   131,   126,   125 };
__YYSCLASS yytabelem yyr1[]={

     0,    27,    28,    28,    29,    29,    29,    29,    29,    29,
    33,    34,    30,    36,    31,    38,    32,    39,    35,    24,
    24,    23,    23,    26,    26,    25,    25,    18,    18,    18,
    17,    17,    37,    37,    13,    13,    13,    40,    22,    22,
    22,    22,    22,    22,    22,    22,    22,    22,    41,    10,
    42,    10,    43,    10,    10,    10,    10,    10,    10,    10,
    10,    10,    10,    10,    10,    10,    10,    10,    10,    10,
    10,    10,    10,    10,    10,    10,    10,    10,    10,     9,
     9,    19,    20,    20,     8,     8,    16,    21,    21,    21,
    21,     7,     7,     7,     7,     7,     7,     7,     7,     7,
     7,     7,     7,     7,     5,     5,     5,     5,     6,     6,
    11,    11,    12,    12,     4,     4,     3,     3,     2,     1,
     1,    14,    14,    15,    15 };
__YYSCLASS yytabelem yyr2[]={

     0,     2,     2,     4,     2,     2,     2,     2,     2,     2,
     9,    21,     9,     1,    13,     1,    13,     1,    15,     7,
     1,     3,     7,     3,     3,     5,     1,     9,     9,     1,
     3,     7,     3,     7,     3,     3,     1,     7,     3,     7,
     3,     7,     3,     7,     3,     7,     3,     7,     1,    15,
     1,    15,     1,    17,    13,     9,     7,     7,     7,     7,
     7,     9,     9,     7,     9,     7,     7,     7,     7,     7,
     7,     7,     7,     7,     7,     7,     5,     5,     3,     3,
     3,     3,     5,     1,     3,     9,     9,     3,     3,     3,
     3,     7,     5,     7,     3,     3,     3,     3,     3,     3,
     3,     5,     9,     3,     5,     9,     9,    15,    11,     1,
     3,     7,     3,     3,     7,     5,     3,     7,     7,     3,
     7,     3,     7,     3,     1 };
__YYSCLASS yytabelem yychk[]={

 -3000,   -27,   -28,   -29,   -30,   -31,   -32,   -33,   -34,   -35,
   269,   270,   273,   268,   272,   -29,   303,   303,   303,   270,
   271,   -26,   303,   298,   274,   -24,    40,   -25,   275,   303,
   303,   -25,   -10,   281,   284,   287,   265,    45,    -7,    40,
   303,   -16,   298,   299,   300,   301,   302,    -5,   266,   293,
    -4,   -21,   123,   289,   292,   291,   290,   274,   -23,   303,
   274,   303,   -24,    91,   274,   257,   258,   262,   263,   265,
   267,    61,   261,    60,   259,    62,   260,    43,    45,    42,
    47,    37,   282,   283,   -19,   -23,   -10,   -14,   -15,   -10,
   -10,   -10,   -10,    -2,   264,    -6,    91,    46,    40,    -5,
   303,    40,    40,    -3,   125,   -10,   -36,    41,    44,   -38,
   -23,   -39,   -10,   -10,   -10,   -10,   262,   263,   267,   -10,
   -10,   -10,   -10,   -10,   -10,   -10,   -10,   -10,   -10,   -10,
   -10,   -19,   -19,   262,   285,   288,    59,    41,   -10,   -11,
   -10,   -12,    42,   303,    -1,   -10,   -10,   -10,   125,    44,
   -37,   -13,   -40,   -10,   -23,   303,   303,   -10,    93,   -37,
   -10,   -10,   -10,   262,   262,    -9,    -5,    -4,   -10,   -15,
    93,    44,    58,    -6,    91,    41,    44,    41,    41,   -10,
    59,    58,    46,   -18,   277,   276,    -9,    -9,   -43,   286,
    46,   -12,   -10,   -10,   -10,   -10,   -13,   -22,   294,   123,
   297,   295,   296,   303,   123,   278,   278,   -41,   -42,   279,
   -10,   303,    93,   294,   297,   295,   296,   303,   -23,    58,
    58,   -10,   -10,    -8,   -21,    -6,   125,   125,   125,   125,
   125,   125,   -17,   -10,   -17,   -20,   280,    40,    44,   -10,
   -10,   -10,    41 };
__YYSCLASS yytabelem yydef[]={

     0,    -2,     1,     2,     4,     5,     6,     7,     8,     9,
     0,     0,     0,     0,     0,     3,     0,    20,    26,     0,
     0,    26,    23,    24,     0,     0,     0,     0,     0,    20,
     0,     0,    12,     0,     0,   124,     0,     0,    78,     0,
   109,    94,    95,    96,    97,    98,    99,   100,     0,     0,
   103,     0,     0,    87,    88,    89,    90,    13,     0,    21,
    15,    25,    10,     0,    17,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,    81,     0,     0,   121,   123,
    -2,    77,     0,    92,     0,   104,     0,     0,     0,   101,
   109,     0,     0,     0,   115,   116,    36,    19,     0,     0,
     0,    36,    57,    58,    59,    60,     0,     0,     0,    63,
    65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
    75,     0,     0,     0,     0,    56,   124,    91,    93,     0,
   112,   110,   113,   109,     0,   119,     0,     0,   114,     0,
    14,    32,    34,    35,     0,    -2,    22,    16,     0,    29,
    61,    62,    64,     0,     0,    52,    79,    80,    55,   122,
   105,     0,     0,   106,     0,   118,     0,   102,    86,   117,
    36,     0,     0,    18,     0,     0,    48,    50,     0,     0,
     0,   111,   112,     0,     0,   120,    33,    37,    38,     0,
    40,    42,    44,    46,     0,     0,     0,     0,     0,     0,
    54,   109,   108,     0,     0,     0,     0,     0,     0,     0,
     0,    49,    51,    83,    84,   107,    39,    41,    43,    45,
    47,    11,    27,    30,    28,    53,     0,     0,     0,    82,
     0,    31,    85 };
typedef struct { char *t_name; int t_val; } yytoktype;
#ifndef YYDEBUG
#	define YYDEBUG	0	/* don't allow debugging */
#endif

#if YYDEBUG

__YYSCLASS yytoktype yytoks[] =
{
	"OR",	257,
	"AND",	258,
	"LE",	259,
	"GE",	260,
	"NE",	261,
	"IN",	262,
	"BETWEEN",	263,
	"ASSIGN_OP",	264,
	"NOT",	265,
	"OLD",	266,
	"LIKE",	267,
	"EXTERNAL",	268,
	"CONSTANT",	269,
	"FUNCTION",	270,
	"LOOKUP",	271,
	"RULE",	272,
	"GROUP",	273,
	"IS",	274,
	"USING",	275,
	"FAIL",	276,
	"WARN",	277,
	"WITH",	278,
	"COMPUTE",	279,
	"WHERE",	280,
	"FOR",	281,
	"ALL",	282,
	"SOME",	283,
	"IF",	284,
	"THEN",	285,
	"ELSE",	286,
	"BEGIN",	287,
	"END",	288,
	"COUNT",	289,
	"MAX",	290,
	"MIN",	291,
	"SUM",	292,
	"ROWNUM",	293,
	"KW_INTEGER",	294,
	"KW_FLOAT",	295,
	"KW_BOOLEAN",	296,
	"KW_STRING",	297,
	"INTEGER",	298,
	"FLOAT",	299,
	"STRING",	300,
	"BOOLEAN",	301,
	"KW_NULL",	302,
	"IDENTIFIER",	303,
	"=",	61,
	"<",	60,
	">",	62,
	"+",	43,
	"-",	45,
	"*",	42,
	"/",	47,
	"%",	37,
	".",	46,
	"-unknown-",	-1	/* ends search */
};

__YYSCLASS char * yyreds[] =
{
	"-no such reduction-",
	"program : decl_list",
	"decl_list : declaration",
	"decl_list : decl_list declaration",
	"declaration : constant_decl",
	"declaration : function_decl",
	"declaration : group_decl",
	"declaration : external_func",
	"declaration : external_lookup",
	"declaration : rule_decl",
	"external_func : EXTERNAL FUNCTION IDENTIFIER parameters",
	"external_lookup : EXTERNAL LOOKUP IDENTIFIER '[' id_list ']' '.' '{' id_list '}'",
	"constant_decl : CONSTANT IDENTIFIER IS expr",
	"function_decl : FUNCTION IDENTIFIER parameters IS",
	"function_decl : FUNCTION IDENTIFIER parameters IS stmt_list",
	"group_decl : GROUP IDENTIFIER using_clause IS",
	"group_decl : GROUP IDENTIFIER using_clause IS expr",
	"rule_decl : RULE rule_id using_clause IS",
	"rule_decl : RULE rule_id using_clause IS stmt_list error_clause",
	"parameters : '(' id_list ')'",
	"parameters : /* empty */",
	"id_list : IDENTIFIER",
	"id_list : id_list ',' IDENTIFIER",
	"rule_id : IDENTIFIER",
	"rule_id : INTEGER",
	"using_clause : USING IDENTIFIER",
	"using_clause : /* empty */",
	"error_clause : WARN WITH ':' error_exprs",
	"error_clause : FAIL WITH ':' error_exprs",
	"error_clause : /* empty */",
	"error_exprs : expr",
	"error_exprs : error_exprs ',' expr",
	"stmt_list : statement",
	"stmt_list : stmt_list ';' statement",
	"statement : var_decl",
	"statement : expr",
	"statement : /* empty */",
	"var_decl : id_list ':' type_decl",
	"type_decl : KW_INTEGER",
	"type_decl : '{' KW_INTEGER '}'",
	"type_decl : KW_STRING",
	"type_decl : '{' KW_STRING '}'",
	"type_decl : KW_FLOAT",
	"type_decl : '{' KW_FLOAT '}'",
	"type_decl : KW_BOOLEAN",
	"type_decl : '{' KW_BOOLEAN '}'",
	"type_decl : IDENTIFIER",
	"type_decl : '{' IDENTIFIER '}'",
	"expr : FOR ALL for_idlist IN for_expr",
	"expr : FOR ALL for_idlist IN for_expr expr",
	"expr : FOR SOME for_idlist IN for_expr",
	"expr : FOR SOME for_idlist IN for_expr expr",
	"expr : FOR for_idlist IN for_expr",
	"expr : FOR for_idlist IN for_expr COMPUTE group_expr where_clause",
	"expr : IF expr THEN expr ELSE expr",
	"expr : IF expr THEN expr",
	"expr : BEGIN block END",
	"expr : expr OR expr",
	"expr : expr AND expr",
	"expr : expr IN expr",
	"expr : expr BETWEEN expr",
	"expr : expr NOT IN expr",
	"expr : expr NOT BETWEEN expr",
	"expr : expr LIKE expr",
	"expr : expr NOT LIKE expr",
	"expr : expr '=' expr",
	"expr : expr NE expr",
	"expr : expr '<' expr",
	"expr : expr LE expr",
	"expr : expr '>' expr",
	"expr : expr GE expr",
	"expr : expr '+' expr",
	"expr : expr '-' expr",
	"expr : expr '*' expr",
	"expr : expr '/' expr",
	"expr : expr '%' expr",
	"expr : NOT expr",
	"expr : '-' expr",
	"expr : factor",
	"for_expr : variable",
	"for_expr : list",
	"for_idlist : id_list",
	"where_clause : WHERE expr",
	"where_clause : /* empty */",
	"group_expr : grpfunc",
	"group_expr : grpfunc '(' expr ')'",
	"group_expr_sa : grpfunc '(' expr ')'",
	"grpfunc : COUNT",
	"grpfunc : SUM",
	"grpfunc : MIN",
	"grpfunc : MAX",
	"factor : '(' expr ')'",
	"factor : IDENTIFIER arg_list",
	"factor : IDENTIFIER ASSIGN_OP expr",
	"factor : group_expr_sa",
	"factor : INTEGER",
	"factor : FLOAT",
	"factor : STRING",
	"factor : BOOLEAN",
	"factor : KW_NULL",
	"factor : variable",
	"factor : OLD variable",
	"factor : ROWNUM '(' expr ')'",
	"factor : list",
	"variable : IDENTIFIER opt_substr",
	"variable : IDENTIFIER '[' index_list ']'",
	"variable : IDENTIFIER '.' IDENTIFIER opt_substr",
	"variable : IDENTIFIER '[' index_list ']' '.' IDENTIFIER opt_substr",
	"opt_substr : '[' expr ':' expr ']'",
	"opt_substr : /* empty */",
	"index_list : index_expr",
	"index_list : index_list ',' index_expr",
	"index_expr : expr",
	"index_expr : '*'",
	"list : '{' list_items '}'",
	"list : '{' '}'",
	"list_items : expr",
	"list_items : list_items ',' expr",
	"arg_list : '(' args ')'",
	"args : expr",
	"args : args ',' expr",
	"block : block_stmt",
	"block : block ';' block_stmt",
	"block_stmt : expr",
	"block_stmt : /* empty */",
};
#endif /* YYDEBUG */
#define YYFLAG  (-3000)
/* @(#) $Revision: 70.7 $ */    

/*
** Skeleton parser driver for yacc output
*/

#if defined(NLS) && !defined(NL_SETN)
#include <msgbuf.h>
#endif

#ifndef nl_msg
#define nl_msg(i,s) (s)
#endif

/*
** yacc user known macros and defines
*/
#define YYERROR		goto yyerrlab

#ifndef __RUNTIME_YYMAXDEPTH
#define YYACCEPT	return(0)
#define YYABORT		return(1)
#else
#define YYACCEPT	{free_stacks(); return(0);}
#define YYABORT		{free_stacks(); return(1);}
#endif

#define YYBACKUP( newtoken, newvalue )\
{\
	if ( yychar >= 0 || ( yyr2[ yytmp ] >> 1 ) != 1 )\
	{\
		yyerror( (nl_msg(30001,"syntax error - cannot backup")) );\
		goto yyerrlab;\
	}\
	yychar = newtoken;\
	yystate = *yyps;\
	yylval = newvalue;\
	goto yynewstate;\
}
#define YYRECOVERING()	(!!yyerrflag)
#ifndef YYDEBUG
#	define YYDEBUG	1	/* make debugging available */
#endif

/*
** user known globals
*/
int yydebug;			/* set to 1 to get debugging */

/*
** driver internal defines
*/
/* define for YYFLAG now generated by yacc program. */
/*#define YYFLAG		(FLAGVAL)*/

/*
** global variables used by the parser
*/
# ifndef __RUNTIME_YYMAXDEPTH
__YYSCLASS YYSTYPE yyv[ YYMAXDEPTH ];	/* value stack */
__YYSCLASS int yys[ YYMAXDEPTH ];		/* state stack */
# else
__YYSCLASS YYSTYPE *yyv;			/* pointer to malloc'ed value stack */
__YYSCLASS int *yys;			/* pointer to malloc'ed stack stack */

#if defined(__STDC__) || defined (__cplusplus)
#include <stdlib.h>
#else
	extern char *malloc();
	extern char *realloc();
	extern void free();
#endif /* __STDC__ or __cplusplus */


static int allocate_stacks(); 
static void free_stacks();
# ifndef YYINCREMENT
# define YYINCREMENT (YYMAXDEPTH/2) + 10
# endif
# endif	/* __RUNTIME_YYMAXDEPTH */
long  yymaxdepth = YYMAXDEPTH;

__YYSCLASS YYSTYPE *yypv;			/* top of value stack */
__YYSCLASS int *yyps;			/* top of state stack */

__YYSCLASS int yystate;			/* current state */
__YYSCLASS int yytmp;			/* extra var (lasts between blocks) */

int yynerrs;			/* number of errors */
__YYSCLASS int yyerrflag;			/* error recovery flag */
int yychar;			/* current input token number */



/*
** yyparse - return 0 if worked, 1 if syntax error not recovered from
*/
int
yyparse()
{
	register YYSTYPE *yypvt;	/* top of value stack for $vars */

	/*
	** Initialize externals - yyparse may be called more than once
	*/
# ifdef __RUNTIME_YYMAXDEPTH
	if (allocate_stacks()) YYABORT;
# endif
	yypv = &yyv[-1];
	yyps = &yys[-1];
	yystate = 0;
	yytmp = 0;
	yynerrs = 0;
	yyerrflag = 0;
	yychar = -1;

	goto yystack;
	{
		register YYSTYPE *yy_pv;	/* top of value stack */
		register int *yy_ps;		/* top of state stack */
		register int yy_state;		/* current state */
		register int  yy_n;		/* internal state number info */

		/*
		** get globals into registers.
		** branch to here only if YYBACKUP was called.
		*/
	yynewstate:
		yy_pv = yypv;
		yy_ps = yyps;
		yy_state = yystate;
		goto yy_newstate;

		/*
		** get globals into registers.
		** either we just started, or we just finished a reduction
		*/
	yystack:
		yy_pv = yypv;
		yy_ps = yyps;
		yy_state = yystate;

		/*
		** top of for (;;) loop while no reductions done
		*/
	yy_stack:
		/*
		** put a state and value onto the stacks
		*/
#if YYDEBUG
		/*
		** if debugging, look up token value in list of value vs.
		** name pairs.  0 and negative (-1) are special values.
		** Note: linear search is used since time is not a real
		** consideration while debugging.
		*/
		if ( yydebug )
		{
			register int yy_i;

			printf( "State %d, token ", yy_state );
			if ( yychar == 0 )
				printf( "end-of-file\n" );
			else if ( yychar < 0 )
				printf( "-none-\n" );
			else
			{
				for ( yy_i = 0; yytoks[yy_i].t_val >= 0;
					yy_i++ )
				{
					if ( yytoks[yy_i].t_val == yychar )
						break;
				}
				printf( "%s\n", yytoks[yy_i].t_name );
			}
		}
#endif /* YYDEBUG */
		if ( ++yy_ps >= &yys[ yymaxdepth ] )	/* room on stack? */
		{
# ifndef __RUNTIME_YYMAXDEPTH
			yyerror( (nl_msg(30002,"yacc stack overflow")) );
			YYABORT;
# else
			/* save old stack bases to recalculate pointers */
			YYSTYPE * yyv_old = yyv;
			int * yys_old = yys;
			yymaxdepth += YYINCREMENT;
			yys = (int *) realloc(yys, yymaxdepth * sizeof(int));
			yyv = (YYSTYPE *) realloc(yyv, yymaxdepth * sizeof(YYSTYPE));
			if (yys==0 || yyv==0) {
			    yyerror( (nl_msg(30002,"yacc stack overflow")) );
			    YYABORT;
			    }
			/* Reset pointers into stack */
			yy_ps = (yy_ps - yys_old) + yys;
			yyps = (yyps - yys_old) + yys;
			yy_pv = (yy_pv - yyv_old) + yyv;
			yypv = (yypv - yyv_old) + yyv;
# endif

		}
		*yy_ps = yy_state;
		*++yy_pv = yyval;

		/*
		** we have a new state - find out what to do
		*/
	yy_newstate:
		if ( ( yy_n = yypact[ yy_state ] ) <= YYFLAG )
			goto yydefault;		/* simple state */
#if YYDEBUG
		/*
		** if debugging, need to mark whether new token grabbed
		*/
		yytmp = yychar < 0;
#endif
		if ( ( yychar < 0 ) && ( ( yychar = yylex() ) < 0 ) )
			yychar = 0;		/* reached EOF */
#if YYDEBUG
		if ( yydebug && yytmp )
		{
			register int yy_i;

			printf( "Received token " );
			if ( yychar == 0 )
				printf( "end-of-file\n" );
			else if ( yychar < 0 )
				printf( "-none-\n" );
			else
			{
				for ( yy_i = 0; yytoks[yy_i].t_val >= 0;
					yy_i++ )
				{
					if ( yytoks[yy_i].t_val == yychar )
						break;
				}
				printf( "%s\n", yytoks[yy_i].t_name );
			}
		}
#endif /* YYDEBUG */
		if ( ( ( yy_n += yychar ) < 0 ) || ( yy_n >= YYLAST ) )
			goto yydefault;
		if ( yychk[ yy_n = yyact[ yy_n ] ] == yychar )	/*valid shift*/
		{
			yychar = -1;
			yyval = yylval;
			yy_state = yy_n;
			if ( yyerrflag > 0 )
				yyerrflag--;
			goto yy_stack;
		}

	yydefault:
		if ( ( yy_n = yydef[ yy_state ] ) == -2 )
		{
#if YYDEBUG
			yytmp = yychar < 0;
#endif
			if ( ( yychar < 0 ) && ( ( yychar = yylex() ) < 0 ) )
				yychar = 0;		/* reached EOF */
#if YYDEBUG
			if ( yydebug && yytmp )
			{
				register int yy_i;

				printf( "Received token " );
				if ( yychar == 0 )
					printf( "end-of-file\n" );
				else if ( yychar < 0 )
					printf( "-none-\n" );
				else
				{
					for ( yy_i = 0;
						yytoks[yy_i].t_val >= 0;
						yy_i++ )
					{
						if ( yytoks[yy_i].t_val
							== yychar )
						{
							break;
						}
					}
					printf( "%s\n", yytoks[yy_i].t_name );
				}
			}
#endif /* YYDEBUG */
			/*
			** look through exception table
			*/
			{
				register int *yyxi = yyexca;

				while ( ( *yyxi != -1 ) ||
					( yyxi[1] != yy_state ) )
				{
					yyxi += 2;
				}
				while ( ( *(yyxi += 2) >= 0 ) &&
					( *yyxi != yychar ) )
					;
				if ( ( yy_n = yyxi[1] ) < 0 )
					YYACCEPT;
			}
		}

		/*
		** check for syntax error
		*/
		if ( yy_n == 0 )	/* have an error */
		{
			/* no worry about speed here! */
			switch ( yyerrflag )
			{
			case 0:		/* new error */
				yyerror( (nl_msg(30003,"syntax error")) );
				yynerrs++;
				goto skip_init;
			yyerrlab:
				/*
				** get globals into registers.
				** we have a user generated syntax type error
				*/
				yy_pv = yypv;
				yy_ps = yyps;
				yy_state = yystate;
				yynerrs++;
			skip_init:
			case 1:
			case 2:		/* incompletely recovered error */
					/* try again... */
				yyerrflag = 3;
				/*
				** find state where "error" is a legal
				** shift action
				*/
				while ( yy_ps >= yys )
				{
					yy_n = yypact[ *yy_ps ] + YYERRCODE;
					if ( yy_n >= 0 && yy_n < YYLAST &&
						yychk[yyact[yy_n]] == YYERRCODE)					{
						/*
						** simulate shift of "error"
						*/
						yy_state = yyact[ yy_n ];
						goto yy_stack;
					}
					/*
					** current state has no shift on
					** "error", pop stack
					*/
#if YYDEBUG
#	define _POP_ "Error recovery pops state %d, uncovers state %d\n"
					if ( yydebug )
						printf( _POP_, *yy_ps,
							yy_ps[-1] );
#	undef _POP_
#endif
					yy_ps--;
					yy_pv--;
				}
				/*
				** there is no state on stack with "error" as
				** a valid shift.  give up.
				*/
				YYABORT;
			case 3:		/* no shift yet; eat a token */
#if YYDEBUG
				/*
				** if debugging, look up token in list of
				** pairs.  0 and negative shouldn't occur,
				** but since timing doesn't matter when
				** debugging, it doesn't hurt to leave the
				** tests here.
				*/
				if ( yydebug )
				{
					register int yy_i;

					printf( "Error recovery discards " );
					if ( yychar == 0 )
						printf( "token end-of-file\n" );
					else if ( yychar < 0 )
						printf( "token -none-\n" );
					else
					{
						for ( yy_i = 0;
							yytoks[yy_i].t_val >= 0;
							yy_i++ )
						{
							if ( yytoks[yy_i].t_val
								== yychar )
							{
								break;
							}
						}
						printf( "token %s\n",
							yytoks[yy_i].t_name );
					}
				}
#endif /* YYDEBUG */
				if ( yychar == 0 )	/* reached EOF. quit */
					YYABORT;
				yychar = -1;
				goto yy_newstate;
			}
		}/* end if ( yy_n == 0 ) */
		/*
		** reduction by production yy_n
		** put stack tops, etc. so things right after switch
		*/
#if YYDEBUG
		/*
		** if debugging, print the string that is the user's
		** specification of the reduction which is just about
		** to be done.
		*/
		if ( yydebug )
			printf( "Reduce by (%d) \"%s\"\n",
				yy_n, yyreds[ yy_n ] );
#endif
		yytmp = yy_n;			/* value to switch over */
		yypvt = yy_pv;			/* $vars top of value stack */
		/*
		** Look in goto table for next state
		** Sorry about using yy_state here as temporary
		** register variable, but why not, if it works...
		** If yyr2[ yy_n ] doesn't have the low order bit
		** set, then there is no action to be done for
		** this reduction.  So, no saving & unsaving of
		** registers done.  The only difference between the
		** code just after the if and the body of the if is
		** the goto yy_stack in the body.  This way the test
		** can be made before the choice of what to do is needed.
		*/
		{
			/* length of production doubled with extra bit */
			register int yy_len = yyr2[ yy_n ];

			if ( !( yy_len & 01 ) )
			{
				yy_len >>= 1;
				yyval = ( yy_pv -= yy_len )[1];	/* $$ = $1 */
				yy_state = yypgo[ yy_n = yyr1[ yy_n ] ] +
					*( yy_ps -= yy_len ) + 1;
				if ( yy_state >= YYLAST ||
					yychk[ yy_state =
					yyact[ yy_state ] ] != -yy_n )
				{
					yy_state = yyact[ yypgo[ yy_n ] ];
				}
				goto yy_stack;
			}
			yy_len >>= 1;
			yyval = ( yy_pv -= yy_len )[1];	/* $$ = $1 */
			yy_state = yypgo[ yy_n = yyr1[ yy_n ] ] +
				*( yy_ps -= yy_len ) + 1;
			if ( yy_state >= YYLAST ||
				yychk[ yy_state = yyact[ yy_state ] ] != -yy_n )
			{
				yy_state = yyact[ yypgo[ yy_n ] ];
			}
		}
					/* save until reenter driver code */
		yystate = yy_state;
		yyps = yy_ps;
		yypv = yy_pv;
	}
	/*
	** code supplied by user is placed in this switch
	*/
	switch( yytmp )
	{
		
case 10:
# line 149 "bre_yac.y"
{ declare_extfunc(yypvt[-1].str, yypvt[-0].idlist); } break;
case 11:
# line 153 "bre_yac.y"
{ declare_lookup(yypvt[-7].str, yypvt[-5].idlist, yypvt[-1].idlist); } break;
case 12:
# line 156 "bre_yac.y"
{ declare_constant(yypvt[-2].str, yypvt[-0].expr); } break;
case 13:
# line 159 "bre_yac.y"
{ declare_func(yypvt[-2].str, yypvt[-1].idlist); } break;
case 14:
# line 160 "bre_yac.y"
{ terminate_func(); } break;
case 15:
# line 163 "bre_yac.y"
{ declare_group(yypvt[-2].str, yypvt[-1].str); } break;
case 16:
# line 164 "bre_yac.y"
{ terminate_group(yypvt[-0].expr); } break;
case 17:
# line 167 "bre_yac.y"
{ declare_rule(yypvt[-2].str, yypvt[-1].str); } break;
case 18:
# line 168 "bre_yac.y"
{ terminate_rule(yypvt[-0].expr); } break;
case 19:
# line 171 "bre_yac.y"
{ yyval.idlist = yypvt[-1].idlist; } break;
case 20:
# line 172 "bre_yac.y"
{ yyval.idlist = NULL; } break;
case 21:
# line 175 "bre_yac.y"
{ yyval.idlist = idlist(NULL, yypvt[-0].str); } break;
case 22:
# line 176 "bre_yac.y"
{ yyval.idlist = idlist(yypvt[-2].idlist, yypvt[-0].str); } break;
case 23:
# line 179 "bre_yac.y"
{ yyval.str = yypvt[-0].str; } break;
case 24:
# line 180 "bre_yac.y"
{ yyval.str = make_ruleid(yypvt[-0].l); } break;
case 25:
# line 183 "bre_yac.y"
{ yyval.str = yypvt[-0].str; } break;
case 26:
# line 184 "bre_yac.y"
{ yyval.str = NULL; } break;
case 27:
# line 187 "bre_yac.y"
{ yyval.expr = expr_error(E_WARN, yypvt[-0].expr); } break;
case 28:
# line 188 "bre_yac.y"
{ yyval.expr = expr_error(E_FAIL, yypvt[-0].expr); } break;
case 29:
# line 189 "bre_yac.y"
{ yyval.expr = NULL; } break;
case 30:
# line 192 "bre_yac.y"
{ yyval.expr = expr_args(NULL, yypvt[-0].expr); } break;
case 31:
# line 193 "bre_yac.y"
{ yyval.expr = expr_args(yypvt[-2].expr, yypvt[-0].expr); } break;
case 32:
# line 196 "bre_yac.y"
{ statement(yypvt[-0].expr); } break;
case 33:
# line 197 "bre_yac.y"
{ statement(yypvt[-0].expr); } break;
case 34:
# line 200 "bre_yac.y"
{ yyval.expr = NULL; } break;
case 35:
# line 201 "bre_yac.y"
{ yyval.expr = yypvt[-0].expr; } break;
case 36:
# line 202 "bre_yac.y"
{ yyval.expr = NULL; } break;
case 37:
# line 205 "bre_yac.y"
{ declare_type(yypvt[-2].idlist, yypvt[-0].type); } break;
case 38:
# line 208 "bre_yac.y"
{ yyval.type = basic_type(KW_INTEGER); } break;
case 39:
# line 209 "bre_yac.y"
{ yyval.type = list_type(KW_INTEGER); } break;
case 40:
# line 210 "bre_yac.y"
{ yyval.type = basic_type(KW_STRING); } break;
case 41:
# line 211 "bre_yac.y"
{ yyval.type = list_type(KW_STRING); } break;
case 42:
# line 212 "bre_yac.y"
{ yyval.type = basic_type(KW_FLOAT); } break;
case 43:
# line 213 "bre_yac.y"
{ yyval.type = list_type(KW_FLOAT); } break;
case 44:
# line 214 "bre_yac.y"
{ yyval.type = basic_type(KW_BOOLEAN); } break;
case 45:
# line 215 "bre_yac.y"
{ yyval.type = list_type(KW_BOOLEAN); } break;
case 46:
# line 216 "bre_yac.y"
{ yyval.type = complex_type(yypvt[-0].str); } break;
case 47:
# line 217 "bre_yac.y"
{ yyval.type = complex_list_type(yypvt[-1].str); } break;
case 48:
# line 220 "bre_yac.y"
{ yyval.expr = expr_iterator(E_FORALL, yypvt[-2].expr, yypvt[-0].expr); } break;
case 49:
# line 222 "bre_yac.y"
{ yyval.expr = expr_for(E_FORALL, yypvt[-1].expr, yypvt[-0].expr); } break;
case 50:
# line 224 "bre_yac.y"
{ yyval.expr = expr_iterator(E_FORSOME, yypvt[-2].expr, yypvt[-0].expr); } break;
case 51:
# line 226 "bre_yac.y"
{ yyval.expr = expr_for(E_FORSOME, yypvt[-1].expr, yypvt[-0].expr); } break;
case 52:
# line 229 "bre_yac.y"
{ yyval.expr = expr_iterator(E_COMPUTE, yypvt[-2].expr, yypvt[-0].expr); } break;
case 53:
# line 231 "bre_yac.y"
{ yyval.expr = expr_compute(yypvt[-1].expr, yypvt[-3].expr, yypvt[-0].expr); } break;
case 54:
# line 234 "bre_yac.y"
{ yyval.expr = expr_ifelse(yypvt[-4].expr, yypvt[-2].expr, yypvt[-0].expr); } break;
case 55:
# line 236 "bre_yac.y"
{ yyval.expr = expr_if(yypvt[-2].expr, yypvt[-0].expr); } break;
case 56:
# line 238 "bre_yac.y"
{ yyval.expr = expr_block(yypvt[-1].expr); } break;
case 57:
# line 240 "bre_yac.y"
{ yyval.expr = expr_logical(E_OR, yypvt[-2].expr, yypvt[-0].expr); } break;
case 58:
# line 241 "bre_yac.y"
{ yyval.expr = expr_logical(E_AND, yypvt[-2].expr, yypvt[-0].expr); } break;
case 59:
# line 242 "bre_yac.y"
{ yyval.expr = expr_in(E_IN, yypvt[-2].expr, yypvt[-0].expr); } break;
case 60:
# line 243 "bre_yac.y"
{ yyval.expr = expr_between(E_BETWEEN, yypvt[-2].expr, yypvt[-0].expr); } break;
case 61:
# line 244 "bre_yac.y"
{ yyval.expr = expr_in(E_NIN, yypvt[-3].expr, yypvt[-0].expr); } break;
case 62:
# line 245 "bre_yac.y"
{ yyval.expr = expr_between(E_NBETWEEN, yypvt[-3].expr, yypvt[-0].expr); } break;
case 63:
# line 246 "bre_yac.y"
{ yyval.expr = expr_like(E_LIKE, yypvt[-2].expr, yypvt[-0].expr); } break;
case 64:
# line 247 "bre_yac.y"
{ yyval.expr = expr_like(E_NLIKE, yypvt[-3].expr, yypvt[-0].expr); } break;
case 65:
# line 248 "bre_yac.y"
{ yyval.expr = expr_equality(E_EQ, yypvt[-2].expr, yypvt[-0].expr); } break;
case 66:
# line 249 "bre_yac.y"
{ yyval.expr = expr_equality(E_NE, yypvt[-2].expr, yypvt[-0].expr); } break;
case 67:
# line 250 "bre_yac.y"
{ yyval.expr = expr_relational(E_LT, yypvt[-2].expr, yypvt[-0].expr); } break;
case 68:
# line 251 "bre_yac.y"
{ yyval.expr = expr_relational(E_LE, yypvt[-2].expr, yypvt[-0].expr); } break;
case 69:
# line 252 "bre_yac.y"
{ yyval.expr = expr_relational(E_GT, yypvt[-2].expr, yypvt[-0].expr); } break;
case 70:
# line 253 "bre_yac.y"
{ yyval.expr = expr_relational(E_GE, yypvt[-2].expr, yypvt[-0].expr); } break;
case 71:
# line 254 "bre_yac.y"
{ yyval.expr = expr_add(yypvt[-2].expr, yypvt[-0].expr); } break;
case 72:
# line 255 "bre_yac.y"
{ yyval.expr = expr_arithmetic(E_SUB, yypvt[-2].expr, yypvt[-0].expr); } break;
case 73:
# line 256 "bre_yac.y"
{ yyval.expr = expr_arithmetic(E_MUL, yypvt[-2].expr, yypvt[-0].expr); } break;
case 74:
# line 257 "bre_yac.y"
{ yyval.expr = expr_arithmetic(E_DIV, yypvt[-2].expr, yypvt[-0].expr); } break;
case 75:
# line 258 "bre_yac.y"
{ yyval.expr = expr_arithmetic(E_MOD, yypvt[-2].expr, yypvt[-0].expr); } break;
case 76:
# line 259 "bre_yac.y"
{ yyval.expr = expr_logical(E_NOT, NULL, yypvt[-0].expr); } break;
case 77:
# line 260 "bre_yac.y"
{ yyval.expr = expr_arithmetic(E_NEG, NULL, yypvt[-0].expr); } break;
case 78:
# line 261 "bre_yac.y"
{ yyval.expr = yypvt[-0].expr; } break;
case 79:
# line 264 "bre_yac.y"
{ yyval.expr = yypvt[-0].expr; } break;
case 80:
# line 265 "bre_yac.y"
{ yyval.expr = yypvt[-0].expr; } break;
case 81:
# line 268 "bre_yac.y"
{ yyval.expr = expr_for_idlist(yypvt[-0].idlist); } break;
case 82:
# line 271 "bre_yac.y"
{ yyval.expr = yypvt[-0].expr; } break;
case 83:
# line 272 "bre_yac.y"
{ yyval.expr = NULL; } break;
case 84:
# line 275 "bre_yac.y"
{ yyval.expr = expr_grpfunc(yypvt[-0].op, NULL); } break;
case 85:
# line 276 "bre_yac.y"
{ yyval.expr = expr_grpfunc(yypvt[-3].op, yypvt[-1].expr); } break;
case 86:
# line 279 "bre_yac.y"
{ yyval.expr = expr_grpfunc_sa(yypvt[-3].op, yypvt[-1].expr); } break;
case 87:
# line 282 "bre_yac.y"
{ yyval.op = E_COUNT; } break;
case 88:
# line 283 "bre_yac.y"
{ yyval.op = E_SUM; } break;
case 89:
# line 284 "bre_yac.y"
{ yyval.op = E_MIN; } break;
case 90:
# line 285 "bre_yac.y"
{ yyval.op = E_MAX; } break;
case 91:
# line 288 "bre_yac.y"
{ yyval.expr = yypvt[-1].expr; } break;
case 92:
# line 289 "bre_yac.y"
{ yyval.expr = expr_call(yypvt[-1].str,yypvt[-0].expr); } break;
case 93:
# line 290 "bre_yac.y"
{ yyval.expr = expr_assign(yypvt[-2].str, yypvt[-0].expr); } break;
case 94:
# line 291 "bre_yac.y"
{ yyval.expr = yypvt[-0].expr; } break;
case 95:
# line 292 "bre_yac.y"
{ yyval.expr = expr_integer(yypvt[-0].l); } break;
case 96:
# line 293 "bre_yac.y"
{ yyval.expr = expr_float(yypvt[-0].d); } break;
case 97:
# line 294 "bre_yac.y"
{ yyval.expr = expr_string(yypvt[-0].str); } break;
case 98:
# line 295 "bre_yac.y"
{ yyval.expr = expr_boolean(yypvt[-0].l); } break;
case 99:
# line 296 "bre_yac.y"
{ yyval.expr = expr_null(); } break;
case 100:
# line 297 "bre_yac.y"
{ yyval.expr = yypvt[-0].expr; } break;
case 101:
# line 298 "bre_yac.y"
{ yyval.expr = expr_old(yypvt[-0].expr); } break;
case 102:
# line 299 "bre_yac.y"
{ yyval.expr = expr_rownum(yypvt[-1].expr); } break;
case 103:
# line 300 "bre_yac.y"
{ yyval.expr = yypvt[-0].expr; } break;
case 104:
# line 303 "bre_yac.y"
{ yyval.expr = expr_variable(yypvt[-1].str, yypvt[-0].expr); } break;
case 105:
# line 304 "bre_yac.y"
{ yyval.expr = expr_indexed_table(yypvt[-3].str, yypvt[-1].expr); } break;
case 106:
# line 305 "bre_yac.y"
{ yyval.expr = expr_tablefield(yypvt[-3].str, yypvt[-1].str, yypvt[-0].expr); } break;
case 107:
# line 307 "bre_yac.y"
{ yyval.expr = expr_indexed_tablefield(yypvt[-6].str, yypvt[-4].expr, yypvt[-1].str, yypvt[-0].expr); } break;
case 108:
# line 310 "bre_yac.y"
{ yyval.expr = expr_substr(yypvt[-3].expr, yypvt[-1].expr); } break;
case 109:
# line 311 "bre_yac.y"
{ yyval.expr = NULL; } break;
case 110:
# line 314 "bre_yac.y"
{ yyval.expr = expr_args(NULL, yypvt[-0].expr); } break;
case 111:
# line 315 "bre_yac.y"
{ yyval.expr = expr_args(yypvt[-2].expr, yypvt[-0].expr); } break;
case 112:
# line 318 "bre_yac.y"
{ yyval.expr = yypvt[-0].expr; } break;
case 113:
# line 319 "bre_yac.y"
{ yyval.expr = expr_null(); } break;
case 114:
# line 322 "bre_yac.y"
{ yyval.expr = expr_list(yypvt[-1].expr); } break;
case 115:
# line 323 "bre_yac.y"
{ yyval.expr = expr_list(NULL); } break;
case 116:
# line 326 "bre_yac.y"
{ yyval.expr = expr_args(NULL, yypvt[-0].expr); } break;
case 117:
# line 327 "bre_yac.y"
{ yyval.expr = expr_args(yypvt[-2].expr, yypvt[-0].expr); } break;
case 118:
# line 330 "bre_yac.y"
{ yyval.expr = yypvt[-1].expr; } break;
case 119:
# line 333 "bre_yac.y"
{ yyval.expr = expr_args(NULL, yypvt[-0].expr); } break;
case 120:
# line 334 "bre_yac.y"
{ yyval.expr = expr_args(yypvt[-2].expr, yypvt[-0].expr); } break;
case 121:
# line 337 "bre_yac.y"
{ yyval.expr = expr_args(NULL, yypvt[-0].expr); } break;
case 122:
# line 338 "bre_yac.y"
{ yyval.expr = expr_args(yypvt[-2].expr, yypvt[-0].expr); } break;
case 123:
# line 341 "bre_yac.y"
{ yyval.expr = yypvt[-0].expr; } break;
case 124:
# line 342 "bre_yac.y"
{ yyval.expr = NULL; } break;
	}
	goto yystack;		/* reset registers in driver code */
}

# ifdef __RUNTIME_YYMAXDEPTH

static int allocate_stacks() {
	/* allocate the yys and yyv stacks */
	yys = (int *) malloc(yymaxdepth * sizeof(int));
	yyv = (YYSTYPE *) malloc(yymaxdepth * sizeof(YYSTYPE));

	if (yys==0 || yyv==0) {
	   yyerror( (nl_msg(30004,"unable to allocate space for yacc stacks")) );
	   return(1);
	   }
	else return(0);

}


static void free_stacks() {
	if (yys!=0) free((char *) yys);
	if (yyv!=0) free((char *) yyv);
}

# endif  /* defined(__RUNTIME_YYMAXDEPTH) */

