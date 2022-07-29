/*** $Id$ ***/

/*
 */
#ifndef lint
static char rcsid[] = "@(#)$Id$";
#endif

/***********************************************************************
 * BRE Bit set routine.
 */

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <string.h>
#include <assert.h>

#include "bre_api.h"
#include "bre_prv.h"

#define NBITS			(sizeof(unsigned long)*8)
#define NLONGS(max)		( (max+1)/NBITS + (((max+1)%NBITS == 0) ? 0 : 1) )
#define INDEX(v)		((v)/NBITS)
#define POSITIVE_MASK(v)	( (unsigned long) (1<<((v)%NBITS)) )
#define NEGATIVE_MASK(v)	(~(POSITIVE_MASK(v)))

/*----------------------------------------------------------------------
 * Allocate a set. 'max_size' is the maximum
 * value to be stored in the set.
 *
 * The returned BITSET will be empty.
 *
 */
BITSET *bitset_new(int max_value)
{
	BITSET *bs;

	bs = (BITSET *)BRE_MALLOC( sizeof(BITSET) );
	if( bs == NULL )
		Bre_Error(BREMSG_DIE_NOMEM);

	bs->max_value = max_value;

	bs->set = (unsigned long *)calloc( NLONGS(max_value), sizeof(unsigned long) );

	if( bs->set == NULL )
		Bre_Error(BREMSG_DIE_NOMEM);

	return bs;
}

/*----------------------------------------------------------------------
 * Free the memory used by a the bre_set 'bs'.
 *
 */
void bitset_free(BITSET *bs)
{
	BRE_FREE(bs->set);
	BRE_FREE(bs);
}

/*----------------------------------------------------------------------
 * Return the state of the bit 'value'.
 *
 */
int bitset_check(BITSET *bs, int value)
{
	unsigned long result;

	BRE_ASSERT(value <= bs->max_value);

	result = bs->set[ INDEX(value) ] & POSITIVE_MASK(value);

	return result != 0;
}

/*----------------------------------------------------------------------
 * Set the bit whose value is 'value'.
 *
 */
void bitset_set(BITSET *bs, int value)
{
	BRE_ASSERT(value <= bs->max_value);

	bs->set[ INDEX(value) ] |= POSITIVE_MASK(value);
}

/*----------------------------------------------------------------------
 * Clear the bit whose value is 'value'.
 *
 */
void bitset_clear(BITSET *bs, int value)
{
	BRE_ASSERT(value <= bs->max_value);

	bs->set[ INDEX(value) ] &= NEGATIVE_MASK(value);
}

/*----------------------------------------------------------------------
 * Calculate the intersection of 'bs1' and 'bs2'
 * Place the result in 'bs1'.
 *
 * This is a bitwise AND operation.
 *
 */
void bitset_union(BITSET *bs1, BITSET *bs2)
{
	unsigned long *p, *q;
	int i, max_bytes;

	BRE_ASSERT(bs1->max_value == bs2->max_value);

	p = bs1->set;
	q = bs2->set;

	max_bytes = NLONGS(bs1->max_value);

	for(i=0; i<max_bytes; i++) {
		p[i] = p[i] & q[i];
	}
}

/*----------------------------------------------------------------------
 * Calculate the union of 'bs1' and 'bs2'
 * Place the result in 'bs1'.
 *
 * This is a bitwise AND operation.
 *
 */
void bitset_intersection(BITSET *bs1, BITSET *bs2)
{
	unsigned long *p, *q;
	int i, max_bytes;

	BRE_ASSERT(bs1->max_value == bs2->max_value);

	p = bs1->set;
	q = bs2->set;

	max_bytes = NLONGS(bs1->max_value);

	for(i=0; i<max_bytes; i++) {
		p[i] = p[i] & q[i];
	}
}

/*----------------------------------------------------------------------
 * Get the first number in the set.
 * RETURNS:
 *	0 - no values found.
 *	1 - got a value
 *
 */
static int last_value;

int bitset_first(BITSET *bs, int *value)
{
	int i, result;
	int idx;

	for(i=0; i <= bs->max_value; i++) {
		idx = INDEX(i);
		if( bs->set[idx] == 0 )
			continue;

		result = bitset_check(bs, i);
		if( result ) {
			*value = i;
			last_value = i;
			return 1;
		}
	}

	return 0;
}

/*----------------------------------------------------------------------
 * Get the next number in the set.
 *
 * RETURNS:
 *	0 - no next value found.
 *	1 - got a value
 */
int bitset_next(BITSET *bs, int *value)
{
	int i, result;
	int idx;

	for(i=last_value+1; i <= bs->max_value; i++) {
		idx = INDEX(i);
		if( bs->set[idx] == 0 )
			continue;

		result = bitset_check(bs, i);
		if( result ) {
			*value = i;
			last_value = i;
			return 1;
		}
	}

	return 0;
}

