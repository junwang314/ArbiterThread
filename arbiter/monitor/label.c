#include <stdlib.h> /* rand(), srand() */

#include <ab_debug.h>
#include "label.h"

static inline cat_t get_cat_type(cat_t cat)
{
	return (cat & 0b10000000);
}

#define SET_FLAG(flag)		(flag = 1)	//used in check_label()
#define CLEAR_FLAG(flag)	(flag = 0)

cat_t create_cat(cat_type t)
{
	static cat_t cat_gen = 0;
	if (++cat_gen >= 0b10000000) {
		AB_MSG("ERROR: category used up\n");	
		return -1;
	}
	return (cat_t) (t | (cat_gen & 0b01111111));
}

//return 0 if ownership runs out of space
cat_t add_onwership(own_t O, cat_t cat)
{
	int i;
	
	for (i = 0; i < 8; i++) {
		// find a blank space and put cat into it
		if ((O[i] != 0))
			continue;
		else {
			O[i] = cat;
			return cat;
		}
	}
	return 0;  //no blank space available for the new cat 
}

int check_label(label_t L1, own_t O1, label_t L2, own_t O2)
{
	int i, j, flag;
	
	// check RULE 2-1, 3: every secrecy category in 1 must present in 2
	for (i = 0; i < 8; i++) {
		CLEAR_FLAG(flag);
		
		// pick a secrecy category in 1's label
		if ((L1[i] == 0) || (get_cat_type(L1[i]) == CAT_I))
			continue;
		// if it is in 1's ownership, bypass the check
		for (j = 0; j < 8; j++) {
			if (L1[i] == O1[j]) {
				SET_FLAG(flag);
				break;
			}
		}
		if (flag) {
			continue;
		}
		// if it is in 2's label, check pass to the next loop!
		for (j = 0; j < 8; j++) {
			if (L1[i] == L2[j])
				break;
			else if (j == 7)
				return -1;
		}
	}
	
	// check RULE 2-1, 3: every integrity category in 2 must present in 1
	for (i = 0; i < 8; i++) {
		CLEAR_FLAG(flag);
		
		// pick an integrity category in 2's label
		if ((L2[i] == 0) || (get_cat_type(L2[i]) == CAT_S))
			continue;
		// if it is in 1's ownership, bypass the check
		for (j = 0; j < 8; j++) {
			if (L2[i] == O1[j]) {
				SET_FLAG(flag);
				break;
			}
		}
		if (flag) {
			continue;
		}
		// if it is in 1's label, check pass to the next loop!
		for (j = 0; j < 8; j++) {
			if (L2[i] == L1[j])
				break;
			else if (j == 7)
				return -1;
		}
	}
	
	// check RULE #2-2: 2's ownership must be a subset of 1's
	if (O2 != NULL) { // no need to check ownership if creating data object
		for (i = 0; i < 8; i++) {
			// pick a category in 2's ownership
			if ((O2[i] == 0))
				continue;
			// if it is in 1's ownership, check pass to the next loop!
			for (j = 0; j < 8; j++) {
				if (O2[i] == O1[j])
					break;
				else if (j == 7)
					return -1;
			}
		}
	}
	
	// pass the entire label check!
	return 0;
}

enum mem_prot check_mem_prot(label_t L1, own_t O1, label_t L2)
{
	enum mem_prot prot;
	int pw, pr;

	// check information flow from 1 to 2, i.e., write
	pw = check_label(L1, O1, L2, NULL);
	
	// check information flow from 2 to 1, i.e., read
	pr = check_label(L2, O1, L1, NULL);
	
	if (pr == 0 && pw == 0)
		return PROT_RW;
	else if (pr == 0 && pw == -1)
		return PROT_R;
	else
		return PROT_N;
}
