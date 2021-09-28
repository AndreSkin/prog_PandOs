#include "../resources/pandos_types.h"
#include "../resources/pandos_const.h"
#include "mcpy.h"
#include "../Fase_1/asl.h"
#include "../Fase_1/pcb.h"
#include "scheduler.h"
#include "interrupts.h"

extern pcb_PTR R_Queue;
extern pcb_PTR CurrProc;
extern int SoftBlocked;
extern int sem[MAX_SUB_DEV];
extern cpu_t initTime;

/*TOD quando viene lanciato un interrupt*/
cpu_t Int_initTime;

void getInterruptLine(unsigned int cause_reg)
{
    STCK(Int_initTime);
   
    /*0xFF00 = 1111 1111 0000 0000, ovvero isolo gli 8 bit di IP*/
    cause_reg = cause_reg & 0xFF00;
    cause_reg >>= 8;
    unsigned int IP_reg = cause_reg;

    int toCheck = 2;

    /*Se il primo bit è 1, panic perchè questi interrupts non sono da gestire*/
    if(IP_reg & 1)
        PANIC();

    /*Controllo bit per bit*/
    for (int i = 1; i < 8; i++)
    {
        if(IP_reg & toCheck)
            General_Interrupt(i);
        toCheck *= 2;
    }
}

void General_Interrupt(int line)
{
  /*
    Pops pagina 30: 0x1000.0040 = INT_DEV_BM

    0x1000.0040 + 0x10 Interrupt Line 7 Interrupting Devices Bit Map
    0x1000.0040 + 0x0C Interrupt Line 6 Interrupting Devices Bit Map
    0x1000.0040 + 0x08 Interrupt Line 5 Interrupting Devices Bit Map
    0x1000.0040 + 0x04 Interrupt Line 4 Interrupting Devices Bit Map
    0x1000.0040        Interrupt Line 3 Interrupting Devices Bit Map
  */

  /*Non-timer dev interrupt*/
  if (line > 2)
  {
    /*Grazie a questa formula ottengo l'indirizzo della linea*/
    memaddr* dev = (memaddr*) (INT_DEV_BM + ((line - 3) * 0x4));

    int toCheck = 1;
    for (int i = 0; i < DEVPERINT; i++) //Ciclo 8 volte (devices per interrupt line)
    {
      /*Cerco il device corretto*/
      if (*dev & toCheck)
      {
        Non_T_Int(line, i);
      }
      toCheck *= 2;
    }
  }
  /*Timer interrupt*/
  else if (line == 1)
  {
    setTIMER(TIMECONV(MAXINT));
    /*Rendo lo stato di currentProcess lo stato attuale*/
    CurrProc->p_s = *((state_t*) BIOSDATAPAGE);
    /*Calcolo il tempo di currentProcess*/
    CurrProc->p_time += (NOW - initTime);
    /*Inserisco currentProcess nella readyQueue e chiamo lo Scheduler*/
    insertProcQ(&R_Queue, CurrProc);
    scheduler();
  }
  /* Interval timer interrupt*/
  else
  {
    /*Carico l'interval timer*/
    LDIT(PSECOND);

    /*Finchè ci sono PCB associati al semaforo*/
    while (headBlocked(&sem[MAX_SUB_DEV - 1]) != NULL)
    {
      /*Sblocca un processo*/
      pcb_t* sbloc = removeBlocked(&sem[MAX_SUB_DEV - 1]);

      if (sbloc != NULL)
      {
          sbloc->p_semAdd = NULL;
          /*Calcolo il tempo di 'sbloc'*/
          sbloc->p_time += NOW - Int_initTime;
          /*Inserisco 'sbloc' nella readyQueue*/
          insertProcQ(&R_Queue, sbloc);
          /*Diminuisco il contatore dei processi SoftBlocked*/
          SoftBlocked = SoftBlocked - 1;
      }
    }
    /*Ora questo semaforo non ha più processi associati*/
    sem[MAX_SUB_DEV - 1] = 0;

    /*Se in questo momento non c'è nessun processo corrente chiamo lo Scheduler*/
    if (CurrProc == NULL)
    {
      scheduler();
    }
    /*Altrimenti carico il vecchio stato*/
    else
    {
      LDST((state_t*) BIOSDATAPAGE);
    }
  }
}

void Non_T_Int(int line, int dev)
{
  /*Salvataggio del device register secondo la formula alla fine di pagina 28 pops*/
  devreg_t * devR = (devreg_t*) (BEGIN_DEVREG + ((line - 3) * 0x80) + (dev * 0x10));
  /*Questo valore indica se il terminale riceve o trasmette un interrupt*/
  int Term_RT = 0;
  /*Stato da ritornare a v0*/
  unsigned int status_toReturn;

  if (2 < line && line < 7)
  {
    /*Invio un ACK*/
    devR->dtp.command = ACK;
    /*Salvo lo status da ritornare*/
    status_toReturn = devR->dtp.status;
  }
  /*Terminali*/
  else if (line == 7)
  {
      termreg_t* termreg = (termreg_t*) devR;

      /*Se non è pronto a ricevere*/
      if (termreg->recv_status != READY)
      {
        /*Salvo lo status da ritornare*/
        status_toReturn = termreg->recv_status;
        termreg->recv_command = ACK;
        /*Il terminale può ricevere interrupts*/
        Term_RT = 1;
      }
      else
      {
          /*Salvo lo status da ritornare*/
          status_toReturn = termreg->transm_status;
          termreg->transm_command = ACK;
      }
      /*Trovo il numero del device*/
      dev = 2*dev + Term_RT;
  }

  /*Trovo il numero del semaforo del device*/
  int semNum = (line - 3) * 8 + dev;

  /*Operazione V sul semaforo del device*/
  sem[semNum] += 1;
  /*Sblocco il processo sul semaforo*/
  pcb_t* unlocked = removeBlocked(&sem[semNum]);
  /*Se c'era almeno un processo bloccato*/
  if (unlocked != NULL)
  {
    /*Inserisco lo stato da ritornare nel registro v0*/
    unlocked->p_s.reg_v0 = status_toReturn;
    /*Il processo non è più bloccato*/
    unlocked->p_semAdd = NULL;
    /*Calcolo il tempo del processo*/
    unlocked->p_time += NOW - Int_initTime;
    /*Diminuisco il numero di processi SoftBlocked*/
    SoftBlocked -= 1;
    /*Inserisco il processo sbloccato nella readyQueue*/
    insertProcQ(&R_Queue, unlocked);
  }
  /*Se nessun processo era in esecuzione chiamo lo Scheduler*/
  if (CurrProc == NULL)
  {
    scheduler();
  }
  /*Altrimenti carico il vecchio stato*/
  else
  {
    LDST((state_t *) BIOSDATAPAGE);
  }
}

