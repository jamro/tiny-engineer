import { cleanup, render, screen, waitFor } from "@testing-library/preact";
import userEvent from "@testing-library/user-event";
import { afterEach, beforeEach, expect, vi } from "vitest";
import type { Settings, WifiNetwork } from "../src/api/types";
import { App } from "../src/App";

const DEFAULT_SETTINGS: Settings = {
  ok: true,
  hostname: "tiny-engineer",
  sleep_timeout: 10,
  continuous_timeout: 5,
  volume: 70,
  welcome: true,
  serial_log: false,
  loading: "progress",
  eyes_style: "classic",
  access_token_set: false,
  link_mode: "wifi",
  wifi_configured: true,
  wifi_networks: ["home"],
  servo_mins: [80, 80, 80, 80, 80],
  servo_maxs: [100, 100, 100, 100, 100],
  rgb_order: "GRB",
  oled_rotate_180: false,
};

export interface RecordedRequest {
  method: string;
  path: string;
  params: Partial<Record<string, string>>;
  authorization?: string;
  json?: unknown;
}

function applySettingsPatch(
  settings: Settings,
  { access_token: accessToken, ...patch }: Partial<Settings> & { access_token?: string },
): Settings {
  return {
    ...settings,
    ...patch,
    access_token_set: accessToken === undefined ? settings.access_token_set : accessToken !== "",
    reboot_required: (patch.hostname ?? settings.hostname) !== settings.hostname,
  };
}

// Stands in for the firmware HTTP API and records every request the UI makes.
interface FakeRobotOptions {
  authRequired?: boolean;
  token?: string;
  provisioning?: boolean;
  settings?: Partial<Settings>;
}

export function fakeRobot({
  authRequired = false,
  token = "",
  provisioning = false,
  settings = {},
}: FakeRobotOptions = {}) {
  const robot = {
    requests: [] as RecordedRequest[],
    failing: new Set<string>(),
    provisioning,
    settings: { ...DEFAULT_SETTINGS, wifi_configured: !provisioning, ...settings },
    // Runs before each reply, so a test can look at the page mid-request.
    onRequest: undefined as ((request: RecordedRequest) => void) | undefined,
    sent(method: string, path: string) {
      return this.requests.filter((request) => request.method === method && request.path === path);
    },
    lastPost(path: string) {
      return this.sent("POST", path).at(-1);
    },
  };

  vi.stubGlobal(
    "fetch",
    vi.fn((input: string, options: RequestInit = {}) => {
      const url = new URL(input, "http://robot.test");
      const headers = (options.headers ?? {}) as Record<string, string>;
      const request: RecordedRequest = {
        method: options.method ?? "GET",
        path: url.pathname,
        params: Object.fromEntries(url.searchParams),
        authorization: headers.Authorization,
        json: typeof options.body === "string" ? JSON.parse(options.body) : undefined,
      };
      const json = (body: unknown, status = 200) =>
        Promise.resolve(Response.json(body, { status }));

      robot.requests.push(request);
      robot.onRequest?.(request);

      if (request.path === "/auth") {
        return json({
          ok: true,
          required: authRequired,
          version: "test",
          wifi_configured: robot.settings.wifi_configured,
          link_mode: robot.settings.link_mode,
          provisioning: robot.provisioning,
        });
      }

      if (authRequired && request.authorization !== `Bearer ${token}`) {
        return json({ ok: false, error: "unauthorized" }, 401);
      }

      if (robot.failing.has(request.path)) {
        return json({ ok: false, error: "robot said no" }, 400);
      }

      if (request.path === "/health") {
        return json({
          ok: true,
          version: "test",
          provisioning: robot.provisioning,
          wifi_configured: robot.settings.wifi_configured,
        });
      }

      if (request.path === "/settings" && request.method === "POST") {
        robot.settings = applySettingsPatch(robot.settings, request.json as Partial<Settings>);
      }

      if (request.path === "/settings/wifi" && request.method === "PUT") {
        robot.settings.wifi_networks = (request.json as WifiNetwork[]).map(({ ssid }) => ssid);
        robot.settings.wifi_configured = true;
        robot.settings.link_mode = "wifi";

        // A tested network ends setup AP mode.
        if (robot.provisioning) {
          robot.provisioning = false;

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
    }),
  );

  return robot;
}

beforeEach(() => sessionStorage.clear());
afterEach(cleanup);

export function renderApp(path = "/") {
  window.history.replaceState({}, "", path);
  render(<App />);

  const user = userEvent.setup();

  return {
    user,
    // Waits until the button is enabled, so a click never lands while the last action runs.
    click: async (name: string | RegExp) => {
      const button = await screen.findByRole<HTMLButtonElement>("button", { name });

      await waitFor(() => expect(button.disabled).toBe(false));
      await user.click(button);
    },
  };
}
