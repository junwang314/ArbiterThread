#include <stdio.h>
#include "label.h"

void main()
{
	/*
	cat_t ar = 0b00000001;
	cat_t aw = 0b10000010;
        cat_t dr = 0b01111111;
        cat_t dw = 0b10001111;
        */
        cat_t ar = create_category(CAT_S);
        cat_t aw = create_category(CAT_I);       
        cat_t dr = create_category(CAT_S);
        cat_t dw = create_category(CAT_I); 
        label_t La = {ar, aw};
        label_t Lb = {ar, aw};
        label_t Lc = {ar};
        label_t Ld = {dw};
        label_t L1 = {ar, aw};
        label_t L2 = {ar};
        own_t Oa = {ar, aw};
        own_t Ob = {};
        own_t Oc = {};
        own_t Od = {dr, dw};

	printf("a->b: %d\n", check_label(La, Oa, Lb, Ob));
	printf("a->c: %d\n", check_label(La, Oa, Lc, Oc));
	printf("a->d: %d\n", check_label(La, Oa, Ld, Od));
	printf("b->a: %d\n", check_label(Lb, Ob, La, Oa));	
	printf("a->1: %d\n", check_label(La, Oa, L1, 0));
	printf("a->2: %d\n", check_label(La, Oa, L2, NULL));

	printf("1-a: %d\n", check_mem_prot(La, Oa, L1));
	printf("2-a: %d\n", check_mem_prot(La, Oa, L2));
	printf("1-b: %d\n", check_mem_prot(Lb, Ob, L1));
	printf("2-b: %d\n", check_mem_prot(Lb, Ob, L2));
	printf("1-c: %d\n", check_mem_prot(Lc, Oc, L1));
	printf("2-c: %d\n", check_mem_prot(Lc, Oc, L2));
	printf("1-d: %d\n", check_mem_prot(Ld, Od, L1));
	printf("2-d: %d\n", check_mem_prot(Ld, Od, L2));

}
