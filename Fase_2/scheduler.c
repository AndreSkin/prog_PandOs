#include "scheduler.h"
#include "../resources/pandos_types.h"
#include "../Fase_1/pcb.h"
#include <umps3/umps/libumps.h>

/*"Importo" ReadyQueue, CurrentProcess e i contatori dei processi*/
extern pcb_PTR R_Queue;
extern pcb_PTR CurrProc;
extern int SoftBlocked;
extern int ProcCount;

/*Tempo in cui un processo inizia*/
cpu_t initTime;
/*Par. 3.2 student guide*/
void scheduler()
{
  /*Se ho un processo corrente il suo tempo di esecuzione sarà il TOD - il tempo di inizio*/
  if (CurrProc != NULL)
  {
    CurrProc->p_time += (NOW - initTime);
  }

  /*
    Il nuovo processo corrente sarà l'elemento più vecchio della ReadyQueue
    Rimuovo dunque il pcb dalla ready queue e lo inserisco in current process
  */
  CurrProc = removeProcQ(&R_Queue);

  /*Se il processo corrente non è NULL (aka la ReadyQueue non era vuota)*/
  if(CurrProc != NULL)
  {
    /*Imposto il nuovo tempo iniziale al TOD attuale*/
    STCK(initTime);
    /*Imposto una timeslice di 5ms per il PLT*/
    setTIMER(TIMECONV(TIMESLICE)); //TIMESLICE = 5000 (microsecondi)
    /*Carico lo stato del nuovo processo*/
    LDST(&(CurrProc->p_s));
  }
  /*La ReadyQueue è vuota*/
  else
  {
    /*Se non ci sono più processi fermo il sistema*/
    if (ProcCount == 0)
    {
      HALT();
    }
    /*Se ci sono processi che devono terminare*/
    if(ProcCount > 0 && SoftBlocked > 0)
    {
      /*Imposto ad un valore molto alto il timer degli interrupt*/
      setTIMER(TIMECONV(MAXINT));
      /*Attivo gli interrupts*/
      setSTATUS(IECON | IMON);
      /*
        Idle finchè non avviene un evento esterno (par. 7.2.2 pops)
        che però NON dovrebbe essere la fine del PLT
      */
      WAIT(); 
    }
	  /*Se mi trovo in deadlock vado in panic*/
	  else if(ProcCount > 0 && SoftBlocked == 0)
	  {
		PANIC();
	  }
  }
}
