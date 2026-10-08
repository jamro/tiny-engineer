import test from "node:test";
import assert from "node:assert/strict";
import {
  parseAnimArgs,
  parseCommonArgs,
  parsePlayArgs,
  stripGlobalFlags,
} from "../src/util/args.js";

test("parseCommonArgs extracts --url and rest", () => {
  assert.deepEqual(parseCommonArgs(["--url", "http://x", "cursor"]), {
    help: false,
    quiet: false,
    url: "http://x",
    rest: ["cursor"],
  });
  assert.deepEqual(parseCommonArgs(["--url=http://y", "cursor", "extra"]), {
    help: false,
    quiet: false,
    url: "http://y",
    rest: ["cursor", "extra"],
  });
});

test("parseCommonArgs help and missing --url value", () => {
  assert.equal(parseCommonArgs(["-h"]).help, true);
  assert.equal(parseCommonArgs(["--url"]).error, "--url requires a value");
});

test("parseCommonArgs accepts quiet before or after ide id", () => {
  assert.equal(parseCommonArgs(["cursor", "-q"]).quiet, true);
  assert.deepEqual(parseCommonArgs(["cursor", "-q"]).rest, ["cursor"]);
  assert.equal(parseCommonArgs(["-q", "cursor"]).quiet, true);
  assert.deepEqual(parseCommonArgs(["-q", "cursor"]).rest, ["cursor"]);
  assert.equal(parseCommonArgs(["--silent", "--url", "http://x", "cursor"]).quiet, true);
});

test("parseAnimArgs requires name", () => {
  assert.equal(parseAnimArgs([]).error, "anim needs a pose name");
  assert.deepEqual(parseAnimArgs(["ring", "--url", "http://x"]), {
    help: false,
    quiet: false,
    name: "ring",
    url: "http://x",
  });
});

test("parseAnimArgs accepts -q after name", () => {
  assert.equal(parseAnimArgs(["ring", "-q"]).quiet, true);
  assert.equal(parseAnimArgs(["--quiet", "ring"]).quiet, true);
});

test("parsePlayArgs requires wav path", () => {
  assert.equal(parsePlayArgs([]).error, "play needs a WAV file");
  assert.deepEqual(parsePlayArgs(["clip.wav", "--name", "thinking"]), {
    wavPath: "clip.wav",
    name: "thinking",
    quiet: false,
  });
});

test("parsePlayArgs accepts --silent", () => {
  assert.equal(parsePlayArgs(["clip.wav", "--silent"]).quiet, true);
});

test("stripGlobalFlags pulls leading quiet", () => {
  assert.deepEqual(stripGlobalFlags(["-q", "anim", "ring"]), {
    quiet: true,
    rest: ["anim", "ring"],
  });
  assert.deepEqual(stripGlobalFlags(["--quiet", "--silent", "play", "a.wav"]), {
    quiet: true,
    rest: ["play", "a.wav"],
  });
  assert.deepEqual(stripGlobalFlags(["anim", "ring"]), {
    quiet: false,
    rest: ["anim", "ring"],
  });
});
