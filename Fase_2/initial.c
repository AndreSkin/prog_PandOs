#include "umps3/umps/libumps.h"
#include "../resources/pandos_const.h"
#include "../resources/pandos_types.h"
#include "../Fase_1/pcb.h"
#include "../Fase_1/asl.h"
#include "scheduler.h"
#include "exceptions.h"



/*Dichiarazione delle funzioni extern*/
/*Funzione di test*/
extern void test();
/*Funzione per il TLB-Refill*/
extern void uTLB_RefillHandler();
/*Exception handler*/
extern void exceptionHandler();


/*Inizializzazione del kernel:*/
/*Dichiarazione delle variabili globali*/

/* Numero dei processi iniziati ma non ancora terminati*/
int ProcCount;
/*Numero dei processi iniziati e non ancora terminati,
bloccati a causa di un I/O o di una timer request*/
int SoftBlocked;
/*Puntatore ad una coda di PCB in stato ready*/
pcb_PTR R_Queue;
/*Puntatore al PCB in stato running*/
pcb_PTR CurrProc;
/*Array di interi rappresentanti i semafori per i sub-devices di sistema*/
int sem[MAX_SUB_DEV]; //[49]

/*Dichiarazione del Pass Up Vector*/
HIDDEN passupvector_t* PUV;

/*Stato iniziale del processore*/
HIDDEN state_t initState;

int main()
{

    /*Nucleus initialization (par. 3.1 student guide)*/

    PUV = (passupvector_t*) PASSUPVECTOR;

    /*Riempimento del pass up vector*/
    PUV->tlb_refill_handler = (memaddr) uTLB_RefillHandler;
    PUV->tlb_refill_stackPtr = (memaddr) KERNELSTACK; //(0x2000.1000)
    PUV->exception_stackPtr = (memaddr) KERNELSTACK;
    PUV->exception_handler = (memaddr) exceptionHandler;

    /*Inizializzazione dei moduli di fase 1:*/
    /*Inizializzazione liste dei semafori e dei PCB*/
    initPcbs();
    initASL();
    /*Inizializzazione della ready queue come lista vuota di PCB*/
    R_Queue = mkEmptyProcQ();

    /*Inizialmente non ci sono processi*/
    SoftBlocked =0;
    ProcCount=0;
    CurrProc = NULL;

    /*Inizializzazione dei semafori*/
    for(int i = 0; i < MAX_SUB_DEV; i++)
    {
        sem[i] = 0;
    }

    /*Carico interval timer con 100000 us (= 100 ms)*/
    LDIT(PSECOND);

    /*Istanzio il primo processo*/
    pcb_PTR FirstProc = allocPcb();
    /*Assegno NULL a tutti i "process tree fields"*/
    FirstProc->p_next = NULL;
    FirstProc->p_prev = NULL;
    FirstProc->p_prnt = NULL;
    FirstProc->p_child = NULL;
    FirstProc->p_next_sib = NULL;
    FirstProc->p_prev_sib = NULL;

    /*Imposto p_time a 0 e p_semAdd e p_supportStruct a NULL*/
    FirstProc->p_time = 0;
    FirstProc->p_semAdd = NULL;
    FirstProc->p_supportStruct = NULL;

    /*Incremento il numero di processi attivi*/
    ProcCount = ProcCount + 1;

    STST(&initState);

    /*Imposto a RAMTOP lo SP*/
    RAMTOP(initState.reg_sp);

    /*Interrupt mask e Kernel mode attivati + PLT (p. 25 pops)*/
    initState.status = IEPON | IMON | TEBITON;
    /*
    TE               Int. mask      KUp  IEp  KUc IEc
    1  000 0000 0000 1111 1111 0000  0    1    0   0
    
    IEc = 0 -> Global interrupts disattivati
    KUc = 0 -> Kernel mode
    KUp = 0 e IEp = 1 -> setto i previous bits perchè la modifica abbia effetto dopo la prima LDST
    Int. mask = 1 ... 1 -> il processo ha gli interrupts abilitati
    TE = 1 -> PLT attivo
    */

    /*Il registro t9 per convenzione contiene l'indirizzo iniziale di una procedura
    e deve essere uguale al Program Counter*/
    initState.pc_epc = (memaddr) test;
    initState.reg_t9 = initState.pc_epc;

    FirstProc->p_s = initState;

    /*Inserisco il primo processo nella Ready Queue*/
    insertProcQ(&(R_Queue), FirstProc);

    /*Chiamo lo Scheduler*/
    scheduler();

    return 0;
}
