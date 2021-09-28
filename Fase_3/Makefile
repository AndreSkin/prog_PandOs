XT_PRG_PREFIX = mipsel-linux-gnu-
CC = $(XT_PRG_PREFIX)gcc
LD = $(XT_PRG_PREFIX)ld


ifneq ($(wildcard /usr/bin/umps3),)
	UMPS3_DIR_PREFIX = /usr
else
	UMPS3_DIR_PREFIX = /usr/local
endif

UMPS3_DATA_DIR = $(UMPS3_DIR_PREFIX)/share/umps3
UMPS3_INCLUDE_DIR = $(UMPS3_DIR_PREFIX)/include/umps3
UMPS3_INCLUDE_DIR2 = $(UMPS3_DIR_PREFIX)/include

DIR1 = ../Fase_1
DIR2 = ../Fase_2
DIR3 = ../Fase_3
DIR_TEST = ../resources
PROJ_INCLUDE = -I$(../) -I$(DIR1) -I$(DIR2) -I$(DIR3)

CFLAGS_LANG = -ffreestanding
CFLAGS_MIPS = -mips1 -mabi=32 -mno-gpopt -G 0 -mno-abicalls -fno-pic -mfp32
CFLAGS = $(CFLAGS_LANG) $(CFLAGS_MIPS) -I$(UMPS3_INCLUDE_DIR) -I$(UMPS3_INCLUDE_DIR2) -Wall -O0


LDFLAGS = -G 0 -nostdlib -T $(UMPS3_DATA_DIR)/umpscore.ldscript


VPATH = $(UMPS3_DATA_DIR):$(../):$(DIR1):$(DIR2):$(DIR3):$(DIR_TEST)

.PHONY : all clean

all : kernel.core.umps

kernel.core.umps : kernel
	umps3-elf2umps -k $<

kernel : asl.o pcb.o initial.o scheduler.o crtso.o interrupts.o exceptions.o libumps.o mcpy.o vmSupport.o sysSupport.o initProc.o
	$(LD) -o $@ $^ $(LDFLAGS)

clean :
	-rm -f *.o kernel kernel.*.umps ../resources/*.umps && clear

%.o : %.S
	$(CC) $(CFLAGS) -c -o $@ $<
	
