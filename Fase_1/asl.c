#include "asl.h"
#include "pcb.h"

//Array di SEMD. MXPROC = 20
static semd_t semd_table[MAXPROC+2];

//Lista dei SEMD liberi o inutilizzati
static semd_t* semdFree_h;

//Lista dei semafori attivi
static semd_t* semd_h;

static int* max = (int*)MAXINT;


int insertBlocked(int *semAdd, pcb_t *p)
{	
	semd_t* asl = semd_h;

    while (asl->s_semAdd !=max)
    {		
        if (asl->s_semAdd == semAdd) // se il semaforo è gia presente
        {
			//Aggiungo la lista dei processi
            insertProcQ(&(asl->s_procQ), p);
			
			//Aggiungo l'indirizzo del semforo
            p->p_semAdd = semAdd;
            return FALSE;
        }
        // se il semaforo non è presente ne inserisco un nuovo rispettando un ordine crescente
        else if (asl->s_next->s_semAdd > semAdd || asl->s_next->s_semAdd == (int*)MAXINT)
        {

            if (semdFree_h == NULL)
            {
            	return TRUE;
            }

            else
            {
				//Stacco un elemento da semdFree_h
                semd_t* toAdd = semdFree_h; 
                semdFree_h = semdFree_h->s_next;

				//Aggiungo un elemento alla semd_h
                toAdd->s_next = asl->s_next;
                asl->s_next = toAdd;

				//Aggiungo la coda di proessi all semaforo aggiunto 
                toAdd->s_procQ = mkEmptyProcQ();
                insertProcQ(&(toAdd->s_procQ), p);
				
				// Aggiungo l'indirizzo del semaforo 
                p->p_semAdd = semAdd;

                // Aggiornamento degli indirizzi dei semafori
                toAdd->s_semAdd = semAdd;

                return FALSE;
            }
        }
			
        asl = asl->s_next;
    }

    return FALSE;
}


pcb_t* removeBlocked(int *semAdd)
{
    semd_t* asl = semd_h->s_next;

    semd_t* aslPrevious = semd_h;

    while (asl->s_semAdd !=max)
    {
        if (asl->s_semAdd == semAdd)
        {
			//Rimuovo il primo elemento dalla coda dei processi e lo salvo
            pcb_t* toReturn = removeProcQ(&(asl->s_procQ));

			//Se la coda dei processi del semaforo è vuota
            if (emptyProcQ(asl->s_procQ))
            {
				
                aslPrevious->s_next = asl->s_next;

                semd_t* toAdd = asl; //Eleemnto da aggiungere alla semdFree_h

                //Inserimento in testa a semdFree_h
                toAdd->s_next = semdFree_h;
                semdFree_h = toAdd;

            }

	    if (toReturn != NULL) toReturn->p_semAdd = NULL;
            return toReturn;
        }

        aslPrevious = asl;

        asl = asl->s_next;
    }
    return NULL;
}

pcb_t* outBlocked(pcb_t *p)
{
    semd_t* asl = semd_h->s_next;
	semd_t* aslprevious = semd_h;

    if (p == NULL || p->p_semAdd == NULL) return NULL;

    while (asl->s_semAdd !=max)
    {
        // Se il semAdd è quello cercato ...
	if(asl->s_semAdd == p->p_semAdd)
        {
            pcb_t* toReturn = outProcQ(&(asl->s_procQ), p);

            if (emptyProcQ(asl->s_procQ))    // caso in cui il semaforo è stato svuotato, 
            {                                   // lo togliamo dalla ASL
                aslprevious->s_next = asl->s_next;  

                semd_t* toAdd = asl;     // aggiungiamo il descrittore tolto alla lista dei descrittori liberi

                toAdd->s_next = semdFree_h;  // Inserimento in testa a semdFree_h
                semdFree_h = toAdd;
            }

            if (toReturn != NULL) toReturn->p_semAdd = NULL;
            return toReturn;
        }
        aslprevious = asl;
        asl = asl->s_next;
    }

    return NULL;
}

pcb_t* headBlocked(int *semAdd)
{
    semd_t* asl = semd_h;

    while (asl->s_semAdd !=max)
    {
        // Corrisponde al semAdd cercato
        if(asl->s_semAdd == semAdd)
        {
            //Se la coda dei processi è vuota 
            if (asl->s_procQ == NULL){
				return NULL;
			}
			
            //Altrimenti ritorna il pcb in testa senza rimuoverlo
            else
				return asl->s_procQ->p_prev;

        }

        asl = asl->s_next;
    }

    return NULL;
}

void initASL()
{
    semdFree_h = &semd_table[1];

    semd_t* asl = semdFree_h;

    int i = 2;

	//Inserisco tutti la semd_table in semdFree_h
    while (i < MAXPROC+1)
    {
        asl->s_next = &semd_table[i];
        asl = asl->s_next;
        i = i + 1;
    }

    asl->s_next = NULL;

    //Inizializzo un nodo sentinella per avere il più piccolo elemento in testa
    semd_h = &semd_table[0];
    semd_h->s_semAdd = (int*)0x00000000;
    semd_h->s_procQ = NULL;

    //Inizializzo un nodo sentinella per avere il più grande elemento possibile in coda
    semd_h->s_next = &semd_table[MAXPROC +1];
    semd_h->s_next->s_semAdd = max;
    semd_h->s_next->s_procQ = NULL;

    semd_h->s_next->s_next = NULL;
}
