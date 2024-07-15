/// @file all.h
/// @brief File da includere nei programmi utente
#pragma once

#include <costanti.h>
#include <libce.h>
#include <sys.h>
#include <io.h>
#include "lib.h"
#include <cstdint>

extern "C" void flushCacheLine(void *p);

extern "C" uint64_t __rdtscp(unsigned int *aux);

extern "C" void __lfence();

/*! @brief Disabilita l'esecuzione speculativa per il processo
*  @param cmd Il comando da eseguire (PR_SPEC_ENABLE o PR_SPEC_DISABLE)
*/
extern "C" void ssb_ctrl(int cmd);
