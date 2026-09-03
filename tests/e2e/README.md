# End-to-end devnet test

Proves a device-signed transaction is accepted by a real QRL v2.0 node:
fund the device account on a private devnet, sign a transfer on Speculos,
verify the signature independently with wallet.js, broadcast it, and watch it
mine.

## Prerequisites

- Docker, Kurtosis CLI, and the four devnet images built per the
  `cyyber/qrl-package` instructions (`qrledger/qrysm:qrl-genesis-generator-latest`,
  `qrledger/go-qrl:stable`, `qrledger/qrysm:validator-latest`,
  `qrledger/qrysm:beacon-chain-latest`)
- Node.js 22+, then `npm install` in this directory (pulls
  `@theqrl/wallet.js` and `@noble/hashes` from npm)
- The app built for nanos+ and a dev-tools image with the packages from
  `tests/requirements.txt` installed

## Runbook

```sh
# 1. Devnet up; find the EL RPC port mapped from 8545 (e.g. 32770)
kurtosis run --enclave my-testnet github.com/cyyber/qrl-package
kurtosis enclave inspect my-testnet
RPC=http://127.0.0.1:<port>

# 2. Get the device address once (any prior run, or the pytest below prints it)
#    then fund it from prefunded genesis account #0
node fund.mjs $RPC <device_addr_128hex> 1000000000000000000

# 3. Build the unsigned preimage for the device transaction
#    (fresh account => nonce 0; recipient e.g. prefunded #0)
node mkpreimage.mjs 0 <recipient_addr_128hex> 50000000000000000 /tmp/e2e

# 4. Sign on Speculos (from the repo's tests/ dir, dev-tools image)
docker run --rm -v "$PWD/../..:/app" -v /tmp/e2e:/e2e -w /app/tests <image> \
  bash -c "E2E_PREIMAGE=$(cat /tmp/e2e/device_preimage.hex) \
           E2E_OUT=/e2e/device_out.json \
           pytest test_e2e_device_sign.py --device nanosp -q"

# 5. Verify + broadcast + await receipt
node broadcast.mjs $RPC /tmp/e2e/device_params.json /tmp/e2e/device_out.json
```

Teardown: `kurtosis enclave rm -f my-testnet`.

## Files

- `qrltx.mjs`: RLP encode (unsigned 11-field preimage and signed 13-field
  envelope, go-qrl `DynamicFeeTx` order), legacy-Keccak sighash, `qrl_*`
  JSON-RPC helpers. `CHAIN_ID` env overrides the default devnet id 3151908.
- `fund.mjs`: sends value from qrl-package prefunded account #0 (dev-only,
  publicly known seed).
- `mkpreimage.mjs`: writes `device_params.json` + `device_preimage.hex`.
- `broadcast.mjs`: re-derives the sighash, verifies the device signature
  with wallet.js (refuses to broadcast otherwise), assembles the signed
  envelope, broadcasts, waits for the receipt.
- `../test_e2e_device_sign.py`: Ragger/Speculos harness; gated behind
  `E2E_PREIMAGE` so normal test runs skip it.
