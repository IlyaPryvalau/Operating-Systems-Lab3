#ifndef PERIODLIST_H
#define PERIODLIST_H

typedef struct bytePeriod { //structure that contains the byte period and the count of times that this period was met in the file
    int period; 
    unsigned long long count;
} T_PERIOD;

#define PERIOD_ITEM_TYPE T_PERIOD //define the type of list item to be used in list

typedef struct listPeriod {
    PERIOD_ITEM_TYPE *pointer;
    int itemCount;
} TPeriodList;

int PeriodListAdd(TPeriodList *List, PERIOD_ITEM_TYPE value);
void PeriodListSort(TPeriodList *List, int (*comparing_function)(PERIOD_ITEM_TYPE, PERIOD_ITEM_TYPE));
int PeriodInList(TPeriodList *List, PERIOD_ITEM_TYPE item, int (*comparing_function)(PERIOD_ITEM_TYPE, PERIOD_ITEM_TYPE));
int PeriodCompareSort (PERIOD_ITEM_TYPE a, PERIOD_ITEM_TYPE b);
int PeriodCompareInList (PERIOD_ITEM_TYPE a, PERIOD_ITEM_TYPE b);
TPeriodList PeriodListInit(TPeriodList *list);

#endif