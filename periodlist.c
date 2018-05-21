#include "periodlist.h"
#include <malloc.h>

int PeriodListAdd(TPeriodList *List, PERIOD_ITEM_TYPE value) { //dynamicly adds items of type PERIOD_ITEM_TYPE to list
    PERIOD_ITEM_TYPE *newPtr;
	if ((newPtr = (PERIOD_ITEM_TYPE *)realloc(List->pointer, sizeof value * (List->itemCount + 1))) == NULL)
		return 1;
	List->pointer = newPtr;
	newPtr[List->itemCount++] = value;
	//printf("%d\n", newPtr[List->itemCount - 1].period);
	return 0;
}

void PeriodListSort(TPeriodList *List, int (*comparing_function)(PERIOD_ITEM_TYPE, PERIOD_ITEM_TYPE)) { //sorts the list using the comparing_function
    PERIOD_ITEM_TYPE min;
    int indmin;
    for (int i = 0; i < List->itemCount - 1; i++) {
        min = List->pointer[i];
        indmin = i;
        for (int j = i + 1; j < List->itemCount; j++){
            if (comparing_function(List->pointer[j], min)){
                min = List->pointer[j];
                indmin = j;
            }
        }
        List->pointer[indmin] = List->pointer[i];
        List->pointer[i] = min;
    }
}

int PeriodInList(TPeriodList *List, PERIOD_ITEM_TYPE item, int (*comparing_function)(PERIOD_ITEM_TYPE, PERIOD_ITEM_TYPE)){//checks if item is in list
    for (int i = 0; i < List->itemCount; i++){
        if (comparing_function(List->pointer[i], item))
            return i; 
    }
    return -1; //returns -1 if item isn't int the list
}

int PeriodCompareSort (PERIOD_ITEM_TYPE a, PERIOD_ITEM_TYPE b){
    return a.period < b.period;
}

int PeriodCompareInList (PERIOD_ITEM_TYPE a, PERIOD_ITEM_TYPE b){
    return a.period == b.period;
}

TPeriodList PeriodListInit(TPeriodList *list) {
    list->pointer = NULL;
    list->itemCount = 0;
    return *list;
}