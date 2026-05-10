#pragma once

/**
 * Instruction class of the QRL application.
 */
#define CLA 0xE0

/**
 * Length of APPNAME variable in the Makefile.
 */
#define APPNAME_LEN (sizeof(APPNAME) - 1)

/**
 * Maximum length of MAJOR_VERSION || MINOR_VERSION || PATCH_VERSION.
 */
#define APPVERSION_LEN 3

/**
 * Maximum length of application name.
 */
#define MAX_APPNAME_LEN 64

/**
 * Maximum transaction length (bytes).
 */
#define MAX_TRANSACTION_LEN 510

/**
 * Maximum signature length (bytes).
 */
#define MAX_DER_SIG_LEN 72

/**
 * Exponent used to convert the smallest unit to QRL when formatting an
 * amount for display (N QRL = N * 10^EXPONENT_SMALLEST_UNIT smallest units).
 *
 * NOTE: post-migration QRL is on EVM, where the on-chain unit is wei
 * (10^-18 QRL). The Ledger transaction format here stores `value` as
 * uint64_t (see src/transaction/tx_types.h), so the exponent / unit must
 * match how the wallet encodes the amount before sending it to the device.
 */
#define EXPONENT_SMALLEST_UNIT 3

/**
 * Prefix byte for Zond addresses.
 */
#define ZOND_ADDRESS_PREFIX 'Q'
