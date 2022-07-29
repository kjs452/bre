#ifndef _BRE_BIT_H
#define _BRE_BIT_H
/*----------------------------------------------------------------------
 * Header file for bit set routines.
 *
 *
 */
typedef struct {
	int		max_value;
	unsigned long	*set;
} BITSET;

/*
 * Proto-types
 */
extern BITSET	*bitset_new(int);
extern void	bitset_free(BITSET *);
extern int	bitset_check(BITSET *, int);
extern void	bitset_set(BITSET *, int);
extern void	bitset_clear(BITSET *, int);
extern void	bitset_union(BITSET *, BITSET *);
extern void	bitset_intersection(BITSET *, BITSET *);
extern int	bitset_first(BITSET *, int *);
extern int	bitset_next(BITSET *, int *);

#endif
