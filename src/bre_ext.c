/*** $Id: bre_ext.c,v 1.2 1998/01/19 12:15:18 stauffer Exp stauffer $ ***/

/*
 */
#ifndef lint
static char rcsid[] = "@(#)$Id: bre_ext.c,v 1.2 1998/01/19 12:15:18 stauffer Exp stauffer $";
#endif

/**********************************************************************
 * BRE_EXT.C
 *
 *	Builtin external functions.
 *
 *	DATE	- A lookup table that maps a date string into
 *		  a lookup record.
 *
 *	today_date - A function, returns a string representing todays
 *		  date. Ie. "19980119"
 *
 *	embedded_spaces(str)	- Returns true if the string 'str'
 *		contains spaces embedded between non-space characters.
 *
 */

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <ctype.h>

#include "bre_api.h"
#include "bre_prv.h"

/*----------------------------------------------------------------------
 * normalize
 *
 * calculate the number of seconds between January 1, 1970
 * and year/month/day. Return that value.
 *
 * This function does not worry about time zone.
 *
 */
#define leap(y)	(((y % 4 == 0) && (y % 100 != 0)) || (y % 400 == 0))
#define DAYSEC	(3600*24)
#define YERSEC	(3600*24*365)
#define HOURSEC	(3600)
#define MINSEC	(60)
#define TIME0	1970

static int months[13] =
	{0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

static time_t normalize(int year, int month, int day, int hour, int minute, int sec)
{

	int yeardays, leapers, x;
	time_t seconds;

	yeardays = 0;
	for(x=1; x<month; x++)
		yeardays += months[x];

	if( (month > 2) && leap(year) )
		yeardays++;

	leapers = 0;
	for(x=TIME0; x<year; x++)
		if(leap(x)) leapers++;
	
	seconds = yeardays*DAYSEC + (year-TIME0)*YERSEC + leapers*DAYSEC
			+ (day-1)*DAYSEC + hour*HOURSEC + minute*MINSEC + sec;

	return seconds;
}


/*----------------------------------------------------------------------
 * This callback function 'date()' should be installed as:
 *	"DATE", "S", "IISIIIII"
 *
 * Date parsed out the information from a date string, and returns
 * all the components.
 *
 * external lookup date[date_string].{YEAR, MONTH, MONTH-STR, DAY, HOUR, MINUTE, SECOND, N}
 *
 *	FIELD		TYPE		DESCRIPTION
 *	YEAR		- integer	year, eg. 1988
 *	MONTH		- integer	month, eg. 3
 *	MONTH-STR	- string	month as string, eg. "January"
 *	DAY		- integer	day of month, eg. 4
 *	HOUR		- integer	hour (0-23)
 *	MINUTE		- integer	minute (0-59)
 *	SECOND		- integer	second (0-59)
 *	N		- integer	normalized time
 *
 * rule ex1 is
 *	x: date;
 *
 *	for all li in ORDER_LINE_ITEMS
 *		x := date[ li.SHIP-DATE ];
 *
 *		if x.year < 1970 then
 *			x.month <> 3
 *
 *		date[li.RECV-DATE].N > date[li.SHIP-DATE].N
 *
 *		date["19981203"].MONTH = 12 
 *
 *		date["19984488"] = null 		--<--- Bogus date
 *
 * The format of the string 'date_string' is expected to be:
 *	"YYYYMMDD"		(len=8)
 *		or....
 *	"YYYYMMDD HH:MM:SS"	(len=17)
 *	 01234567890123456
 *
 * If HH:MM:SS is omitted, then those fields will be set to:
 *		00:00:00
 *
 * If date is invalid, this function returns NULL.
 *
 */
static int date(char *name, void *external_cookie, BRE_DATUM *in, BRE_DATUM *out)
{
	char *date_str, *p, *month_str;
	int date_len;
	int year, month, day;
	int hour, minute, second;
	time_t n;
	static char *monthstrs[12] = {
		"January", "February", "March", "April", "May", "June",
		"July", "August", "September", "October", "November", "December" };

	if( in[0].s.str == NULL || in[0].s.len == 0 )
		return 1;

	date_len = in[0].s.len;
	date_str = in[0].s.str;

	if( date_len != 8 && date_len != 17)
		return 1;

	for(p=date_str; *p; p++) {
		if( strchr("0123456789", *p) == NULL )
			return 1;
	}

	year = (date_str[0] - '0')*1000
			+ (date_str[1] - '0')*100
			+ (date_str[2] - '0')*10
			+ (date_str[3] - '0');

	month = (date_str[4] - '0')*10 + (date_str[5] - '0');
	day = (date_str[6] - '0')*10 + (date_str[7] - '0');

	if( month > 12 || month < 1 || year < TIME0 || day < 1
			|| day > months[month] + (month==2 && leap(year)) )
			return 1;

	if( date_len == 17 ) {
		hour	= (date_str[ 9] - '0')*10 + (date_str[10] - '0');
		minute	= (date_str[12] - '0')*10 + (date_str[13] - '0');
		second	= (date_str[15] - '0')*10 + (date_str[16] - '0');
	} else {
		hour	= 0;
		minute	= 0;
		second	= 0;
	}

	if( hour > 23 || minute > 59 || second > 59 )
		return 1;

	n = normalize(year, month, day, hour, minute, second);

	month_str = monthstrs[month-1];

	out[0].i.nil = 0;
	out[0].i.val = year;

	out[1].i.nil = 0;
	out[1].i.val = month;

	out[2].s.len = strlen(month_str);
	out[2].s.str = month_str;

	out[3].i.nil = 0;
	out[3].i.val = day;

	out[4].i.nil = 0;
	out[4].i.val = hour;

	out[5].i.nil = 0;
	out[5].i.val = minute;

	out[6].i.nil = 0;
	out[6].i.val = second;

	out[7].i.nil = 0;
	out[7].i.val = n;

	return 0;
}

/*----------------------------------------------------------------------
 * Get todays date and return a string of the form:
 *	"yyyymmdd" (ie. "19980304")
 *
 */
static int today_date(char *name, void *external_cookie, BRE_DATUM *in, BRE_DATUM *out)
{
	static char today[100];
	time_t t;
	struct tm *tm;

	t = time(NULL);
	tm = localtime(&t);

	sprintf(today, "%04d%02d%02d",	tm->tm_year + 1900,
					tm->tm_mon+1,
					tm->tm_mday);

	out[0].s.str = today;
	out[0].s.len = strlen(today);

	return 0;
}

/*----------------------------------------------------------------------
 * This functions looks at the string in[0].s.str and checks to
 * see if it contains any embedded spaces.
 *
 *	"testing"		-> false
 *	"   testing   "		-> false
 *	"     "			-> false
 *	""			-> false
 *	"   test string "	-> true
 *	"te st str ing"		-> true
 *
 */
static int embedded_spaces(char *name, void *external_cookie, BRE_DATUM *in, BRE_DATUM *out)
{
	char *str, *p;
	int len;
	enum { ES_INIT, ES_NONSPACE, ES_SPACE, ES_EMBEDDED } state;

	str = in[0].s.str;
	len = in[0].s.len;

	if( str == NULL || len == 0 ) {
		out[0].i.nil = 0;
		out[0].i.val = 0;
		return 0;
	}

	state = ES_INIT;
	for(p=str; *p; p++) {
		switch( state ) {
		case ES_INIT:
			if( !isspace(*p) )
				state = ES_NONSPACE;
			break;

		case ES_NONSPACE:
			if( isspace(*p) )
				state = ES_SPACE;
			break;

		case ES_SPACE:
			if( !isspace(*p) )
				state = ES_EMBEDDED;
			break;

		case ES_EMBEDDED:
			break;

		default:
			BRE_ASSERT(0);
		}

		if( state == ES_EMBEDDED )
			break;
	}

	if( state == ES_EMBEDDED ) {
		out[0].i.nil = 0;
		out[0].i.val = 1;
	} else {
		out[0].i.nil = 0;
		out[0].i.val = 0;
	}
	return 0;
}

/*----------------------------------------------------------------------
 * Generate a fatal error.
 *
 */
static int fatal_error(char *name, void *external_cookie, BRE_DATUM *in, BRE_DATUM *out)
{
	char *str;

	if( in[0].s.str == NULL || in[0].s.len == 0 )
		str = "nil";
	else
		str = in[0].s.str;

	bre_fatal(str);

	out[0].i.nil = 0;
	out[0].i.val = 0;

	return 0;
}

/*----------------------------------------------------------------------
 * Register all the builtin functions and lookups. If you
 * add to this function, make sure to include the proto-type
 * "external" code too.
 *
 */
void bre_register_builtins_private(int regext, FILE *fp)
{
	if( regext ) {
		bre_register_external("date", NULL, "S", "IISIIIII", date);
		bre_register_external("today_date", NULL, "", "S", today_date);
		bre_register_external("embedded_spaces", NULL, "S", "B", embedded_spaces);
		bre_register_external("fatal_error", NULL, "S", "B", fatal_error);
	}

	if( fp ) {
		fprintf(fp, "external lookup date[date_string].{YEAR, MONTH,"
					" MONTH-STR, DAY, HOUR, MINUTE, SECOND, N}\n");
		fprintf(fp, "external function today_date\n");
		fprintf(fp, "external function embedded_spaces(str)\n");
		fprintf(fp, "external function fatal_error(str)\n");
	}
}

/*----------------------------------------------------------------------
 | Purpose:   To register the bre builtin functions.
 |
 | fp         - If FP is non-NULL, then write the proto-type information out
 |              to this file descriptor.
 |
 *----------------------------------------------------------------------*/
   void
bre_register_builtins(
	FILE *fp)
{
	bre_register_builtins_private(1, fp);
}
