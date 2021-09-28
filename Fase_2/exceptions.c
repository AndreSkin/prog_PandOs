#include "../resources/pandos_const.h"
#include "../resources/pandos_types.h"
#include "mcpy.h"
#include "../Fase_1/pcb.h"
#include "../Fase_1/asl.h"
#include "interrupts.h"
#include "scheduler.h"
#include <umps3/umps/libumps.h>
#include <umps3/umps/cp0.h>
#include <umps3/umps/arch.h>
#include "exceptions.h"


extern pcb_PTR CurrProc;
extern pcb_PTR R_Queue;
HIDDEN state_t* oldSt;
extern cpu_t initTime;

extern int ProcCount;
extern int SoftBlocked;
extern int sem[MAX_SUB_DEV];

/*Par. 3.4 student guide*/
void exceptionHandler()
{
    /*Salvo lo stato prima di un interrupt*/
    state_t* excState = (state_t*) BIOSDATAPAGE;
    /*Ottengo la causa dell'eccezione*/
    unsigned int excCode =  excState->cause & 0x3c;
    excCode >>= CAUSESHIFT;
    /*
    0x3c = 11 1100, che significa che prendendo tutto il registro Cause e facendo
    un'operazione di AND, posso "isolare" i bit 2 - 6, che corrispondono a Exccode.
    Inoltre, con un uno shift di 2 posizioni ottengo soltanto i bit 2-6.
    (Cause register al par. 3.3 pops)
    */

    /*Device Interrupt*/
    if (excCode == 0) //0
    {
    /*Ottengo la linea di interrupt*/
        getInterruptLine(excState->cause);
    }
    /*TLB refill exception*/
    else if ((excCode >= 1) && (excCode <= 3)) //1, 2, 3
    {
        PassUporDie(PGFAULTEXCEPT, excState);
    }
    /*Syscall exception*/
    else if (excCode == 8) //8
    {
        SysHandler(excState);
    }
    /*Trap*/
    else //4, 5, 6, 7, 9 - 12
    {
        PassUporDie(GENERALEXCEPT, excState);
    }
}
/*Par. 3.7 student guide*/
void PassUporDie(int tipo_eccez , state_t* stato_eccez )
{
	/*Se la support struct è NULL, termino il processo*/
    if(CurrProc->p_supportStruct == NULL)
    {
        SYS2_TP();
    }
    else
    {
    	/*Copio lo stato nel campo corretto di current process e chiamo LDCXT*/
        (CurrProc->p_supportStruct)->sup_exceptState[tipo_eccez] = *stato_eccez;
        context_t CXT_info = (CurrProc->p_supportStruct)->sup_exceptContext[tipo_eccez];
        LDCXT(CXT_info.stackPtr, CXT_info.status, CXT_info.pc);
    }
}

/* Par. 3.5 student guide*/
void SysHandler(state_t* exception_state)
{
    /*Salvo lo stato*/
    oldSt = exception_state;
    /*Salvo il code della system call, che si trova in a0*/
    unsigned int sysCallCode = (unsigned int) exception_state->reg_a0;
    
    /*Incremento PC*/
    oldSt->pc_epc += WORDLEN;
    
    /*Se il quarto bit dello stato (KUp) non è 0, siamo in user mode*/
    if((exception_state->status & USERPON) != ALLOFF)
    {
        /* Se la syscall è chiamata in user mode*/
        /*Prendo il cause register e pongo a 0 Exc.code*/
        unsigned int oldCause= getCAUSE() & !CAUSE_EXCCODE_MASK; //0x0000007c
       /*Inserisco la reserved instruction in Exc.code e lancio l'eccezione*/
        setCAUSE(oldCause | EXC_RI<< CAUSESHIFT);
        PassUporDie(GENERALEXCEPT, exception_state);
    }

    switch (sysCallCode)
    {
        case CREATEPROCESS: //1
            SYS1_CP();
            break;
        case TERMPROCESS: //2
            SYS2_TP();
            break;
        case PASSEREN: //3
            SYS3_P((int*) exception_state->reg_a1);
            break;
        case VERHOGEN: //4
            SYS4_V((int*) exception_state->reg_a1);
            break;
        case IOWAIT: //5
            SYS5_W_IO();
            break;
        case GETTIME: //6
            SYS6_GCT();
            break;
        case CLOCKWAIT: //7
            SYS7_W_CLK();
            break;
        case GETSUPPORTPTR: //8
            SYS8_GSD();
            break;
        default:
            PassUporDie(GENERALEXCEPT,exception_state);
            break;
    }
}

void SYS1_CP()
{
  /*Utilizzo le informazioni dello stato salvato*/
  /*Prendo lo state da a1*/
  state_t ProcState = *((state_t*) oldSt->reg_a1);
  /*Prendo la support struct da a2*/
  support_t *SuppProc = (support_t*) oldSt->reg_a2;

  /*Alloco un nuovo PCB*/
  pcb_PTR newproc = allocPcb();

  if (newproc != NULL)
  {
      /*Lo inserisco nella readyQueue*/
      insertProcQ(&(R_Queue), newproc);
      /*Il nuovo processo sarà figlio del corrente*/
      insertChild(CurrProc, newproc);

      /*Aumento il numero dei processi*/
      ProcCount += 1;
      newproc->p_semAdd = NULL;
      newproc->p_time = 0;
      /*Lo stato del nuovo processo sarà lo stesso di prima*/
      newproc->p_s = ProcState;

      newproc->p_supportStruct = SuppProc;      
      oldSt->reg_v0 = OK; //0

  }
  /*Errore*/
  else
  {
     oldSt->reg_v0 = NOPROC; //-1
  }

  /*Carico lo stato*/
  LDST(oldSt);
}


void SYS2_TP()
{
    outChild(CurrProc);
    killprocT(CurrProc);

    CurrProc = NULL;
    scheduler();
}

void killprocT(pcb_t* toTerm)
{
    if (toTerm == NULL)
    {
      return;
    }

    /*Finchè il PCB ha figli rimuovo il primo*/
    while (!(emptyChild(toTerm)))
    {
      killprocT(removeChild(toTerm));
    }

    /*Rimuovo il processo dalla readyQueue*/
    outProcQ(&R_Queue, toTerm);
    
    /*Tolgo il PCB dal semaforo se è bloccato o softbloccato*/
    if (toTerm->p_semAdd != NULL)
    {
     /*Controllo i semafori dei devices*/
      if (&(sem[0]) <= toTerm->p_semAdd && toTerm->p_semAdd <= &(sem[48]))
      {
        SoftBlocked -= 1;
      }
      else
      {
        *(toTerm->p_semAdd) += 1;
      }
      /*Rimuovo il processo dalla coda del semaforo su cui è bloccato*/
      outBlocked(toTerm);
    }
    /*Libero il PCB e diminuisco il numero dei processi*/
    freePcb(toTerm);
    ProcCount -= 1;
}

void SYS3_P(int* semAddr)
{
    *semAddr -= 1;

    if (*semAddr < 0)
    {
      /*Blocco il processo e chiamo lo scheduler*/
        CurrProc->p_s = *oldSt;
        insertBlocked(semAddr, CurrProc);
        scheduler();
    }
      /*Carico il vecchio stato*/
      LDST(oldSt);
}

void SYS4_V(int* semAddr)
{
  /*Sblocco un processo e lo inserisco nella readyQueue*/
  *semAddr += 1;
  pcb_t* unlocked = removeBlocked(semAddr);

  if (unlocked != NULL)
  {
      unlocked->p_semAdd = NULL;
      insertProcQ(&R_Queue, unlocked);
  }
  /*Carico il vecchio stato*/
  LDST(oldSt);
}

void SYS5_W_IO()
{
	/* Trovo interrupt line e device number */
    int lineNumb = oldSt->reg_a1;
    int devNumb = oldSt->reg_a2;
    int a3_read = oldSt->reg_a3; /*Indica se aspettiamo o meno una lettura da terminale*/

	/* Se è un terminale */
    if (lineNumb == 7)
	{
		devNumb = 2 * devNumb + a3_read;
	}
    int index = (lineNumb - 3) * 8 + devNumb; /* indirizzo del semaforo del device */

    /*Eseguo la P (blocca sempre)*/
    SoftBlocked += 1;
    (sem[index])--;
    /*Blocco il processo e chiamo lo scheduler*/
    CurrProc->p_s = *oldSt;
    insertBlocked(&(sem[index]), CurrProc);
    scheduler();
}

void SYS6_GCT()
{
    CurrProc->p_time += (NOW - initTime); /* ottengo il tempo utilizato dal proceso corrente */
    oldSt->reg_v0 = CurrProc->p_time; /* carico il tempo sullo stato dell'eccezione */
    STCK(initTime); /*InitTime = NOW*/
    LDST(oldSt);
}

void SYS7_W_CLK()
{
    SoftBlocked += 1;
    /*
    Esegue una P sul semaforo dello pseudo-clock 
    sul quale viene eseguita una V ogni 100ms    
    */
    SoftBlocked += 1;
    (sem[MAX_SUB_DEV])--;
    /*Blocco il processo e chiamo lo scheduler*/
    CurrProc->p_s = *oldSt;
    insertBlocked(&sem[MAX_SUB_DEV - 1], CurrProc);
    scheduler(); 
}

void SYS8_GSD()
{ 
    /* Carico le info dal livello di supporto del processo interessato */
    oldSt->reg_v0 = (unsigned int) CurrProc->p_supportStruct; 
   
    LDST(oldSt);
}

