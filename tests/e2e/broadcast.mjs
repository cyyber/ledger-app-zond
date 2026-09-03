// Assemble and broadcast a device-signed tx, verifying the signature first.
// usage: node broadcast.mjs <rpc_url> <device_params.json> <device_out.json>
import { readFileSync } from 'node:fs';
import { MLDSA87, newMLDSA87Descriptor } from '@theqrl/wallet.js';
import { toBytes, toHex, encodeUnsignedPreimage, encodeSignedTx, sighash, rpc, waitReceipt } from './qrltx.mjs';

const [rpcUrl, paramsFile, deviceFile] = process.argv.slice(2);
if (!deviceFile) {
  console.error('usage: node broadcast.mjs <rpc_url> <device_params.json> <device_out.json>');
  process.exit(1);
}
const p = JSON.parse(readFileSync(paramsFile, 'utf8'));
const d = JSON.parse(readFileSync(deviceFile, 'utf8'));

const params = {
  chainId: BigInt(p.chainId), nonce: BigInt(p.nonce), tip: BigInt(p.tip),
  feeCap: BigInt(p.feeCap), gas: BigInt(p.gas), to: toBytes(p.to),
  value: BigInt(p.value), data: new Uint8Array(0),
};
const pre = encodeUnsignedPreimage(params);
const h = sighash(pre);
console.log('sighash:', toHex(h));

const sig = toBytes(d.signature);
const pk = toBytes(d.public_key);

// Independent verification with wallet.js before broadcasting.
const desc = newMLDSA87Descriptor();
const ok = MLDSA87.verify(sig, h, pk, desc.toDescriptor ? desc.toDescriptor() : desc);
console.log('wallet.js verifies device signature:', ok);
if (!ok) throw new Error('device signature failed independent verification');

const raw = encodeSignedTx(params, sig, pk);
const txHash = await rpc(rpcUrl, 'qrl_sendRawTransaction', [toHex(raw)]);
console.log('tx:', txHash);
const rcpt = await waitReceipt(rpcUrl, txHash);
console.log('status:', rcpt.status, 'block:', rcpt.blockNumber, 'gasUsed:', rcpt.gasUsed);
