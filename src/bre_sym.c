/*** $Id$ ***/

/*
 */
#ifndef lint
static char rcsid[] = "@(#)$Id$";
#endif

/***********************************************************************
 * BRE SYMBOL TABLE ROUTINES
 */

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "bre_api.h"
#include "bre_prv.h"

#define SYM_HASH_SIZE	2011

/*----------------------------------------------------------------------
 * Symbol table data structure:
 *
 *   HashTable[ 2011 ]
 *   +----------+
 *   |		|
 *   +----------+
 *   |		|	SYMBOL
 *   +----------+	+-------+    +-------+    +-------+    +-------+
 *   |		| -->	|       | -> |       | -> |       | -> |       | -> NULL
 *   +----------+	+-------+    +-------+    +-------+    +-------+
 *   |		|
 *   +----------+
 *   |		|
 *   +----------+
 *
 *	...
 *
 *   +----------+
 *   |		|
 *   +----------+	+-------+    +-------+    +-------+
 *   |		| -->	|       | -> |       | -> |       | -> NULL
 *   +----------+	+-------+    +-------+    +-------+
 *   |		|
 *   +----------+
 *   |		|
 *   +----------+	+-------+
 *   |		| -->	|       | -> NULL
 *   +----------+	+-------+
 *   |		|
 *   +----------+
 *   |		|
 *   +----------+
 *   |		|
 *   +----------+
 *
 * Operations:
 *	LOOKUP(id) -	1) Hash identifier 'id'
 *			2) Index into HashTable[] using the hash value.
 *			3) Scan the linked list of symbols for a match.
 *			4) Return the matching symbol or NULL.
 *
 *	INSERT(id) -	1) Hash identifier 'id'
 *			2) Index into HashTable[] using the hash value.
 *			3) Allocate a new SYMBOL structure (and populate with 'id').
 *			4) Insert new symbol to the head of the linked list.
 *
 *	DETACH(sym)	1) Hash sym->name
 *			2) Index HashTable[]
 *			3) Search for symbol in linked list.
 *			4) Remove symbol from linked list.
 *
 *	REMOVE(sym)	1) Perform DETACH(sym)
 *			2) Free the symbol structure.
 */

/*----------------------------------------------------------------------
 * This is the HashTable for storing all
 * the symbols.
 *
 */
static SYMBOL *HashTable[ SYM_HASH_SIZE ];

/*----------------------------------------------------------------------
 * General purpose hash function.
 *
 * Uppercase letters are mapped to lowercase,
 * so that case distinctions are ignored.
 */
static int hash(char *str)
{
	char *p, c;
	unsigned h=0, g;

	for(p=str; *p; p++) {
		c = tolower(*p);
		h = (h<<4) + c;
		if( g = h&0xf0000000 ) {
			h = h ^ (g >> 24);
			h = h ^ g;
		}
	}
	return h % SYM_HASH_SIZE;
}

/*----------------------------------------------------------------------
 * Compare two symbols.
 * In the BRE language case distinctions do not matter.
 *
 */
int symbol_match(char *str1, char *str2)
{
	return !strcasecmp(str1, str2);
}

SYMBOL *sym_lookup(char *name)
{
	SYMBOL *sym;
	int hv;

	hv = hash(name);
	sym = HashTable[hv];
	while( sym ) {
		if( symbol_match(name, sym->name) )
			return sym;
		else
			sym = sym->next;
	}

	return NULL;
}

SYMBOL *sym_lookup_pair(char *prefix, char *name)
{
	char fullname[ 1024 ];

	sprintf(fullname, "%s&%s", prefix, name);
	return sym_lookup(fullname);
}

/*----------------------------------------------------------------------
 * Insert the symbol into the symbol table.
 * The new symbol is returned to the caller.
 *
 * This function does not check to see if the symbol already exists.
 *
 */
SYMBOL *sym_insert(char *name, SYM_KIND kind)
{
	int hv;
	SYMBOL *sym;

	hv = hash(name);

	sym = (SYMBOL*)BRE_MALLOC( sizeof(SYMBOL) );
	if( sym == NULL ) {
		Bre_Error(BREMSG_DIE_NOMEM);
		return NULL;
	}

	sym->name = BRE_STRDUP(name);
	sym->kind = kind;

	sym->next = HashTable[hv];
	HashTable[hv] = sym;

	return sym;
}

SYMBOL *sym_insert_pair(char *prefix, char *name, SYM_KIND kind)
{
	char fullname[ 1024 ];

	sprintf(fullname, "%s&%s", prefix, name);
	return sym_insert(fullname, kind);
}

/*----------------------------------------------------------------------
 * Takes the symbol 'sym' out of the symbol table.
 *
 */
void sym_detach(SYMBOL *sym)
{
	int hv;
	SYMBOL *prev, *s;

	hv = hash(sym->name);

	prev = NULL;
	s = HashTable[hv];
	while(s) {
		if( symbol_match(sym->name, s->name) ) {
			if( prev )
				prev->next = s->next;
			else
				HashTable[hv] = s->next;
			return;
		}
		prev = s;
		s = s->next;
	}
	BRE_ASSERT(0);
}

/*----------------------------------------------------------------------
 * Inserts 'sym' back into the symbol table.
 * (Assumes 'sym' was previous detached with sym_detach, also
 * the symbol name 'sym->name' cannot already exist.)
 *
 */
void sym_reattach(SYMBOL *sym)
{
	int hv;
	SYMBOL *scurr;

	hv = hash(sym->name);

	scurr = HashTable[hv];
	while( scurr ) {
		if( symbol_match(sym->name, scurr->name) )
			BRE_ASSERT(0);
		scurr = scurr->next;
	}

	sym->next = HashTable[hv];
	HashTable[hv] = sym;
}

/*----------------------------------------------------------------------
 * Remove the symbol 'sym' from the symbol table.
 * The caller will have to make sure to free all
 * the extra stuff associated with a symbol
 *
 * This call will free the symbol's name and the
 * SYMBOL pointer itself.
 *
 */
void sym_remove(SYMBOL *sym)
{
	sym_detach(sym);
	BRE_FREE(sym->name);
	BRE_FREE(sym);
}

/*----------------------------------------------------------------------
 * Detach all symbols of kind 'kind' from
 * the internal symbol table.
 *
 */
SYMBOL_LIST *sym_detach_kind(SYM_KIND kind)
{
	int i;
	SYMBOL *curr, *nxt;
	SYMBOL_LIST *head;

	head = NULL;
	for(i=0; i<SYM_HASH_SIZE; i++) {
		if( HashTable[i] == NULL )
			continue;

		for(curr = HashTable[i]; curr; curr=nxt) {
			nxt = curr->next;
			if( curr->kind == kind ) {
				sym_detach(curr);
				symlist_add(&head, curr);
			}
		}
	}

	return head;
}

SYMBOL *sym_first(SYMBOL_CURSOR *sc)
{
	int i;

	for(i=0; i<SYM_HASH_SIZE; i++) {
		if( HashTable[i] != NULL ) {
			sc->curr_hv = i;
			sc->curr_sym = HashTable[i];
			return sc->curr_sym;
		}
	}
	return NULL;
}

SYMBOL *sym_next(SYMBOL_CURSOR *sc)
{
	int i;

	if( sc->curr_sym->next ) {
		sc->curr_sym = sc->curr_sym->next;
		return sc->curr_sym;
	}

	for(i=sc->curr_hv+1; i<SYM_HASH_SIZE; i++) {
		if( HashTable[i] == NULL )
			continue;

		sc->curr_hv = i;
		sc->curr_sym = HashTable[i];
		return sc->curr_sym;
	}

	return NULL;
}

/*----------------------------------------------------------------------
 * Given a string of the form 'foo&bar'
 * fill the buffers 'prefix' and 'name' with 'foo' and 'bar'
 * repsectively.
 */
void sym_extract_pair(char *pair_name, char *prefix, char *name)
{
	char *p, *q, *sep;

	sep = strchr(pair_name, '&');
	BRE_ASSERT( sep != NULL );

	p = pair_name;
	q = prefix;
	while( p != sep )
		*q++ = *p++;
	*q = '\0';

	strcpy(name, sep+1);
}

/*----------------------------------------------------------------------
 * Insert a new symbol to the end of the symbol list.
 *
 */
void symlist_add(SYMBOL_LIST **symlist, SYMBOL *sym)
{
	SYMBOL_LIST *p, *curr;

	p = (SYMBOL_LIST*)BRE_MALLOC( sizeof(SYMBOL_LIST) );
	if( p == NULL )
		Bre_Error(BREMSG_DIE_NOMEM);

	p->next	= NULL;
	p->sym	= sym;

	if( *symlist == NULL ) {
		*symlist = p;
	} else {
		curr=*symlist;
		while(curr->next)
			curr=curr->next;

		curr->next = p;
	}
}

/*----------------------------------------------------------------------
 * This function detaches all the symbols from the
 * symbol table, but leaves the symbol list intact.
 *
 */
void symlist_detach_all(SYMBOL_LIST *symlist)
{
	SYMBOL_LIST *curr;

	for(curr=symlist; curr; curr=curr->next)
		sym_detach(curr->sym);
}

/*----------------------------------------------------------------------
 * This function detaches all the symbols from the symbol table,
 * as well as free'ing the symbols themselves.
 *
 */
void symlist_remove_all(SYMBOL_LIST **symlist)
{
	SYMBOL_LIST *curr, *nxt;

	for(curr=*symlist; curr; curr=nxt) {
		nxt = curr->next;
		sym_remove(curr->sym);
		BRE_FREE(curr);
	}
	*symlist = NULL;
}

/*----------------------------------------------------------------------
 * Scan the list of external objects. (in BRE()->externals)
 * search for a matching name.
 *
 * RETURNS:
 *	NULL - no such obeject was found.
 *	Else, the BRE_EXTERNAL structure is returned.
 */
BRE_EXTERNAL *sym_find_external(char *id)
{
	BRE_EXTERNAL *curr;

	for(curr=BRE()->externals; curr; curr=curr->next) {
		if( symbol_match(curr->name, id) ) {
			return curr;
		}
	}

	return NULL;
}

/*----------------------------------------------------------------------
 * Calculate some symbol table statistics.
 *	nsymbols	- How many symbols are loaded into the symbol table.
 *	nslots		- How big is the hash table.
 *	nchains		- How many slots contain chains of symbols.
 *	min_chain	- Smallest size of a chain.
 *	max_chain	- Largest chain size.
 *
 */
void sym_stats(int *nsymbols, int *nslots, int *nchains, int *min_chain, int *max_chain)
{
	int i, chain_size;
	int first;
	SYMBOL *curr;

	*nsymbols	= 0;
	*nslots		= SYM_HASH_SIZE;
	*nchains	= 0;
	*min_chain	= 0;
	*max_chain	= 0;

	first = 1;

	for(i=0; i<SYM_HASH_SIZE; i++) {
		if( HashTable[i] ) {
			*nchains += 1;
			chain_size = 0;
			for(curr=HashTable[i]; curr; curr=curr->next)
				chain_size += 1;

			*nsymbols += chain_size;

			if( first || chain_size < *min_chain )
				*min_chain = chain_size;

			if( first || chain_size > *max_chain )
				*max_chain = chain_size;

			first = 0;
		}
	}
}
