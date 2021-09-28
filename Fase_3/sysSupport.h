#ifndef SYS_SUP
#define SYS_SUP

#include "../resources/pandos_types.h"
#include "vmSupport.h"

/*Gestisce le eccezioni non gestite in fase 2*/
void GeneralExHandler();

/*Nel caso di una syscall, chiama la funzione corrispondente*/
void SyscallExHandler(support_t* sup_struct, state_t* exceptState, unsigned int number_a0);

/*SYSTEM CALLS*/
/*Wrapper per la system call corrispondente del kernel*/
void Terminate();
/*Restituisce il numero di microsecondi passati dall’accensione del sistema*/
void Get_TOD(state_t* exc_state);
/*Richede una stampa ininterrotta della stringa richiesta sulla stampante associata al processo*/
void Write_To_Printer(support_t* except_supp, state_t* exc_state);
/*Richede una stampa ininterrotta della stringa richiesta sul terminale associato al processo*/
void Write_To_Terminal(support_t* except_supp, state_t* exc_state);
/*Legge una riga (fino al newline) dal terminale associato al processo*/
void Read_From_Terminal(support_t* except_supp, state_t* exc_state);

#endif
