#ifndef VM_SUP
#define VM_SUP

#include "../resources/pandos_const.h"
#include "../resources/pandos_types.h"
#include "umps3/umps/libumps.h"

/*Tlb refill handler*/
void uTLB_RefillHandler();
/*Pager di PandOS*/
void pager();
/*Algoritmo di rimpiazzamento di PandOS*/
int rimpiazzamento();
/*Update del TLB*/
void TLBupdate();
/*Uccide un processo e rilascia la mutex, se presente*/
void SIGKILL(int* semaphore);
/*Legge o scrive (in base a RW) da / su un flash device*/
int RW_device(int RW, int data, int devblock, int devnum);

#endif
