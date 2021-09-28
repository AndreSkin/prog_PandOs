#ifndef EXCEPTIONS_INCLUDED
#define EXCEPTIONS_INCLUDED


/* l'Exception Handler del kernel
*/
void exceptionHandler();

/* Copia lo stato dell'eccezione in una locazione accessibile all'livello supporto 
*  e passa il controllo ad una routine sbecificata dal support level 
*/
void PassUporDie(int except_type, state_t* exceptionState);

/* Associa il codice esatto alla corrispettiva SYS e passa i parametri richiesti */
void SysHandler(state_t* exception_state);

/* Crea un processo e lo rende figlio del processo corrente */
void SYS1_CP();

/* Termina il processo corrente e i suoi figli */
void SYS2_TP();

/* Ricorsivamnete termina il processo corrente e i suoi figli (funzione usate in SYS2_TP)*/
void killprocT(pcb_t* toTerm);

/* Esegue una P sul semaforo */
void SYS3_P(int* semAddr);

/* Esegue una V sul semaforo */
void SYS4_V(int* semAddr);

/* Esegue una P sul semaforo che il kernel usa per un I/O device*/
void SYS5_W_IO();

/* Ritorna il tempo totale della CPU usato dal processo corrente  */
void SYS6_GCT();

/* Esegue una P sul semaforo dello pseudo-clock */
void SYS7_W_CLK();

/* Ritorna le informazioni dell livelo di supporto del processo corrente  */
void SYS8_GSD();

#endif
