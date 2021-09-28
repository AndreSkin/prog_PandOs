#include "../resources/pandos_const.h"
#include "../resources/pandos_types.h"

/*LISTA DEI PCB*/
/**
* Inizializza la pcbFree in modo da contenere 
* tutti gli elementi della pcbFree_table
**/
void initPcbs();

/**
* Inserisce il PCB puntato da p nella lista
* dei PCB liberi (pcbFree_h)
**/
void freePcb(pcb_t* p);

/** 
* Inizializza tutti i campi di un pcb_t* a NULL
**/
void initializePcb(pcb_t* node);

/**
* Restituisce NULL se la pcbFree_h è vuota.
* Altrimenti rimuove un elemento dalla
* pcbFree, inizializza tutti i campi (NULL/0)
* e restituisce l’elemento rimosso.
**/
pcb_t *allocPcb();

/**
* Crea una lista di PCB, 
* inizializzandola come lista vuota
**/
pcb_t* mkEmptyProcQ();

/**
* Restituisce TRUE se la lista puntata da
* tp è vuota, FALSE altrimenti.
**/
int emptyProcQ(pcb_t *tp);

/**
* Inserisce l’elemento puntato da p 
* nella coda dei processi tp
**/
void insertProcQ(pcb_t** tp, pcb_t* p);

/**
* Restituisce l’elemento in fondo alla coda
* dei processi tp, senza rimuoverlo.
* Ritorna NULL se la coda non ha elementi.
**/
pcb_t *headProcQ(pcb_t *tp);

/**
* Rimuove l’elemento piu’ vecchio dalla coda tp.
* Ritorna NULL se la coda è vuota, altrimenti 
* ritorna il puntatore all’elemento rimosso dalla lista
**/
pcb_t* removeProcQ(pcb_t **tp);

/**
* Rimuove il PCB puntato da p dalla coda dei
* processi puntata da tp. Se p non è presente
* nella coda, restituisce NULL
**/
pcb_t* outProcQ(pcb_t **tp, pcb_t *p);


/*ALBERI DI PCB*/
/**
* Restituisce TRUE se il PCB puntato da p
* non ha figli, FALSE altrimenti.
**/
int emptyChild(pcb_t *p);

/**
* Inserisce il PCB puntato da p come figlio
* del PCB puntato da prnt.
**/
void insertChild(pcb_t *prnt, pcb_t *p);

/**
* Rimuove il primo figlio del PCB puntato
* da p. Se p non ha figli, restituisce NULL.
**/
pcb_t* removeChild(pcb_t *p);

/**
* Rimuove il PCB puntato da p dalla lista
* dei figli del padre. Se il PCB puntato da
* p non ha un padre, restituisce NULL,
* altrimenti restituisce l’elemento rimosso
**/
pcb_t *outChild(pcb_t* p);

