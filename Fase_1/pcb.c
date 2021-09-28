#include "pcb.h"

/*Lista dei PCB che sono liberi o inutilizzati.*/
static pcb_t* pcbFree_h;

/*Array di PCB. MAXPROC = 20*/
static pcb_t pcbFree_table[MAXPROC];


void initPcbs()
{
    pcbFree_h = &pcbFree_table[0]; //Assegno al primo elemento della lista il valore del primo elemento dell'array
    pcb_t* head = pcbFree_h;	   //Creo un puntatore alla testa della lista
	
    int i = 1;

    while (i < MAXPROC)
    {
        head->p_next = &pcbFree_table[i];	//Assegno il valore e
        head = head->p_next;				//sposto current una posizione avanti
        i = i + 1;
    }

    head->p_next = NULL;						//Una volta terminato pongo NULL alla fine della lista
}

void freePcb(pcb_t* p)
{
    if (p != NULL)	//Controllo se l'elemento passato alla funzione è NULL
    {
    	/*Inserimento in testa*/
        p->p_prev = NULL;		//p sarà il primo elemento quindi non avrà predecessori
        p->p_next = pcbFree_h;	//Subito dopo p c'è il resto della lista
        pcbFree_h = p;			//La lista diventa la lista precedente con l'aggiunta di p
    }
}

void initializePcb(pcb_t* node)
{
    if (node != NULL)
    {
        node->p_next = NULL;
        node->p_prev = NULL;
        node->p_child = NULL;
        node->p_next_sib = NULL;
        node->p_prnt = NULL;
        node->p_prev_sib = NULL;
    }
}

pcb_t *allocPcb()
{
    if (pcbFree_h == NULL)
    {
        return NULL;	//Ritorno NULL se la lista è vuota
    }
    else
    {
        pcb_t* toReturn = pcbFree_h;	//Salvo il valore da rimuovere in questo puntatore 
        pcbFree_h = pcbFree_h->p_next;  //Faccio avanzare la lista rimuovendo di fatto il primo elemento
        
        /*Inizializzo i campi a NULL*/
	initializePcb(toReturn);

        return toReturn;	//Restituisco l'elemento rimosso
    }
}


pcb_t* mkEmptyProcQ()
{
    return NULL;		
}


int emptyProcQ(pcb_t *tp)
{
    if (tp == NULL)		//Se la lista puntata è vuota
    {
		return TRUE;
    }
    return FALSE;
}


void insertProcQ(pcb_t** tp, pcb_t* p)
{
	if ((*tp) != NULL && p!= NULL)	//Se la coda non è vuota
	{
	    pcb_t* head = (*tp)->p_next; //Creo un puntatore all'ultimo elemento della coda
	    p->p_prev = (*tp);			 //p si troverà prima del resto della coda 
	    p->p_next = head;			 //e dopo tail
		
	    head->p_prev = p;
	    (*tp)->p_next = p;
		/*Ora che p è stato posizionato aggiorno la coda tp in modo che includa anche p*/
	    (*tp) = p;
	}
	else if ((*tp) == NULL && p!= NULL) //Se invece la coda non contiene elementi
	{
	    (*tp) = p;				//Inserisco p
	    (*tp)->p_next = (*tp);	//Primo e utimo elemento della coda
	    (*tp)->p_prev = (*tp);	//saranno proprio p
	}
    
}


pcb_t *headProcQ(pcb_t *tp)
{
    if (tp == NULL) //Se la coda è vuota
    {
    	return NULL;
    }
	//Altrimenti
    return tp ->p_next;        // Restituisce l'elemento in testa
}


pcb_t* removeProcQ(pcb_t **tp)
{
    if (*tp == NULL) //Se la coda è vuota
    {
    	return NULL;
    }
    else if (*tp == (*tp)->p_next) //Se la coda ha un solo elemento
    {
        pcb_t* head = *tp;	//Salvo l'elemento da rimuovere in un puntatore

        *tp = NULL; //Rimuove l'elemento dalla lista

        return head;
    }
    else
    {
        pcb_t* head = (*tp)->p_next;    // Rimuove l'elemento in testa.
        (*tp)->p_next = head->p_next;

        pcb_t* tmp = head->p_next;
        tmp->p_prev = (*tp);

        return head;
    }
}


pcb_t* outProcQ(pcb_t **tp, pcb_t *p)
{
	if ((tp == NULL) || (*tp == NULL) || p == NULL)return NULL;
	else
        {
		if ((*tp) != p) //Se p non è il primo elemento
		{
		    pcb_t* tmp = (*tp)->p_prev;	// Scorro la coda

		    while (tmp != (*tp))
		    {
		        if (tmp == p)			//Se trovo l'elemento
		        {
		            tmp->p_prev->p_next = tmp->p_next;
		            tmp->p_next->p_prev = tmp->p_prev;  //Lo rimuovo
		            return tmp;
		        }
		        tmp = tmp->p_prev;
		    }
		    return NULL;		//Se non lo trovo ritorno NULL
		}
		else if ((*tp) == (*tp)->p_next && (*tp) == p) // Se tp ha un solo elemento = p
		{
			/*Rimuovo p*/
		    pcb_t* tmp = (*tp);
		    *tp = NULL;

		    return tmp;
		}
		else  //Se l'elemento puntato è proprio p
		{
			/*Rimuovo p*/
		    pcb_t* tmp = (*tp);

		    (*tp) = (*tp)->p_next;

		    (*tp)->p_prev = tmp->p_prev;
		    tmp->p_prev->p_next = (*tp);

		    return tmp;
		}
		
    }
    return NULL;
}


int emptyChild(pcb_t *p)
{
    if ((p != NULL) && (p->p_child == NULL)) //Se p non ha figli
    {
    	return TRUE;
    }
    return FALSE;
}

void insertChild(pcb_t *prnt, pcb_t *p)
{
    if (prnt != NULL && p != NULL)
    {
    	/*Inserisco p come primo figlio*/
        pcb_t* tmp = prnt->p_child;
        prnt->p_child = p;
        p->p_next_sib = tmp;
        p->p_prnt = prnt;

        if (tmp != NULL) 
        {
        	tmp->p_prev_sib = p;    //Se c'è un solo figlio alla fine, pongo tmp->prev_sib a p.
        }                           
    }
}

pcb_t* removeChild(pcb_t *p)
{
    if (p != NULL && p->p_child != NULL) //p ha un figlio
    {
        pcb_t* tmp = p->p_child;

        if (tmp->p_next_sib != NULL)     //Se p ha next_sib
        {
            p->p_child = tmp->p_next_sib;	//Rimuovo il primo figlio
            tmp->p_next_sib->p_prev_sib = NULL;
        } 
        else
        {
            p->p_child = NULL;  //Se p aveva un solo figlio
        }

        return tmp;
    }

    return NULL;	//Se p non ha figli ritorno NULL
}

pcb_t *outChild(pcb_t* p)
{
	if (p == NULL) return NULL;
    if (p->p_prnt == NULL) //Se p non ha un padre
    {
    	return NULL;
    }

    if (p->p_prev_sib == NULL) 
    {
    	/*Se p non ha fratelli lo rimuovo come primo figlio con la funzione precedente*/
    	return removeChild(p->p_prnt); 
    }
    else
    {
        pcb_t* tmp = p->p_prev_sib;		//Se p è un figlio ma non è il primo
        tmp->p_next_sib = p->p_next_sib;

        if (p->p_next_sib != NULL)  // Se ha un next_sib
        {
        	p->p_next_sib->p_prev_sib = tmp;	//Unisco il fratello precedente di p con il successivo
        }

        return p;
    }
}

