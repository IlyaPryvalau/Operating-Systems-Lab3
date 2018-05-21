#ifndef INOLIST_H
#define INOLIST_H
#include <sys/types.h>

typedef struct byteIno { //structure that contains the byte Ino and the count of times that this Ino was met in the file
    int ino; 
    unsigned long long count;
} T_Ino;

#define INO_ITEM_TYPE ino_t //define the type of list item to be used in list

typedef struct listIno {
    INO_ITEM_TYPE *pointer;
    int itemCount;
} TInoList;

int InoListAdd(TInoList *List, INO_ITEM_TYPE value);
void InoListSort(TInoList *List, int (*comparing_function)(INO_ITEM_TYPE, INO_ITEM_TYPE));
int InoInList(TInoList *List, INO_ITEM_TYPE item, int (*comparing_function)(INO_ITEM_TYPE, INO_ITEM_TYPE));
int InoCompareSort (INO_ITEM_TYPE a, INO_ITEM_TYPE b);
int InoCompareInList (INO_ITEM_TYPE a, INO_ITEM_TYPE b);
TInoList InoListInit(TInoList *list);

#endif