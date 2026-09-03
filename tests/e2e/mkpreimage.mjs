// Build the unsigned sighash preimage for a device transaction.
// usage: node mkpreimage.mjs <nonce> <to_addr_hex_128> <value_wei> [out_dir]
// Writes device_params.json + device_preimage.hex to out_dir (default: cwd).
import { mkdirSync, writeFileSync } from 'node:fs';
import { join } from 'node:path';
import { CHAIN_ID, toBytes, encodeUnsignedPreimage } from './qrltx.mjs';

const [nonceArg, toArg, valueArg, outDir = '.'] = process.argv.slice(2);
if (!valueArg) {
  console.error('usage: node mkpreimage.mjs <nonce> <to_addr_hex_128> <value_wei> [out_dir]');
  process.exit(1);
}

const paramsJson = {
  chainId: CHAIN_ID.toString(),
  nonce: nonceArg,
  tip: '1000000000',
  feeCap: '10000000000',
  gas: '500000',
  to: toArg.replace(/^0x/i, ''),
  value: valueArg,
};
const params = {
  chainId: CHAIN_ID,
  nonce: BigInt(nonceArg),
  tip: BigInt(paramsJson.tip),
  feeCap: BigInt(paramsJson.feeCap),
  gas: BigInt(paramsJson.gas),
  to: toBytes(paramsJson.to),
  value: BigInt(valueArg),
  data: new Uint8Array(0),
};
const pre = encodeUnsignedPreimage(params);
const hex = Array.from(pre, (b) => b.toString(16).padStart(2, '0')).join('');
mkdirSync(outDir, { recursive: true });
writeFileSync(join(outDir, 'device_params.json'), JSON.stringify(paramsJson, null, 2));
writeFileSync(join(outDir, 'device_preimage.hex'), hex);
console.log('preimage bytes:', pre.length);
console.log(hex);
