#include "initProc.h"

/*Master semaphore*/
int Master_Sem;
/*Semaforo dei sub devices*/
int Sem[MAX_SUB_DEV];
/*Array di support structs*/
static support_t Support_pool[UPROCMAX + 1];

extern void pager();
extern void GeneralExHandler();

extern swap_t SwapPool[UPROCMAX * 2];
extern int SwapPoolSem;

void InstantiateProc(int proc_id)
{
   /*Stato iniziale del processore per Uproc (Par. 4.9.1 student guide)*/
    state_t Uproc_state;
    /*PC (e s_t9) e SP*/
    Uproc_state.pc_epc = UPROCSTARTADDR;
    Uproc_state.reg_t9 = UPROCSTARTADDR;
    Uproc_state.reg_sp = USERSTACKTOP;

    /*User mode (Userp -> KUc) + interrupts (IM e IEp) + PLT (TE)*/
    Uproc_state.status = USERPON | IMON | IEPON | TEBITON ;
    /*Imposto ASID*/
    Uproc_state.entry_hi = proc_id << ASIDSHIFT;
    /*Il campo asid si trova tra i bit 6 - 11 di Entryhi*/


    /*L'ASID è quello del processo*/
    Support_pool[proc_id].sup_asid = proc_id;

    /*Ottengo l'indirizzo di RAMTOP per trovare la cima dello stack*/
    memaddr top;
    RAMTOP(top);
    memaddr Stack = top - (2 * proc_id * PAGESIZE);

    /*Setto i processor context (PC / SP / Status) per: */
    /*Page fault exception*/
    Support_pool[proc_id].sup_exceptContext[PGFAULTEXCEPT].pc = (memaddr) pager;
    Support_pool[proc_id].sup_exceptContext[PGFAULTEXCEPT].stackPtr = (memaddr) (Stack + PAGESIZE);
    /*Kernel mode e interrupts + PLT*/ 
    Support_pool[proc_id].sup_exceptContext[PGFAULTEXCEPT].status = IMON | IEPON | TEBITON;
   
    /*General exception*/
    Support_pool[proc_id].sup_exceptContext[GENERALEXCEPT].pc = (memaddr) GeneralExHandler;
    Support_pool[proc_id].sup_exceptContext[GENERALEXCEPT].stackPtr = (memaddr) Stack;
    /*Kernel mode e interrupts + PLT*/
    Support_pool[proc_id].sup_exceptContext[GENERALEXCEPT].status = IMON | IEPON | TEBITON;
   

    /*Page table*/
    for (int i = 0; i < MAXPAGES - 1; i++)
    {
        /*Par. 4.2.1 student guide, inizializzo le page table*/
        Support_pool[proc_id].sup_privatePgTbl[i].pte_entryHI = KUSEG + (i << VPNSHIFT) + (proc_id << ASIDSHIFT);
        Support_pool[proc_id].sup_privatePgTbl[i].pte_entryLO = DIRTYON /*| ~ VALIDON*/;
    }
    /*Zona dello stack, entry 31*/
    /*Differenzio gli stack*/
    Support_pool[proc_id].sup_privatePgTbl[MAXPAGES - 1].pte_entryHI = 0xBFFFF000 + (proc_id << ASIDSHIFT);
    Support_pool[proc_id].sup_privatePgTbl[MAXPAGES - 1].pte_entryLO = DIRTYON /*| ~ VALIDON*/;
    
    /*Creazione del processo*/
    int UProc_creation = SYSCALL(CREATEPROCESS, (int) &Uproc_state, (int) &(Support_pool[proc_id]), 0);

    /*Se lo stato finale non è OK termina il processo*/
    if (UProc_creation != OK)
    {
        SYSCALL(TERMPROCESS, 0, 0, 0);
    }
    

}

void test()
{
    /*Inizializza Swap Pool table e semaphore*/    
    /*inizialmente il semaforo della pool e' a 1*/
    SwapPoolSem = 1;
    for (int i = 0; i < UPROCMAX * 2; i++)
    {
        SwapPool[i].sw_asid = -1;
    }

    /*Inizializza il semaforo di ogni sub device*/
    for (int i = 0; i < MAX_SUB_DEV; i++)
    {
        Sem[i] = 1;
    }

    /*Crea gli 8 processi*/	
	for(int id=1; id <= UPROCMAX; id++)
    {
		InstantiateProc(id);
	}
    
    /*Faccio una P sul master semaphore inizializzato a 0*/
	Master_Sem = 0;

	for(int i=0; i < UPROCMAX; i++)
    {
		SYSCALL(PASSEREN, (int) &Master_Sem, 0, 0);
	}
    
    /*Infine, chiamo la SYS2 e termino*/
	SYSCALL(TERMPROCESS, 0, 0, 0);
}
