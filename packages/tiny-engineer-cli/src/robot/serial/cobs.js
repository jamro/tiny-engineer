/** @typedef {Uint8Array | Buffer} Bytes */

/**
 * COBS-encode a payload and append the 0x00 frame delimiter.
 * Compatible with firmware nanocobs framing.
 * @param {Bytes} payload
 * @returns {Buffer}
 */
export function cobsEncodeFrame(payload) {
  const out = [];
  let codeIndex = 0;
  out.push(0);
  let code = 1;

  const finishBlock = () => {
    out[codeIndex] = code;
    codeIndex = out.length;
    out.push(0);
    code = 1;
  };

  for (let i = 0; i < payload.length; i++) {
    const byte = payload[i];
    if (byte === 0) {
      finishBlock();
    } else {
      out.push(byte);
      code++;
      if (code === 0xff) finishBlock();
    }
  }

  out[codeIndex] = code;
  out.push(0);
  return Buffer.from(out);
}

/**
 * Decode one COBS frame that includes the trailing 0x00 delimiter.
 * @param {Bytes} frame
 * @returns {Buffer | null}
 */
export function cobsDecodeFrame(frame) {
  if (!frame || frame.length < 2 || frame[frame.length - 1] !== 0) {
    return null;
  }

  const encoded = frame.subarray(0, frame.length - 1);
  const out = [];
  let i = 0;

  while (i < encoded.length) {
    const code = encoded[i++];
    if (code === 0) return null;

    for (let j = 1; j < code; j++) {
      if (i >= encoded.length) return null;
      const byte = encoded[i++];
      if (byte === 0) return null;
      out.push(byte);
    }

    if (code < 0xff && i < encoded.length) {
      out.push(0);
    }
  }

  return Buffer.from(out);
}

/**
 * Incremental COBS frame decoder with resync on the next 0x00 after errors.
 */
export class CobsFrameDecoder {
  constructor() {
    /** @type {number[]} */
    this._buf = [];
    this._overflow = false;
    this._resyncing = false;
    /** @type {((payload: Buffer) => void) | null} */
    this._handler = null;
  }

  /**
   * @param {(payload: Buffer) => void} handler
   */
  setHandler(handler) {
    this._handler = handler;
  }

  reset() {
    this._buf = [];
    this._overflow = false;
    this._resyncing = false;
  }

  resyncing() {
    return this._resyncing;
  }

  /**
   * @param {Bytes} data
   */
  feed(data) {
    for (let i = 0; i < data.length; i++) {
      const byte = data[i];
      if (byte === 0) {
        this._onDelimiter();
        continue;
      }
      if (this._overflow) continue;
      if (this._buf.length >= 4096) {
        this._overflow = true;
        this._resyncing = true;
        continue;
      }
      this._buf.push(byte);
    }
  }

  _onDelimiter() {
    if (this._overflow || this._buf.length === 0) {
      const wasOverflow = this._overflow;
      this._buf = [];
      this._overflow = false;
      if (wasOverflow) this._resyncing = true;
      return;
    }

    const frame = Buffer.from([...this._buf, 0]);
    this._buf = [];
    this._overflow = false;

    const decoded = cobsDecodeFrame(frame);
    if (!decoded) {
      this._resyncing = true;
      return;
    }

    this._resyncing = false;
    this._handler?.(decoded);
  }
}
