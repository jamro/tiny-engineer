import { cursor } from "./cursor/index.js";

/** @type {Map<string, import("./types.js").Integration>} */
const byId = new Map([[cursor.id, cursor]]);

/**
 * @param {string} id
 * @returns {import("./types.js").Integration | undefined}
 */
export function get(id) {
  return byId.get(id);
}

/**
 * @returns {string[]}
 */
export function list() {
  return [...byId.keys()];
}

/**
 * @returns {import("./types.js").Integration[]}
 */
export function all() {
  return [...byId.values()];
}
