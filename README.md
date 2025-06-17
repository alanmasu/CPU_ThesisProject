# RISC-V Template Project

## Struttura del progetto
```
.
├── build 
│   ├── bin [GIT UNTRACKED]
│   │   ├── test
│   │   │   └── <all test binaries>
│   │   └── main.elf
│   └── obj [GIT UNTRACKED]
│       ├── libraries
│       │   └── <all library folders containing object files>
│       └── src
│           └── <all source files compiled to object files>
├── include
│   └── <all include files>
├── libraries
│   └── <all library folders>
│        ├── src
│        │   └── <all source files for the library>
│        └── include
│            └── <all include files for the library>
├── src
│   ├── main.c
│   └── <here are placed Project Sources>
├── test
│   ├── <test 1 folder>
│   │   └── test1.c
│   ├── <test 2 folder>
│   │   └── test2.c
│   └── <test x folder>
│       └── testx.c
├── scripts
│   ├── <all scripts for build, test, etc.>
│   └── <all scripts for the project>
├── scratch [UNTRACKED]
├── linker_script.ld
├── activate 
├── do_build
└── Makefile
```

## Compilazione
Per compilare il progetto, eseguire il comando viene usato `make`, tuttavia ci sono varie regole per la compilazione:
 - `make` Compila librerie e sorgenti, e linka tutto seguendo il linker_script. Crea la cartella `build/bin` e `build/obj` se non esistono.
 - `make test` Compila le librerie ed i sorgenti presenti nelle varie sotto cartelle di `test`, linkando tutte 
le sotto directory di `test` in eseguibili separati assieme alle librerie. Utile per testare le unità.
 - `make all` Esegue `make` e `make test`, compilando tutto il progetto ed i test.
 - `make sim` Come `make all` ma definendo la macro `SIMULATION`, utile per testare le unità in modalità simulazione. 
 - `make dump` Esegue il dump dell'eseguibile principale
 - `make symbols` Esegue il dump dei simboli dell'eseguibile principale
 - `make sections` Esegue il dump delle sezioni dell'eseguibile principale
 - `make clean` Elimina l'esecutable principale.
 - `make cleanall` Elimina l'esecutable principale, le librerie e gli oggetti compilati.

### Script `do_build`
Il file `do_build` è uno script che oltre ad eseguire le direttive make accetta delle opzioni e genera ulteriori file, esportando anche gli eseguibili in vari formati.
Accetta le seguenti opzioni:
 - `-c` Esegue un clean prima di compilare
 - `-s` Compila in modalità simulazione, inoltre esporta gli eseguibili dei test **di questo progetto** in SystemVerilog nella directory del progetto della CPU.
 - `-S` Come `-s` ma esporta gli eseguibili di tutti i test in SystemVerilog.
 - `-t` Compila i test **di questo progetto** esportandoli in header per la compilazione assieme agli ARM
 - `-T` Come `-t` ma esporta tutti i test in header per la compilazione assieme agli ARM
 - `-f <file>` Esporta l'eseguibile `<file>` in un header nel progetto del loader, nel file `testLibProgram.h`
> **NOTA**: per definire quali test fanno parte del progetto, è necessario modificare lo script.