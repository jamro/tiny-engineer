import { readFileSync } from "node:fs";
import { fileURLToPath } from "node:url";
import { vi } from "vitest";

const page = new DOMParser().parseFromString(
  readFileSync(fileURLToPath(import.meta.resolve("../index.html")), "utf8"),
  "text/html",
);

const DEFAULT_SETTINGS = {
  ok: true,
  hostname: "tiny-engineer",
  sleep_timeout: 10,
  continuous_timeout: 5,
  volume: 70,
  welcome: true,
  serial_log: false,
  servo_hold: false,
  wifi_enabled: true,
  loading: "progress",
  eyes_style: "classic",
  access_token_set: false,
  wifi_configured: true,
  wifi_networks: ["home"],
  servo_mins: [60, 40, 45, 35, 40],
  servo_maxs: [130, 130, 135, 125, 130],
  rgb_order: "GRB",
  oled_rotate_180: false,
};

function applySettingsParams(settings, params) {
  const next = { ...settings };

  for (const [key, value] of Object.entries(params)) {
    if (key === "servo_mins" || key === "servo_maxs") {
      next[key] = value.split(",").map(Number);
    } else if (key === "access_token") {
      next.access_token_set = value !== "";
    } else if (
      key === "welcome" ||
      key === "serial_log" ||
      key === "servo_hold" ||
      key === "wifi_enabled" ||
      key === "oled_rotate_180"
    ) {
      next[key] = value === "1";
    } else if (/^\d+$/.test(value)) {
      next[key] = Number(value);
    } else {
      next[key] = value;
    }
  }

  next.reboot_required =
    next.hostname !== settings.hostname ||
    Boolean(next.wifi_enabled) !== Boolean(settings.wifi_enabled);

  return next;
}

// Stands in for the firmware HTTP API and records every request the UI makes.
export function fakeRobot({
  authRequired = false,
  token = "",
  provisioning = false,
  settings = {},
} = {}) {
  const robot = {
    requests: [],
    failing: new Set(),
    settings: { ...DEFAULT_SETTINGS, wifi_configured: !provisioning, ...settings },
    posts(path) {
      return this.requests.filter((request) => request.method === "POST" && request.path === path);
    },
    lastPost(path) {
      return this.posts(path).at(-1);
    },
  };

  globalThis.fetch = vi.fn(async (input, options = {}) => {
    const url = new URL(input, "http://robot.test");
    const request = {
      method: options.method ?? "GET",
      path: url.pathname,
      params: Object.fromEntries(url.searchParams),
      authorization: options.headers?.Authorization,
      json: options.body === undefined ? undefined : JSON.parse(options.body),
    };
    const json = (body, status = 200) => new Response(JSON.stringify(body), { status });

    robot.requests.push(request);

    if (request.path === "/auth") {
      return json({ ok: true, required: authRequired });
    }

    if (authRequired && request.authorization !== `Bearer ${token}`) {
      return json({ ok: false, error: "unauthorized" }, 401);
    }

    if (robot.failing.has(request.path)) {
      return json({ ok: false, error: "robot said no" }, 400);
    }

    if (request.path === "/health") {
      return json({ ok: true, version: "test", provisioning, wifi_configured: !provisioning });
    }

    if (request.path === "/settings" && request.method === "POST") {
      robot.settings = applySettingsParams(robot.settings, request.params);
    }

    if (request.path === "/settings/wifi" && request.method === "PUT") {
      robot.settings.wifi_networks = request.json.map((network) => network.ssid);

      if (provisioning) {
        return json({ ...robot.settings, wifi_connect_success: true, wifi_ip: "192.168.1.10" });
      }
    }

    if (request.path.startsWith("/settings")) {
      return json(robot.settings);
    }

    if (request.path === "/anim") {
      return json({ ok: true, animation: request.params.name ?? "none" });
    }

    return json({ ok: true });
  });

  return robot;
}

// Lets the chain of awaited fetches and DOM updates behind one user action finish.
export async function settle() {
  for (let i = 0; i < 20; i++) {
    await new Promise((resolve) => setTimeout(resolve, 0));
  }
}

export async function loadApp(path = "/") {
  document.body.innerHTML = page.body.innerHTML;
  document.body.className = "";
  window.history.replaceState({}, "", path);
  vi.resetModules();
  await import("../src/main.js");
  await settle();
}

export const $ = (selector) => document.querySelector(selector);

export async function click(selector) {
  $(selector).click();
  await settle();
}

export async function submit(selector) {
  $(selector).dispatchEvent(new Event("submit", { cancelable: true }));
  await settle();
}

export function type(selector, value) {
  const field = $(selector);

  field.dispatchEvent(new Event("focus"));
  field.value = value;
  field.dispatchEvent(new Event("input"));
}
