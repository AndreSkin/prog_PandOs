#ifndef INTERRUPTS_INCLUDED
#define INTERRUPTS_INCLUDED

/* Cicla e gestisce tutti gli interrupt in attesa, dalla priorità più alta a quella più bassa  */ 
void getInterruptLine(unsigned int cause_reg);

/* Cicla e gestisce tutti gli interrupt interni in attesa, dalla priorità più alta a quella più bassa */
void General_Interrupt(int line);

/* Gestisce i non-timer interrupt  */
void Non_T_Int();

#endif

