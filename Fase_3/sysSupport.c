#include "sysSupport.h"
#include "../resources/pandos_const.h"

extern int Sem[MAX_SUB_DEV]; /* = 49*/

void GeneralExHandler()
{
	/*Uso SYS_8 per ottenere un un puntatore alla struttura di supporto del processo corrente*/
	support_t* sup_struct = (support_t*)SYSCALL(GETSUPPORTPTR, 0, 0, 0);

	/*Lo stato del processore al momento di una general exception si trova nell'omonimo campo di sup_exceptState (par. 4.6 student guide)*/
	state_t* exceptState = (state_t*)&(sup_struct->sup_exceptState[GENERALEXCEPT]);

	/*Determina la causa dell'eccezione*/
	int cause = (exceptState->cause & GETEXECCODE) >> CAUSESHIFT;

	if (cause == SYSEXCEPTION) /*SYSEXCEPTION = 8*/
	{
		/*
			Chiamo il syscall handler.
			Si assume che il puntatore alla struttura di supporto sia != NULL
			e che i campi di sup_exceptContext siano correttamente inizializzati.
			Inoltre, il numero della syscall si trova nel registro a0.
			(par. 4.7 student guide)
		*/
		SyscallExHandler(sup_struct, exceptState, exceptState->reg_a0);
	}
	else
	{
		/*Se non è una syscall termino il processo corrente*/
		SIGKILL(NULL);
	}
}

void SyscallExHandler(support_t* sup_struct, state_t* exceptState, unsigned int number_a0)
{
	/*Switch per stabilire quale syscall gestire, se il numero è fuori dal range previsto, il processo viene terminato*/
	switch(number_a0){
        case TERMINATE:
            Terminate();
            break;
        case GET_TOD:
            Get_TOD(exceptState);
            break;
        case WRITEPRINTER:
            Write_To_Printer(sup_struct, exceptState);
            break;
        case WRITETERMINAL:
            Write_To_Terminal(sup_struct, exceptState);
            break;
        case READTERMINAL:
            Read_From_Terminal(sup_struct, exceptState);
            break;
        default:
            SIGKILL(NULL);
            break;
    }
		/*
			Il controllo va restituito al processo chiamante all'istruzione immediatamente successiva la syscall.
			A tal fine il PC deve essere incrementato di 4.
			(par. 4.7 student guide)
		*/
		exceptState->pc_epc += 4;
	    LDST(exceptState);
}

/*NB: ogni syscall che ritorna un valore,lo ritorna nel registro v0 (par. 4.7 student guide)*/

void Terminate()
{
	SIGKILL(NULL);
}

void Get_TOD(state_t* exceptState)
{
	exceptState->reg_v0 = NOW;
	/*STCK(exceptState->reg_v0 );*/
}

void Write_To_Printer(support_t* sup_struct, state_t* exceptState)
{
	/*
		Nel registro a1 si trova l'indirizzo del primo carattere
		Nel registro a2 si trova la lunghezza della stringa
		(par 4.7.3 student guide)
	*/
	char* str = (char*)sup_struct->sup_exceptState[GENERALEXCEPT].reg_a1;
	int str_length = sup_struct->sup_exceptState[GENERALEXCEPT].reg_a2;

	/*
		Verifichiamo che la stringa da stampare non si trovi al di fuori dello spazio degli indirizzi dei processi utente
		e che la stringa non sia lunga 0 o più di 128.
		In caso una di queste condizioni si verifichi, il processo viene terminato (par.4.7.3 student guide)
	*/
	if((int)str < KUSEG || str_length < 0 || str_length > MAXSTRLENG)
	{
        SIGKILL(NULL);
	}
	/*Altrimenti inizia la scrittura*/
    else
	{
		/*
			Ottengo ASID della stampante. 
			NB: sottraiamo 1 perchè gli ASID vanno da 1 a 8 mentre i devices da 0 a 7
		*/
		int printer_asid = sup_struct->sup_asid -1;
		
		/* Indice del semaforo associato alla stampante*/ 
		int printer_sem_index = (PRNTINT - 3) * 8 + printer_asid;  /*PRNTINT = 6*/
		
		/*Salvataggio del device register secondo la formula alla fine di pagina 28 pops*/
		devreg_t * printer_devR = (devreg_t*) (BEGIN_DEVREG + ((PRNTINT - 3) * 0x80) + (printer_asid * 0x10));
		
		/*Entro nella sezione critica*/
		 SYSCALL(PASSEREN, (int) &Sem[printer_sem_index], 0, 0);
		
		int op_result;
		int read = 0;
		
		/*Inizio ad operare sulla stringa*/
		while(read < str_length)
		{
			/*Rendo l'operazione atomica disabilitando gli interrupts mettendo a 0 il bit IEc (pag. 25 pops)*/
			setSTATUS(getSTATUS() & (~IECON));
			
			/*Inserisco i dati*/
			printer_devR->dtp.data0 = *str;
			/*Comando di trasmissione dei dati*/
			printer_devR->dtp.command = TRANSMITCHAR;
			/*Attendo la fine dell'operazione*/
			/*IOWAIT ritorna lo status del device*/
			op_result = SYSCALL(IOWAIT, PRNTINT, printer_asid, 0);
			
			/*Riabilito gli interrupts rimettendo IEc a 1*/
			setSTATUS(getSTATUS() | IECON);
			
			/*Verifico che sia andato tutto a buon fine (ovvero lo stato del dispositivo è READY)*/
			if((op_result & 0x000000FF) == READY) /* = 1*/
			{
				/*Incremento il numero di caratteri letti e avanzo nella lettura della stringa*/
				read++;
				str++;
			}
			/*Altrimenti inserisco in read il negato del device status ed esco dal ciclo*/
			else
			{
				read = - (op_result & 0x000000FF);
				break; 
			}
		}
		
		/*Rilascio la mutex*/
		SYSCALL(VERHOGEN, (int) &Sem[printer_sem_index], 0, 0);
		/*Inserisco in v0 il numero di caratteri letti o lo stato di errore*/
		sup_struct->sup_exceptState[GENERALEXCEPT].reg_v0= read;
	}
	
}


void Write_To_Terminal(support_t* sup_struct, state_t* exceptState)
{
    /*
		Nel registro a1 si trova l'indirizzo del primo carattere
		Nel registro a2 si trova la lunghezza della stringa
		(par 4.7.4 student guide)
	*/
	char* str = (char*)sup_struct->sup_exceptState[GENERALEXCEPT].reg_a1;
	int str_length = sup_struct->sup_exceptState[GENERALEXCEPT].reg_a2;

	/*
	Verifichiamo che la stringa da scrivere non si trovi al di fuori dello spazio degli indirizzi dei processi utente e che la stringa non sia lunga 0 o più di 128.
	In caso una di queste condizioni si verifichi, il processo viene terminato (par.4.7.4 student guide)
	*/
	if((int)str < KUSEG || str_length < 0 || str_length > MAXSTRLENG)
	{
        SIGKILL(NULL);
	}
	
	/*Altrimenti inizia la scrittura*/
    else
	{
		/*Ottengo ASID del terminale.*/
		int Term_asid = sup_struct->sup_asid -1;


		/* Indice del semaforo associato al terminale*/ 
		int Term_sem_index = (TERMINT - 3) * 8 + Term_asid;  /*TERMINT = 7*/


		/*Salvataggio del device register secondo la formula alla fine di pagina 28 pops*/
		devreg_t * Term_devR = (devreg_t*) (BEGIN_DEVREG + ((TERMINT - 3) * 0x80) + (Term_asid * 0x10));


		/*Entro nella sezione critica*/
		 SYSCALL(PASSEREN, (int) &Sem[Term_sem_index], 0, 0);

		int op_result;
		int read = 0;

		/*Inizio ad operare sulla stringa*/
		while(read < str_length)
		{
			/*Rendo l'operazione atomica disabilitando gli interrupts mettendo a 0 il bit IEc (pag. 25 pops)*/
			setSTATUS(getSTATUS() & (~IECON));

			/*Comando di trasmissione dei dati*/
			/*Nei bit 8 - 15 inserisco il nuovo carattere, nei bit 0 - 7 il comando*/
			Term_devR->term.transm_command = *str << BYTELENGTH | TRANSMITCHAR;

			/*Attendo la fine dell'operazione*/
			op_result = SYSCALL(IOWAIT, TERMINT, Term_asid, 0);


			/*Riabilito gli interrupts rimettendo IEc a 1*/
			setSTATUS(getSTATUS() | IECON);


			/*Verifico che sia andato tutto a buon fine (ovvero lo stato del dispositivo è OKCHARTRANS)*/
			if((op_result & 0x000000FF) == OKCHARTRANS) /* = 5*/
			{
				read++;
				str++;
			}

			/*Altrimenti ritorno il negato del device status ed esco dal ciclo*/
			else
			{
				read = - (op_result & 0x000000FF);
				break; 
			}
		}

		/*Rilascio la mutex*/
		SYSCALL(VERHOGEN, (int) &Sem[Term_sem_index], 0, 0);

		/*Inserisco in v0 il numero di caratteri letti o lo stato di errore*/
		sup_struct->sup_exceptState[GENERALEXCEPT].reg_v0= read;
	}
}


void Read_From_Terminal(support_t* sup_struct, state_t* exceptState)
{
	/*Nel registro a1 si trova l'indirizzo del buffer da cui leggere la stringa (par. 4.7.5 student guide)*/
	char* str = (char*)sup_struct->sup_exceptState[GENERALEXCEPT].reg_a1;
	/*
		Verifichiamo che la stringa da scrivere non si trovi al di fuori dello spazio degli indirizzi dei processi utente
		In caso, il processo viene terminato (par.4.7.5 student guide)
	*/
	if((int)str < KUSEG)
	{
        SIGKILL(NULL);
	}
	else
	{
		/*Ottengo ASID del terminale.*/
		int Term_asid = sup_struct->sup_asid -1;
		
		/* Indice del semaforo associato al terminale*/ 
		int Term_sem_index = (TERMINT - 3) * 8 + Term_asid;  /*TERMINT = 7*/
		
		/*Salvataggio del device register secondo la formula alla fine di pagina 28 pops*/
		devreg_t * Term_devR = (devreg_t*) (BEGIN_DEVREG + ((TERMINT - 3) * 0x80) + (Term_asid * 0x10));
		
		/*Entro nella sezione critica*/
		 SYSCALL(PASSEREN, (int) &Sem[Term_sem_index], 0, 0);
		 
		int op_result;
		int read = 0;
		 
		 while(TRUE)
		 {
			 /*Rendo l'operazione atomica disabilitando gli interrupts mettendo a 0 il bit IEc (pag. 25 pops)*/
			setSTATUS(getSTATUS() & (~IECON));
			
			/*Comando di trasmissione dei dati*/
			Term_devR->term.recv_command = TRANSMITCHAR;
 			/*Attendo la fine dell'operazione (termRead = true)*/
            op_result = SYSCALL(IOWAIT, TERMINT, Term_asid, TRUE);
			
			/*Riabilito gli interrupts rimettendo IEc a 1*/
			setSTATUS(getSTATUS() | IECON);
			
			/*Verifico che sia andato tutto a buon fine (ovvero lo stato del dispositivo è OKCHARTRANS)*/
			if((op_result & 0x000000FF) == OKCHARTRANS) /* = 5*/
			{
				read++;
				/*Carattere successivo*/
				*str = (op_result & 0x0000FF00) >> BYTELENGTH;
				str++;
				
				/*Se sto leggendo il carattere newline*/
				if((op_result & 0x0000FF00) >> BYTELENGTH == '\n')
				{
					/*Esco dal ciclo*/
					break;
				}
			}
			/*Altrimenti ritorno il negato del device status ed esco dal ciclo*/
			else
			{
				read = - (op_result & 0x000000FF);
				break; 
			}
		}
		
		/*Rilascio la mutex*/
		SYSCALL(VERHOGEN, (int) &Sem[Term_sem_index], 0, 0);
		/*Inserisco in v0 il numero di caratteri letti o lo stato di errore*/
		sup_struct->sup_exceptState[GENERALEXCEPT].reg_v0 = read;
	}
}
