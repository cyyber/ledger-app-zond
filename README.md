# QRL v2.0 Ledger Application

Ledger hardware wallet application for QRL v2.0, the post-quantum blockchain.
Signs QRL v2.0 dynamic-fee transactions with **ML-DSA-87** (FIPS 204) using
keys derived on-device. Supports Nano S+, Nano X, Stax, and Flex.

- **Signature scheme:** ML-DSA-87 with 2,592-byte public keys, 4,627-byte
  hedged signatures, and a descriptor-bound 8-byte signing context
- **Addresses:** 64-byte `SHAKE256(descriptor ‖ pk)` with checksummed
  `Q` + 128-hex display
- **Derivation:** BIP-32 path `m/44'/238'/account'/change/index` (see the
  QRL-Ledger-v1 wallet profile for how this differs from software wallets)

## Documentation

- [`APP_SPECIFICATION.md`](APP_SPECIFICATION.md): APDU interface
- [`doc/TRANSACTION.md`](doc/TRANSACTION.md): transaction preimage and
  signing format

## Build

Builds run in Ledger's dev-tools container:

```sh
docker run --rm -v "$PWD:/app" -w /app \
    ghcr.io/ledgerhq/ledger-app-builder/ledger-app-dev-tools:latest \
    bash -c 'BOLOS_SDK=$NANOSP_SDK make -j'
```

Substitute `$NANOX_SDK`, `$STAX_SDK`, or `$FLEX_SDK` for the other targets.

## Test

Functional tests run with [Ragger](https://github.com/LedgerHQ/ragger) on
Speculos. The dev-tools image needs the test dependencies added once:

```sh
docker run --rm -v "$PWD:/app" -w /app/tests <image-with-test-deps> \
    pytest --device nanosp
```

(Add the packages from `tests/requirements.txt` to the dev-tools image's
`/opt/venv` once with `pip install -r tests/requirements.txt`.)

Screen snapshots regenerate with `--golden_run` after intentional UI changes.

### End-to-end devnet test

[`tests/e2e/`](tests/e2e/README.md) contains a full runbook and scripts that
prove a device-signed transaction is accepted by a real QRL v2.0 node: spin up
a private Kurtosis devnet, fund the device account, sign on Speculos, verify
the signature independently with `@theqrl/wallet.js` (from npm, pinned), and
broadcast.

## Security model

Key material never leaves the device; derivation is rooted in the Ledger OS
BIP-39 seed. A wallet created by this app is recoverable only through a
Ledger device with the same recovery phrase running this app. It is
deliberately not restorable in QRL software wallets, and software backups
cannot restore it. The device verifies every signature it produces before
releasing it, and secret signing intermediates are wiped from NVM after each
operation.
