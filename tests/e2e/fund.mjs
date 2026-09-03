// Fund an address from prefunded devnet account #0 (cyyber/qrl-package genesis).
// usage: node fund.mjs <rpc_url> <to_addr_hex_128> <value_wei>
import { ExtendedSeed, MLDSA87 } from '@theqrl/wallet.js';
import { CHAIN_ID, toBytes, toHex, encodeUnsignedPreimage, encodeSignedTx, sighash, rpc, waitReceipt } from './qrltx.mjs';

const [rpcUrl, toHexArg, valueArg] = process.argv.slice(2);
if (!valueArg) {
  console.error('usage: node fund.mjs <rpc_url> <to_addr_hex_128> <value_wei>');
  process.exit(1);
}

// Dev-only key, publicly known (qrl-package genesis_constants account #0).
const SEED0 = '0100002fbf2a7d031dbd37936d694d25e3e9be2e25b5270e5753f8b09ae189fd8a8d5de868c8cfc49683476eccc0d5a8aeaf4b';

const wallet = MLDSA87.newWalletFromExtendedSeed(ExtendedSeed.from(SEED0));
const fromAddr = wallet.getAddressStr();
console.log('from:', fromAddr);

const nonceHex = await rpc(rpcUrl, 'qrl_getTransactionCount', [fromAddr, 'pending']);
const params = {
  chainId: CHAIN_ID,
  nonce: BigInt(nonceHex),
  tip: 1_000_000_000n,
  feeCap: 10_000_000_000n,
  gas: 500_000n,
  to: toBytes(toHexArg),
  value: BigInt(valueArg),
  data: new Uint8Array(0),
};
const pre = encodeUnsignedPreimage(params);
const h = sighash(pre);
console.log('sighash:', toHex(h));

const sig = wallet.sign(h);
const pk = wallet.getPK();
const raw = encodeSignedTx(params, sig, pk);
const txHash = await rpc(rpcUrl, 'qrl_sendRawTransaction', [toHex(raw)]);
console.log('tx:', txHash);
const rcpt = await waitReceipt(rpcUrl, txHash);
console.log('status:', rcpt.status, 'block:', rcpt.blockNumber);
const bal = await rpc(rpcUrl, 'qrl_getBalance', ['Q' + toHexArg.replace(/^0x/, ''), 'latest']);
console.log('recipient balance:', BigInt(bal).toString());
