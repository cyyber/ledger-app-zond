#pragma once

#include "parser.h"
#include "types.h"


#define DATA_FIRST 0x00

#define DATA_MORE 0x01

#define DATA_LAST 0x02

/**
 * Dispatch APDU command received to the right handler.
 *
 * @param[in] cmd
 *   Structured APDU command (CLA, INS, P1, P2, Lc, Command data).
 *
 * @return zero or positive integer if success, negative integer otherwise.
 *
 */
int apdu_dispatcher(const command_t *cmd);
