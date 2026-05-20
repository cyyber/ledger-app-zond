# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [3.0.0] - 2026-05-20

### Changed (breaking)

- Migrated QRL address format from 48-byte (`Q` + 96 hex chars) to 64-byte
  (`Q` + 128 hex chars) post-quantum addressing. `ADDRESS_SIZE`,
  `ADDRESS_LEN` and `ADDRESS_LENGTH` are now `64` across `src/mldsa87/`,
  `src/transaction/` and `src/mldsa87/rlp_decode.h`.
- `MAX_MEMO_LEN` reduced from `437` to `421` to accommodate the wider
  recipient address while keeping `MAX_TX_LEN` at `510`.
- Transaction RLP `to` field switches from `0xb0` (RLP 48-byte string) to
  `0xb8 0x40` (RLP long-string, 64 bytes follow). Outer list length
  prefix updates from `0xf8 0x52` to `0xf8 0x63`.
- `GET_PUBLIC_KEY` (P2=0) response grows from 49 bytes (`Q` + 48) to
  65 bytes (`Q` + 64).
- Test fixtures regenerated; Speculos PNG snapshots TBD on next build.

### Notes

- `address_from_bip32_path()` algorithm is unchanged: SHAKE-256 over
  `descriptor || public_key`, output `ADDRESS_SIZE` bytes. Migration is
  purely a constant change; cross-implementation parity with go-qrllib
  and wallet.js verified for the canonical test vector
  (`pk = 0x42 × 2592`, `desc = [1,0,0]` → `Qf9e32f504...e6a040`).

## [2.2.0] - 2026-04-25

### Changed

- Migrated QRL address format from 20-byte to 48-byte for ML-DSA-87
  post-quantum addressing (interim step; superseded by 3.0.0).
- Renamed remaining boilerplate references to QRL.
- Accept Ledger extra params field.

## [2.1.0] - 2023-10-06

### Changed

- Improving the settings use case in order to be able to use app settings parameters stored in NVM
- add a NBGL use case choice when a setting switch is toggled

## [2.0.0] - 2023-07-10

### Added

- Stax porting
- Extensive CI, including mandatory `guidelines_enforcer.yml`
- Extensive `README.md` to modify/compile/test the application on most OS (Linux, MacOS, Windows)
- Extensive `Ragger` tests

### Changed

- Simplified `Makefile` (complexity delegated to the SDK's `Makefile.standard_app`)
- Simplified overall code (moved into the SDK)
- Improving several UI flows to fit Ledger UI guidelines
- Removing `TRY`/`CATCH` usage (using `_no_throw` SDK functions)
- Cleaning unnecessary resources (moved into the SDK)

### Fixed

- Multiple minor lint, prototype or misspell fixes

## [1.0.1] - 2021-01-11

### Fix

- Missing header includes

## [1.0.0] - 2020-11-19

### Added

- Initial commit with the brand new Boilerplate application
