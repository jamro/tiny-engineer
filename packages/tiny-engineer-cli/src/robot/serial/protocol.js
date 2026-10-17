export const SERIAL_PROTO_VERSION = 1;
export const SERIAL_HEADER_SIZE = 7;

/** @enum {number} */
export const SerialMsgType = {
  Req: 1,
  Res: 2,
  Log: 3,
  Evt: 4,
  Stream: 5,
  Sync: 6,
};

export const SerialStreamKind = {
  Data: 1,
  Credit: 2,
  End: 3,
  Error: 4,
};

export const SERIAL_STREAM_HDR_SIZE = 4;

/**
 * @param {number} streamId
 * @param {number} kind
 * @param {number} seq
 * @param {Buffer | Uint8Array} [data]
 * @returns {Buffer}
 */
export function packStreamPayload(streamId, kind, seq, data = Buffer.alloc(0)) {
  const body = Buffer.from(data);
  const out = Buffer.alloc(SERIAL_STREAM_HDR_SIZE + body.length);
  out[0] = streamId & 0xff;
  out[1] = kind & 0xff;
  out.writeUInt16LE(seq & 0xffff, 2);
  body.copy(out, SERIAL_STREAM_HDR_SIZE);
  return out;
}

/**
 * @param {number} type
 * @param {number} id
 * @param {Buffer | Uint8Array | string} payload
 * @param {{ ver?: number, flags?: number }} [opts]
 * @returns {Buffer}
 */
export function packMessage(type, id, payload, { ver = SERIAL_PROTO_VERSION, flags = 0 } = {}) {
  const body = typeof payload === "string" ? Buffer.from(payload, "utf8") : Buffer.from(payload);
  const out = Buffer.alloc(SERIAL_HEADER_SIZE + body.length);
  out[0] = type;
  out[1] = ver;
  out.writeUInt16LE(id & 0xffff, 2);
  out[4] = flags;
  out.writeUInt16LE(body.length, 5);
  body.copy(out, SERIAL_HEADER_SIZE);
  return out;
}

/**
 * @param {Buffer | Uint8Array} data
 * @returns {{ type: number, ver: number, id: number, flags: number, len: number, payload: Buffer } | null}
 */
export function parseMessage(data) {
  if (!data || data.length < SERIAL_HEADER_SIZE) return null;
  const type = data[0];
  const ver = data[1];
  const id = data.readUInt16LE(2);
  const flags = data[4];
  const len = data.readUInt16LE(5);
  if (data.length < SERIAL_HEADER_SIZE + len) return null;
  return {
    type,
    ver,
    id,
    flags,
    len,
    payload: Buffer.from(data.subarray(SERIAL_HEADER_SIZE, SERIAL_HEADER_SIZE + len)),
  };
}
