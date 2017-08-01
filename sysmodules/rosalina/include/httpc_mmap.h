// File derived from libctru
// https://github.com/smealum/ctrulib/blob/master/libctru/include/3ds/services/httpc.h

/**
 * @file httpc_static.h
 * @brief HTTP service init with statically allocated memory.
 */
#pragma once

#include <3ds/services/httpc.h>

/// Initializes HTTPC with static sharedmem.
Result httpcInitMmap(u32 sharedmem_size);

/// Exits HTTPC.
void httpcExitMmap(void);

