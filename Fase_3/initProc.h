#ifndef INIT_PROC
#define INIT_PROC

#include "sysSupport.h"
#include "vmSupport.h"
#include "../resources/pandos_const.h"
#include "../resources/pandos_types.h"
#include "umps3/umps/libumps.h"

/*
Inizializza Swap Pool table esemafori, crea gli 8 processi e ne gestisce la terminazione
Il nome è rimasto 'test' dalla fase 2
*/
void test();

/*Crea un processo utente*/
void InstantiateProc(int proc_id);

#endif
