import assert from "node:assert/strict";
import test from "node:test";

import { ACTION_CAP, createActionRunner } from "../src/daemon/actions.js";

/**
 * @returns {{ spawn: Function, children: Array<Record<string, unknown>> }}
 */
function fakeSpawn() {
  /** @type {Array<Record<string, any>>} */
  const children = [];
  /**
   * @param {string} program
   * @param {string[]} args
   * @param {Record<string, unknown>} opts
   */
  function spawn(program, args, opts) {
    /** @type {Record<string, Function[]>} */
    const handlers = {};
    const child = {
      pid: 100 + children.length,
      program,
      args,
      opts,
      stdinText: "",
      stdin: {
        /** @param {string} chunk */
        write(chunk) {
          child.stdinText += chunk;
        },
        end() {},
        on() {},
      },
      stderr: { on() {} },
      /**
       * @param {string} ev
       * @param {Function} fn
       */
      on(ev, fn) {
        (handlers[ev] ??= []).push(fn);
      },
      /**
       * @param {string} ev
       * @param {unknown[]} rest
       */
      emit(ev, ...rest) {
        for (const fn of handlers[ev] ?? []) fn(...rest);
      },
      kill() {},
    };
    children.push(child);
    return child;
  }
  return { spawn, children };
}

const rule = { id: 0, on: "button.click", run: ["/usr/bin/open", "-a", "Music"] };
const event = { v: 1, type: "button.click", source: "push-button", ms: 5 };

test("spawn is direct, stdin is the event, and the token is not copied", () => {
  const fake = fakeSpawn();
  const runner = createActionRunner({
    spawnImpl: fake.spawn,
    home: "/home/user",
    parentEnv: {
      PATH: "/usr/bin",
      HOME: "/elsewhere",
      TINY_ENGINEER_TOKEN: "secret-token",
      USER: "user",
    },
    setTimer: () => 0,
    clearTimer: () => {},
  });
  assert.equal(runner.start(rule, event), "started");
  const child = fake.children[0];
  assert.equal(child.program, "/usr/bin/open");
  assert.deepEqual(child.args, ["-a", "Music"]);
  assert.equal(child.opts.shell, false);
  assert.equal(child.opts.cwd, "/home/user");
  assert.equal(child.stdinText, `${JSON.stringify(event)}\n`);
  assert.equal(child.opts.env.TINY_ENGINEER_EVENT_TYPE, "button.click");
  assert.equal(child.opts.env.TINY_ENGINEER_EVENT_SOURCE, "push-button");
  assert.equal(child.opts.env.TINY_ENGINEER_EVENT_MS, "5");
  assert.equal(JSON.stringify(child.opts.env).includes("secret-token"), false);
  assert.equal(child.opts.env.TINY_ENGINEER_TOKEN, undefined);
});

test("a busy rule and the global cap skip", () => {
  const fake = fakeSpawn();
  const runner = createActionRunner({
    spawnImpl: fake.spawn,
    home: "/home/user",
    parentEnv: {},
    setTimer: () => 0,
    clearTimer: () => {},
  });
  assert.equal(runner.start(rule, event), "started");
  assert.equal(runner.start(rule, event), "skipped");
  assert.equal(runner.lastAction.kind, "skipped");
  assert.equal(fake.children.length, 1);

  const many = fakeSpawn();
  const capped = createActionRunner({
    spawnImpl: many.spawn,
    home: "/home/user",
    parentEnv: {},
    setTimer: () => 0,
    clearTimer: () => {},
  });
  for (let i = 0; i < ACTION_CAP; i += 1) {
    assert.equal(capped.start({ id: i, on: "button.click", run: ["/bin/true"] }, event), "started");
  }
  assert.equal(capped.start({ id: ACTION_CAP, on: "button.click", run: ["/bin/true"] }, event), "skipped");
  assert.equal(many.children.length, ACTION_CAP);
});

test("timeout kills the child and a custom timeout is honored", () => {
  /** @type {Function | null} */
  let pending = null;
  /** @type {number | null} */
  let delayMs = null;
  /** @type {number[]} */
  const killed = [];
  const fake = fakeSpawn();
  const runner = createActionRunner({
    spawnImpl: fake.spawn,
    home: "/home/user",
    parentEnv: {},
    setTimer: (fn, ms) => {
      pending = fn;
      delayMs = ms;
      return 1;
    },
    clearTimer: () => {},
    killTree: (pid) => {
      killed.push(pid);
    },
  });
  runner.start({ id: 0, on: "button.click", timeoutMs: 1000, run: ["/bin/sleep"] }, event);
  assert.equal(delayMs, 1000);
  pending?.();
  assert.deepEqual(killed, [100]);
  assert.equal(runner.lastAction.kind, "timeout");

  pending = null;
  delayMs = null;
  const defaults = fakeSpawn();
  const defaultRunner = createActionRunner({
    spawnImpl: defaults.spawn,
    home: "/home/user",
    parentEnv: {},
    setTimer: (fn, ms) => {
      pending = fn;
      delayMs = ms;
      return 1;
    },
    clearTimer: () => {},
    killTree: () => {},
  });
  defaultRunner.start(rule, event);
  assert.equal(delayMs, 30_000);
});
