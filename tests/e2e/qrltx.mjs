// Shared helpers for QRL v2.0 devnet e2e: RLP, sighash, RPC.
import { keccak_256 } from '@noble/hashes/sha3.js';

export const CHAIN_ID = BigInt(process.env.CHAIN_ID || '3151908');

export function toBytes(hex) {
  if (hex.startsWith('0x') || hex.startsWith('0X')) hex = hex.slice(2);
  if (hex.length % 2) hex = '0' + hex;
  return Uint8Array.from(hex.match(/../g)?.map((b) => parseInt(b, 16)) ?? []);
}
export const toHex = (b) => '0x' + Array.from(b, (x) => x.toString(16).padStart(2, '0')).join('');

export function intToBytes(v) {
  v = BigInt(v);
  if (v === 0n) return new Uint8Array(0);
  let h = v.toString(16);
  if (h.length % 2) h = '0' + h;
  return toBytes(h);
}

function rlpBytes(b) {
  if (b.length === 1 && b[0] < 0x80) return b;
  if (b.length <= 55) return Uint8Array.from([0x80 + b.length, ...b]);
  const ln = intToBytes(b.length);
  return Uint8Array.from([0xb7 + ln.length, ...ln, ...b]);
}
function rlpList(items) {
  const payload = concat(...items);
  if (payload.length <= 55) return Uint8Array.from([0xc0 + payload.length, ...payload]);
  const ln = intToBytes(payload.length);
  return Uint8Array.from([0xf7 + ln.length, ...ln, ...payload]);
}
export function concat(...arrs) {
  const out = new Uint8Array(arrs.reduce((n, a) => n + a.length, 0));
  let o = 0;
  for (const a of arrs) { out.set(a, o); o += a.length; }
  return out;
}

// go-qrl DynamicFeeTx field order (unsigned sighash preimage, 11 fields).
export function encodeUnsignedPreimage({ chainId, nonce, tip, feeCap, gas, to, value, data }) {
  const fields = [
    rlpBytes(intToBytes(chainId)),
    rlpBytes(intToBytes(nonce)),
    rlpBytes(intToBytes(tip)),
    rlpBytes(intToBytes(feeCap)),
    rlpBytes(intToBytes(gas)),
    rlpBytes(to),                         // 64 bytes
    rlpBytes(intToBytes(value)),
    rlpBytes(data ?? new Uint8Array(0)),
    rlpList([]),                          // access_list
    rlpBytes(Uint8Array.from([1, 0, 0])), // descriptor
    rlpBytes(new Uint8Array(0)),          // extra_params
  ];
  return concat(Uint8Array.from([0x02]), rlpList(fields));
}

// Full signed envelope: preimage fields + signature + public key.
export function encodeSignedTx(params, signature, publicKey) {
  const fields = [
    rlpBytes(intToBytes(params.chainId)),
    rlpBytes(intToBytes(params.nonce)),
    rlpBytes(intToBytes(params.tip)),
    rlpBytes(intToBytes(params.feeCap)),
    rlpBytes(intToBytes(params.gas)),
    rlpBytes(params.to),
    rlpBytes(intToBytes(params.value)),
    rlpBytes(params.data ?? new Uint8Array(0)),
    rlpList([]),
    rlpBytes(Uint8Array.from([1, 0, 0])),
    rlpBytes(new Uint8Array(0)),
    rlpBytes(signature),                  // 4627 bytes
    rlpBytes(publicKey),                  // 2592 bytes
  ];
  return concat(Uint8Array.from([0x02]), rlpList(fields));
}

export const sighash = (preimage) => keccak_256(preimage);

export async function rpc(url, method, params) {
  const res = await fetch(url, {
    method: 'POST',
    headers: { 'content-type': 'application/json' },
    body: JSON.stringify({ jsonrpc: '2.0', id: 1, method, params }),
  });
  const j = await res.json();
  if (j.error) throw new Error(`${method}: ${JSON.stringify(j.error)}`);
  return j.result;
}

export async function waitReceipt(url, txHash, tries = 60) {
  for (let i = 0; i < tries; i++) {
    const r = await rpc(url, 'qrl_getTransactionReceipt', [txHash]);
    if (r) return r;
    await new Promise((s) => setTimeout(s, 2000));
  }
  throw new Error('timeout waiting for receipt ' + txHash);
}
