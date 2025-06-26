#include <main.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <BinaryMatMul.h>
#include <UART.h>
#include <OLED.h>
#include <GPIO.h>
#include <utilities.h>
#include <Timer.h>
#include <inttypes.h>

#define PROFILING
#define PRINT_RESULTS

#if !defined(SIMULATION) && !defined(PROFILING)
    #define PRINTF_DBG(...) printf(__VA_ARGS__)
#else 
    #define PRINTF_DBG(...)
#endif

#if !defined(SIMULATION) && !defined(PROFILING)
    #define PROFILING_ENV_EXCLUSION(x) x
#else
    #define PROFILING_ENV_EXCLUSION(x)
#endif

#ifndef SIMULATION
    #define PRINTF_ERR(...) printf(__VA_ARGS__)
#else
    #define PRINTF_ERR(...)
#endif

#ifdef PROFILING
    #define PRINTF_LOG(...) printf(__VA_ARGS__)
#else
    #define PRINTF_LOG(...)
#endif

#define ITERATIONS 4
#define TIMES_NUM 9

uint64_t times[TIMES_NUM * ITERATIONS] = {0};
uint32_t sizes[ITERATIONS] = {0};
uint32_t sizeIndex = 0;
uint32_t timesIndex = 0;
uint32_t testN = 0;

#ifdef PRINT_RESULTS  
char labels[TIMES_NUM][25] = {
    "Allocazione",
    "Popolazione",
    "Caricamento",
    "Settaggio",
    "Inizializzazione",
    "ComputazioneBTPU",
    "LetturaRisultato",
    "ComputazioneSerialeFast",
    "freeMemory"
};
#endif

void printResults(){
    #ifdef PRINT_RESULTS
    printf("size(bit),");
    for(int label = 0; label < TIMES_NUM - 1; ++label){
        printf("%s,", labels[label]);
    }
    printf("%s,platform\n", labels[TIMES_NUM - 1]);
    

    uint64_t toPrint = 0;
    for(int size = 0; size < sizeIndex; ++size){
        printf("%d,", sizes[size]);
        for (int time = 0; time < TIMES_NUM - 1; ++time) {
            if(time == 0){
                toPrint = times[size * TIMES_NUM + time];
            } else {
                toPrint = times[size * TIMES_NUM + time] - times[size * TIMES_NUM + time - 1];
            }
            printf("%u,", (uint32_t)(toPrint / 40)); // Convert to microseconds
        }
        toPrint = times[size * TIMES_NUM + TIMES_NUM - 1] - times[size * TIMES_NUM + TIMES_NUM - 2];
        printf("%u,FPGA\n", (uint32_t)(toPrint / 40));
    }
    #else 
        printf("Results printing is disabled. Define PRINT_RESULTS to enable it.\n");
    #endif
}

int main(int argc, char const *argv[]){
    printf("Starting code compiled at %s %s\n", __DATE__, __TIME__);

    const uint32_t signCmp = 48;

    const int N = 256;
    const int I_SIZE = N * (N / 32) * 4;
    const int W_SIZE = N * (N / 32) * 4;
    const int O_SIZE = N * (N / 32) * 4;
    const int TOTAL_SIZE = I_SIZE + W_SIZE + O_SIZE;

    int bn = 0;

    for(int i = 0, n = 32; i < ITERATIONS; ++i, n *= 2){
        bn = n / 32;

        timerSetMode(Timer0RegFile, CONTINUOUS);
        timerSetCounter(Timer0RegFile, 0);
        timerStart(Timer0RegFile);

        BinaryMatrix_t A = (BinaryMatrix_t)malloc(n * (n / 32) * sizeof(uint32_t));         //768 byte
        BinaryMatrix_t W = (BinaryMatrix_t)malloc(n * (n / 32) * sizeof(uint32_t));         // 1.536 kB
        BinaryMatrix_t O = (BinaryMatrix_t)malloc(n * (n / 32) * sizeof(uint32_t));         // 1 kB
        BinaryMatrix_t OSerial = (BinaryMatrix_t)malloc(n * (n / 32) * sizeof(uint32_t));   // 1 kB

        times[timesIndex++] = timerGetCounter(Timer0RegFile);

        if(!A || !W || !O || !OSerial){
            PRINTF_ERR("[ERROR]: Memory allocation failed -> N: %d\n", n);
            PRINTF_ERR("A: %p, W: %p, O: %p, OSerial: %p\n", A, W, O, OSerial);
            printResults();
            PRINTF_ERR("Exiting...\n");
            while(1);
        }
        sizes[sizeIndex++] = n;
        ++testN;

        PRINTF_DBG("Memory allocation successful!\n");

        PRINTF_DBG("\nInitializing matrices...\n");
        for(int i = 0; i < n * (n / 32); ++i){
            A[i] = i + 1;
        }
        for(int i = 0; i < n * (n / 32); ++i){
            W[i] = i;
        }
        memset(O, 0, n * (n / 32) * sizeof(uint32_t));
        memset(OSerial, 0, n * (n / 32) * sizeof(uint32_t));

        times[timesIndex++] = timerGetCounter(Timer0RegFile);

        PRINTF_DBG("\nMatrice A:\n");
        PROFILING_ENV_EXCLUSION(printIntBMatrixN(A, 2, 2, n, n);)

        PRINTF_DBG("\nMatrice W:\n");
        PROFILING_ENV_EXCLUSION(printIntBMatrixN(W, 2, 2, n, n);)

        PRINTF_DBG("\nMatrice O (inizializzata a zero):\n");
        PROFILING_ENV_EXCLUSION(printIntBMatrixN(O, 2, 2, n, n);)


        PRINTF_DBG("\nLoading matices to BTPU fragments...\n");
        loadBinaryMatrixToFragments(A, BTPU0_IO0_MEMORY, n, n);
        loadBinaryMatrixToFragments(W, BTPU0_W_MEMORY, n, n);

        times[timesIndex++] = timerGetCounter(Timer0RegFile);

        btpuSetBlocks(BTPU0RegFile, bn, bn, bn);
        btpuSetAddrs(BTPU0RegFile, 0, 0, bn * bn); 

        times[timesIndex++] = timerGetCounter(Timer0RegFile);

        PRINTF_DBG("\nStarting first multiplication...\n");
        btpuStartBinaryMatrixMul(BTPU0RegFile, signCmp, true, true, BTPU_USE_MEMORY_0_CONFIG);

        times[timesIndex++] = timerGetCounter(Timer0RegFile);

        btpuWaitBinaryMatrixMul(BTPU0RegFile);

        times[timesIndex++] = timerGetCounter(Timer0RegFile);

        storeFramentsToBinaryMatrix(BTPU0_IO1_MEMORY + (bn * bn), O, n, n);

        times[timesIndex++] = timerGetCounter(Timer0RegFile);

        fastBinaryMatrixMul(A, W, OSerial, signCmp, n, n, n);

        times[timesIndex++] = timerGetCounter(Timer0RegFile);


        free(A);
        free(W);
        free(O);
        free(OSerial);

        A = NULL;
        W = NULL;
        O = NULL;
        OSerial = NULL;

        times[timesIndex++] = timerGetCounter(Timer0RegFile);
        timerStop(Timer0RegFile);

    }
    

    PRINTF_LOG("\nAll tests completed!\n");
    PRINTF_LOG("\nPrinting results:\n");
    printResults();   

    PRINTF_LOG("Setting BRAM Port to EXTERNAL...\n");
    BTPU0RegFile->creg.reg.BRAM_PORT_SEL = BTPU_BRAM_PORT_SEL_EXT; // Set BRAM port to external

    PRINTF_LOG("Trap CPU...\n");
    while(1);

    return 0;
}
