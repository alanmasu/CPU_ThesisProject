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

#define TIMES_NUM 9

uint32_t times[TIMES_NUM] = {0}; 
uint32_t timesIndex = 0;
char labels[TIMES_NUM][100] = {
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

int main(int argc, char const *argv[]){
    printf("Starting code compiled at %s %s\n", __DATE__, __TIME__);
    const int B_M = 2;
    const int B_N = 3;
    const int B_K = 4;
    const uint32_t signCmp = 48;

    const int M = B_M * BINARY_FRAG_SIZE;
    const int N = B_N * BINARY_FRAG_SIZE;
    const int K = B_K * BINARY_FRAG_SIZE;
    
    timerSetMode(Timer0RegFile, CONTINUOUS);
    timerSetCounter(Timer0RegFile, 0);
    timerStart(Timer0RegFile);
 
    BinaryMatrix_t A = (BinaryMatrix_t)malloc(M * (N / 32) * sizeof(uint32_t));         //768 byte
    BinaryMatrix_t W = (BinaryMatrix_t)malloc(N * (K / 32) * sizeof(uint32_t));         // 1.536 kB
    BinaryMatrix_t O = (BinaryMatrix_t)malloc(M * (K / 32) * sizeof(uint32_t));         // 1 kB
    BinaryMatrix_t OSerial = (BinaryMatrix_t)malloc(M * (K / 32) * sizeof(uint32_t));   // 1 kB

    times[timesIndex++] = timerGetCounter(Timer0RegFile);

    if(!A || !W || !O || !OSerial){
        PRINTF_ERR("Memory allocation failed!\n");
        while(1);
    }

    PRINTF_DBG("Memory allocation successful!\n");

    PRINTF_DBG("\nInitializing matrices...\n");
    for(int i = 0; i < M * (N / 32); ++i){
        A[i] = i + 1;
    }
    for(int i = 0; i < N * (K / 32); ++i){
        W[i] = i;
    }
    memset(O, 0, M * (K / 32) * sizeof(uint32_t));

    times[timesIndex++] = timerGetCounter(Timer0RegFile);

    PRINTF_DBG("\nMatrice A:\n");
    PROFILING_ENV_EXCLUSION(printIntBMatrixN(A, 2, 2, M, N);)

    PRINTF_DBG("\nMatrice W:\n");
    PROFILING_ENV_EXCLUSION(printIntBMatrixN(W, 2, 2, N, K);)

    PRINTF_DBG("\nMatrice O (inizializzata a zero):\n");
    PROFILING_ENV_EXCLUSION(printIntBMatrixN(O, 2, 2, M, K);)


    PRINTF_DBG("\nLoading matices to BTPU fragments...\n");
    loadBinaryMatrixToFragments(A, BTPU0_IO0_MEMORY, M, N);
    loadBinaryMatrixToFragments(W, BTPU0_W_MEMORY, N, K);

    times[timesIndex++] = timerGetCounter(Timer0RegFile);

    btpuSetBlocks(BTPU0RegFile, B_M, B_N, B_K);
    btpuSetAddrs(BTPU0RegFile, 0, 0, B_N * B_K); 

    times[timesIndex++] = timerGetCounter(Timer0RegFile);

    PRINTF_DBG("\nStarting first multiplication...\n");
    btpuStartBinaryMatrixMul(BTPU0RegFile, signCmp, true, true, BTPU_USE_MEMORY_0_CONFIG);

    times[timesIndex++] = timerGetCounter(Timer0RegFile);

    btpuWaitBinaryMatrixMul(BTPU0RegFile);

    times[timesIndex++] = timerGetCounter(Timer0RegFile);

    storeFramentsToBinaryMatrix(BTPU0_IO1_MEMORY + (B_N * B_K), O, M, K);

    times[timesIndex++] = timerGetCounter(Timer0RegFile);

    fastBinaryMatrixMul(A, W, OSerial, signCmp, M, N, K);

    times[timesIndex++] = timerGetCounter(Timer0RegFile);


    free(A);
    free(W);
    free(O);
    free(OSerial);

    times[timesIndex++] = timerGetCounter(Timer0RegFile);
    

    PRINTF_LOG("\nExecution terminated.\n");
    if(timesIndex > TIMES_NUM){
        PRINTF_ERR("Error: Too many time measurements taken (%d > %d)\n", timesIndex, TIMES_NUM);
        while(1);
    }

    PRINTF_LOG("Label,TimerVal_tiks\n");
    for (int i = 0; i < TIMES_NUM; ++i){
        PRINTF_LOG("%s,%u\n", labels[i], times[i]);
    }

    PRINTF_DBG("Setting BRAM Port to EXTERNAL...\n");
    BTPU0RegFile->creg.reg.BRAM_PORT_SEL = BTPU_BRAM_PORT_SEL_EXT; // Set BRAM port to external

    PRINTF_DBG("Trap CPU...\n");
    while(1);

    return 0;
}
