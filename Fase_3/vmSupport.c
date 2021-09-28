#include "vmSupport.h"

/*Par. 4.4.1 student guide*/
swap_t SwapPool[UPROCMAX * 2];
/*La swap pool table è una struttura condivisa che va acceduta in mutex*/
int SwapPoolSem;

extern int Master_Sem;
extern int Sem[MAX_SUB_DEV];

extern pcb_t* CurrProc;

void uTLB_RefillHandler() 
{
	/*Pag.47 student guide*/

	/*Passaggio 1, determinare il page number p della TLB entry mancante*/
	/*Ottiene l'indirizzo di BIOSDATAPAGE*/
	state_t* CurrProc_Saved_ex_state = (state_t*) BIOSDATAPAGE;
	/*Trova il numero della pagina*/
	int p = (CurrProc_Saved_ex_state->entry_hi);

	/*Passaggio 2, trovo la Page table entry per p*/
	int p_Page_Table_entry = (((p & 0xFFFFF000) >> VPNSHIFT) - 0x80000);
	/*
	0xFFFFF000 = Primi 20 bit a 1 e gli altri a 0 in modo da mantenere il  VPN
	Lo shift di 12 bit serve ad "isolare" il VPN
	0x80000 = 1 seguito da 19 0, serve ad eliminare il bit 31 di EntryHi, che è la stack page
	(par. 6.3.2 pops)
	*/

	/*Le prime 31 pagine sono dedicate alle pagine .data e .text dello spazio logico degli indirizzi (pag.45 Student Guide)*/
	if (p_Page_Table_entry > 30 || p_Page_Table_entry < 0)
	{
		p_Page_Table_entry = 31;
	}

	pteEntry_t CurrProc_PgTbl = CurrProc->p_supportStruct->sup_privatePgTbl[p_Page_Table_entry];

	/*Passaggio 3, scrivo la Page Table Entry nella TLB*/
	setENTRYHI(CurrProc_PgTbl.pte_entryHI);
    setENTRYLO(CurrProc_PgTbl.pte_entryLO);
	/*Scrive la entry nel TLB*/
    TLBWR();

	/*Passaggio 4, ritorna il controllo al current proc*/
    LDST(CurrProc_Saved_ex_state);
}

void pager()
{
	/*
	* Pagina 50 student guide:
		• Page fault on a load operation: TLB-Invalid exception – TLBL
		• Page fault on a store operation: TLB-Invalid exception – TLBS
		• An attempted write to a read-only page: TLB-Modification exception – Mod
			TLBModification exceptions should not occur. If they do, they should be treated as a program trap
	*/

	/*Passaggio 1, ottenere tramite SYS8 la struttura di supporto del current process*/
	support_t* sup_struct = (support_t*)SYSCALL(GETSUPPORTPTR, 0, 0, 0);

	/*Passaggio 2, ottenere la causa della TLB exception*/
	/*Lo stato responsabile della TLB exception si trova nell'omonima struttura di supporto (par. 4.4.2 student guide)*/
	state_t* exceptState = (state_t*)&(sup_struct->sup_exceptState[PGFAULTEXCEPT]);
	/*Determina la causa dell'eccezione*/
	int cause = (exceptState->cause & GETEXECCODE) >> CAUSESHIFT;
	
	/*Passaggio 3, Se la causa è una TLB-Modification exception va trattata come una Program trap*/
	if (cause != TLBINVLDL && cause != TLBINVLDS)
	{
		SIGKILL(NULL);
	}

	/*Passaggio 4, ottenere la mutua esclusione sulla Swap Pool table facendo una 'P' sul relativo semaforo*/
	SYSCALL(PASSEREN, (int)&SwapPoolSem, 0, 0);

	/*Passaggio 5, determinare il page number mancante p*/
	int exc_state_entryhi = (exceptState->entry_hi);
	/*Calcola il page number*/
	int pNum = (((exc_state_entryhi & 0xFFFFF000) >> VPNSHIFT)- 0x80000);
	
	if (pNum > 30 || pNum < 0)
	{
		pNum = 31;
	}

	/*Passaggio 6, l'algoritmo di rimpiazzamento sceglie un frame i dalla swap pool*/
	int frame_i = rimpiazzamento();

	/*Passaggio 7, controllare se la pagina corrispondente è occupata*/
	/*Ottengo l'indirizzo della pagina*/
	int page = SWAP_POOL_STARTING_ADDRESS + (frame_i * PAGESIZE);

	if (SwapPool[frame_i].sw_asid != -1)
	{
		/*Passaggio 8*/
		/*È richiesto che avvenga atomicamente quindi disabilito gli interrupts*/
		setSTATUS(getSTATUS() & (~IECON));

		/*Pone il V bit a 0 per invalidare la entry*/
		SwapPool[frame_i].sw_pte->pte_entryLO = SwapPool[frame_i].sw_pte->pte_entryLO & ~VALIDON;
		/*Aggiorna il TLB*/
		TLBupdate();

		/*Riabilito gli interrupts*/
		setSTATUS(getSTATUS() | IECON);


		/*Ottengo VPN nella pool*/
		int devNum = SwapPool[frame_i].sw_pageNo;
		/*Ottengo ASID*/
		int pageOwner = SwapPool[frame_i].sw_asid;

		if (SwapPool[frame_i].sw_pte->pte_entryLO & DIRTYON)
		{
			/*Update del backing store del processo x con controllo errori*/
			int update_result = (RW_device(FLASHWRITE, page, devNum, pageOwner - 1)); /* FLASHWRITE = 3 */
			if (update_result != 1)
			{
				SIGKILL(&SwapPoolSem);
			}
		}
	}

	/*Passaggio 9, leggo il contenuto della logical page p del device associato al processo corrente */
	/*Lettura e controllo errori*/
	int asid = sup_struct->sup_asid;
	int read_result = RW_device(FLASHREAD, page, pNum, asid - 1); /* FLASHREAD = 2 */
	if (read_result != 1)
	{
		SIGKILL(&SwapPoolSem);
	}
	
		
	/*Passaggio 10, Update della Swap Pool table’s entry i*/
	pteEntry_t* entry_i = &(sup_struct->sup_privatePgTbl[pNum]);
	SwapPool[frame_i].sw_asid = asid;
	SwapPool[frame_i].sw_pageNo = pNum;
	SwapPool[frame_i].sw_pte = entry_i;

	/*È richiesto che quanto segue avvenga atomicamente quindi disabilito gli interrupts*/
	setSTATUS(getSTATUS() & (~IECON));
	
	/*Passaggio 11, Update della page table del current process per modificare i bit V e PFN*/
	SwapPool[frame_i].sw_pte->pte_entryLO = page | VALIDON | DIRTYON;

	/*Passaggio 12, Update del TLB*/
	TLBupdate();

	/*Riabilito gli interrupts*/
	setSTATUS(getSTATUS() | IECON);

	/*Passaggio 13, rilascio la mutua esclusione*/
	SYSCALL(VERHOGEN, (int)&SwapPoolSem, 0, 0);

	/*Passaggio 14, ritorno il controllo a current process perchè possa ritentare l'istruzione che ha creato il page fault*/
	LDST((state_t*) &(sup_struct->sup_exceptState[PGFAULTEXCEPT]));
}


int RW_device(int RW, int data, int devblock, int devnum)
{
	/*Ottengo il semaforo del flash device*/
	/* Indice del semaforo associato al terminale*/
	int flashSem = (FLASHINT - 3) * 8 + devnum;  /*FLASHINT = 4*/

	/*prende la mutua esclusione sul device register*/
	SYSCALL(PASSEREN, (int) &Sem[flashSem], 0, 0);

	/*Salvataggio del device register secondo la formula alla fine di pagina 28 pops*/
	devreg_t * Dev_devR = (devreg_t*) (BEGIN_DEVREG + ((FLASHINT - 3) * 0x80) + (devnum * 0x10));
    /*carica il data0 con il blocco da leggere o scrivere*/
    Dev_devR->dtp.data0 = data;

	/*Rendo l'operazione atomica disabilitando gli interrupts mettendo a 0 il bit IEc (pag. 25 pops)*/
	setSTATUS(getSTATUS() & (~IECON));

	/*Scrive il comando da effettuare (Par. 4.5.1 student guide)*/
	Dev_devR->dtp.command =  ((devblock) << 8) | RW;
	/*chiama la SYS5 per attendere la fine del I/O*/
	int state = SYSCALL(IOWAIT, FLASHINT, devnum, 0);

	/*Riabilito gli interrupts rimettendo IEc a 1*/
	setSTATUS(getSTATUS() | IECON);

	SYSCALL(VERHOGEN, (int) &Sem[flashSem], 0, 0);
	/*In caso di errore torna -1*/
	if (state != 1){
        return -1;
    }
    return state;
}

void TLBupdate()
{
	/*Student guide par. 4.5.2, approccio 2*/
	TLBCLR();
}

int rimpiazzamento()
{
	/*serve a salvare l'ultima ricerca, static come da student guide par. 4.5.4*/
    static int frame_to_return = 0;
    /*controllo se esiste un frame libero*/
    for (int i = 0; i < UPROCMAX * 2; i++)
    {
        if (SwapPool[i].sw_asid == NOPROC)
        {
			frame_to_return = i;
            return frame_to_return;
        }
    }
    /*Altrimenti incremento e faccio mod la grandezza della Swap Pool (par. 4.5.4 student guide)*/
	frame_to_return = (frame_to_return + 1) % (UPROCMAX * 2);
	return frame_to_return;
}

void SIGKILL(int *semaphore)
{
	for (int i = 0; i < UPROCMAX * 2; i++)
	{
		if (SwapPool[i].sw_asid == CurrProc->p_supportStruct->sup_asid)
		{
			/*Rimette gli ASID a -1*/
			SwapPool[i].sw_asid = NOPROC;
		}
	}

    /*Se il processo era in mutex, questa viene rilasciata*/
    if (semaphore != NULL)
    {
        SYSCALL(VERHOGEN, (int)semaphore, 0, 0);
    }

    /*V sul master semaphore*/ 
    SYSCALL(VERHOGEN, (int) &Master_Sem, 0, 0);

    /*Infine, il processo viene terminato*/
    SYSCALL(TERMPROCESS, 0, 0, 0);
}



