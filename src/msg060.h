#ifndef _MSG060H_INCLUDED
#define _MSG060H_INCLUDED

/*
** WARNING: Do not modify this file by hand! 
**          This file is generated by msgcat.
*/

typedef enum
{
   BREMSG_DIE_NOMEM               = 1,
   BREMSG_DIE_REALLOC             = 2,
   BREMSG_LEX_INVCHR              = 3,
   BREMSG_LEX_STRBIG              = 4,
   BREMSG_LEX_EOFSTR              = 5,
   BREMSG_LEX_INVSTR              = 6,
   BREMSG_LEX_BIGNUM              = 7,
   BREMSG_LEX_INVNUM              = 8,
   BREMSG_LEX_EOFCOM              = 9,
   BREMSG_YACC_SYNTAX             = 10,
   BREMSG_INCOMPATABLE_TYPES      = 11,
   BREMSG_WRONG_RIGHT             = 12,
   BREMSG_WRONG_TYPES             = 13,
   BREMSG_UNKNOWN_IDENTIFIER      = 14,
   BREMSG_NOSUCH_FIELD            = 15,
   BREMSG_ILLEGAL_CONTEXT         = 16,
   BREMSG_NAME_CLASH              = 17,
   BREMSG_BAD_DECL                = 18,
   BREMSG_NOT_INIT                = 19,
   BREMSG_IF_NOBOOL               = 20,
   BREMSG_LOG_LNBOOL              = 21,
   BREMSG_LOG_RNBOOL              = 22,
   BREMSG_IN_BADRIGHT             = 23,
   BREMSG_BETWEEN_TWO             = 24,
   BREMSG_BETWEEN_RIGHT           = 25,
   BREMSG_LEFT_STR                = 26,
   BREMSG_RIGHT_STR               = 27,
   BREMSG_ADD_ERR                 = 28,
   BREMSG_NOTINTEGER              = 29,
   BREMSG_NUMERICAL               = 30,
   BREMSG_GROUP_ARGS              = 31,
   BREMSG_COUNT_ARGS              = 32,
   BREMSG_GROUP_INVTYPE           = 33,
   BREMSG_ARGS_FEW                = 34,
   BREMSG_ARGS_MANY               = 35,
   BREMSG_ARGS_INCOMPATIBLE       = 36,
   BREMSG_NOSUCH_FUNCTION         = 37,
   BREMSG_INV_FUNCCALL            = 38,
   BREMSG_INV_RECURSE             = 39,
   BREMSG_LIST_TYPE               = 40,
   BREMSG_LIST_NULL               = 41,
   BREMSG_OLD_ERR                 = 42,
   BREMSG_ROWNUM_TYPE             = 43,
   BREMSG_SUBSTR_INT1             = 44,
   BREMSG_SUBSTR_INT2             = 45,
   BREMSG_SUBSTR_COMPAT           = 46,
   BREMSG_UNDEF_VAR               = 47,
   BREMSG_INVCONTEXT_LOOKUP       = 48,
   BREMSG_ILLEGAL_INDEXING        = 49,
   BREMSG_NONINT_INDEXING         = 50,
   BREMSG_INDEX_VARPAR            = 51,
   BREMSG_VARPAR_NOTINT           = 52,
   BREMSG_MANY_KEYS               = 53,
   BREMSG_FEW_KEYS                = 54,
   BREMSG_INVTYPE_KEYS            = 55,
   BREMSG_INVINDEX_FIELD          = 56,
   BREMSG_MISSING_KEYS            = 57,
   BREMSG_CLASH_FIELDTABLE        = 58,
   BREMSG_DUPLICATE_VAR           = 59,
   BREMSG_BAD_STRITERATE          = 60,
   BREMSG_LSIZE_UNKNOWN           = 61,
   BREMSG_WRONG_NVARS             = 62,
   BREMSG_VARTYPE_NOTMATCH        = 63,
   BREMSG_ILLEGAL_MULTIVAR        = 64,
   BREMSG_NOTTABLE_EXPR           = 65,
   BREMSG_INV_TABLEEXPR           = 66,
   BREMSG_BAD_ITERATOR            = 67,
   BREMSG_BOOLEAN_REQUIRED        = 68,
   BREMSG_BADARGS_COUNT           = 69,
   BREMSG_UNKNOWN_TABLE           = 70,
   BREMSG_BAD_TYPE                = 71,
   BREMSG_BAD_REDECL              = 72,
   BREMSG_CLASH_FIELD             = 73,
   BREMSG_UNEXPECTED_DECL         = 74,
   BREMSG_VAR_CONTEXT             = 75,
   BREMSG_REDEF_CURROW            = 76,
   BREMSG_REDEF_PRVROW            = 77,
   BREMSG_RULE_CLASH              = 78,
   BREMSG_UNKNOWN_TABLENAME       = 79,
   BREMSG_TABLE_EXPECTED          = 80,
   BREMSG_EMPTY_RULE              = 81,
   BREMSG_RULE_BOOLEAN            = 82,
   BREMSG_FUNC_CLASH              = 83,
   BREMSG_DUP_PARAM               = 84,
   BREMSG_PARM_CLASH              = 85,
   BREMSG_EMPTY_FUNC              = 86,
   BREMSG_INV_RETURN              = 87,
   BREMSG_GROUP_CLASH             = 88,
   BREMSG_GROUP_USING             = 89,
   BREMSG_GROUP_RETURN            = 90,
   BREMSG_GROUP_VARS              = 91,
   BREMSG_CONST_CLASH             = 92,
   BREMSG_NON_CONSTANT            = 93,
   BREMSG_DYNAMIC_LOOKUP          = 94,
   BREMSG_FUNCPARM_UNIQUE         = 95,
   BREMSG_EXTFUNC_MANYARGS        = 96,
   BREMSG_EXTFUNC_FEWARGS         = 97,
   BREMSG_EXTFUNC_CLASH           = 98,
   BREMSG_SYM_NOTLINKED           = 99,
   BREMSG_KEY_NOTUNIQUE           = 100,
   BREMSG_MANYKEYS_LINKED         = 101,
   BREMSG_FEWKEYS_LINKED          = 102,
   BREMSG_LOOKUPFIELD_UNIQUE      = 103,
   BREMSG_MANYFIELDS_LINKED       = 104,
   BREMSG_FEWFIELDS_LINKED        = 105,
   BREMSG_EXTLOOKUP_CLASH         = 106,
   BREMSG_ERROR_ARGS              = 107,
   BREMSG_ILLEGAL_ASSIGNFIELD     = 108,
   BREMSG_NOSUCH_VAR              = 109,
   BREMSG_ILLEGAL_ASSIGN          = 110,
   BREMSG_ASSIGN_TYPES            = 111,
   BREMSG_MIXED_LIST              = 112,
   BREMSG_PARM_NOTDECL            = 113,
   MSG_SET_060_LIMIT              = 114

} MSG_SET_060;

#endif  /*_MSG060H_INCLUDED*/
