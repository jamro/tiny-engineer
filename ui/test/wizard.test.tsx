import { screen, waitFor, within } from "@testing-library/preact";
import { describe, expect, it } from "vitest";
import { fakeRobot, renderApp } from "./robot";

const angle = () => screen.getByTestId("calib-angle").textContent;
const stepLabel = () => screen.getByText(/^Step \d of 5/).textContent;
const statusText = () => screen.queryByRole("status")?.textContent;

async function openWizard(settings = {}) {
  const robot = fakeRobot({ provisioning: true, settings });
  const app = renderApp("/");

  await screen.findByRole("heading", { name: "Tiny Engineer setup" });

  return { robot, ...app };
}

async function skipCalibration(settings = {}) {
  const ctx = await openWizard(settings);

  await ctx.click("Skip calibration");
  await waitFor(() => expect(stepLabel()).toBe("Step 2 of 5 · Screen"));

  return ctx;
}

async function startGuided(settings = {}) {
  const ctx = await openWizard(settings);

  await ctx.click("Calibrate ranges");
  await screen.findByRole("heading", { name: "Body · center" });

  return ctx;
}

describe("setup wizard", () => {
  it("takes over the config page while the robot is provisioning", async () => {
    fakeRobot({ provisioning: true });
    renderApp("/");

    await screen.findByRole("heading", { name: "Tiny Engineer setup" });
    expect(screen.queryByRole("navigation")).toBeNull();
    expect(stepLabel()).toBe("Step 1 of 5 · Servos");
  });

  it("opens the servos hub before assembly or guided calibration", async () => {
    await openWizard();

    expect(screen.getByRole("button", { name: "Calibrate ranges" })).toBeTruthy();
    expect(screen.getByRole("button", { name: "Skip calibration" })).toBeTruthy();
    expect(screen.getByRole("button", { name: "Assembly: 90° + identify" })).toBeTruthy();
    expect(screen.queryByRole("heading", { name: "Body · center" })).toBeNull();
    expect(screen.queryByRole("button", { name: "Move all to 90°" })).toBeNull();
  });

  it("skips calibration, saves loaded ranges, and advances to Screen", async () => {
    const { robot } = await skipCalibration();

    expect(robot.lastPost("/settings")?.json).toEqual({
      servo_mins: [80, 80, 80, 80, 80],
      servo_maxs: [100, 100, 100, 100, 100],
    });
    await waitFor(() => expect(robot.lastPost("/setup/oled")?.json).toEqual({ rotate_180: false }));
  });

  it("centers all servos and identifies one from assembly, then returns to the hub", async () => {
    const { robot, click } = await openWizard();

    await click("Assembly: 90° + identify");
    await click("Move all to 90°");

    await waitFor(() => expect(robot.lastPost("/setup/servo")?.json).toEqual({ all: 90 }));

    await click("Identify Head");

    await waitFor(() =>
      expect(
        robot
          .sent("POST", "/setup/servo")
          .filter((request) => (request.json as { index?: number }).index === 0)
          .map((request) => (request.json as { angle: number }).angle),
      ).toEqual([93, 87, 90]),
    );

    await click("Back to hub");

    expect(screen.getByRole("button", { name: "Calibrate ranges" })).toBeTruthy();
    expect(screen.queryByRole("heading", { name: "Body · center" })).toBeNull();
  });

  it("offers no control that moves a servo from the calibration bar", async () => {
    await startGuided();
    await screen.findByTestId("calib-angle");

    const guide = document.querySelector(".calib-guide");

    expect(guide?.getAttribute("aria-hidden")).toBe("true");
    expect(guide?.querySelector("input, button")).toBeNull();
  });

  it("nudges center and min with the same buttons, mirrors max about the session center", async () => {
    const { robot, click } = await startGuided();

    // Stock 80–100 → default park at midpoint 90°.
    await waitFor(() => expect(angle()).toBe("90"));
    await click("−5°");
    await waitFor(() => expect(angle()).toBe("85"));
    await click("Set center (85°)");

    await screen.findByRole("heading", { name: "Body · minimum" });
    // Park at current min 80°; nudge to 55°, mirror max = 115.
    await waitFor(() => expect(angle()).toBe("80"));
    await click("−10°");
    await click("−10°");
    await click("−5°");
    await waitFor(() => expect(angle()).toBe("55"));
    await click("Set min · demo range");

    await waitFor(() =>
      expect(screen.getByRole("heading", { name: "Neck · center" })).toBeTruthy(),
    );

    const bodyMoves = robot
      .sent("POST", "/setup/servo")
      .filter((request) => (request.json as { index?: number }).index === 4)
      .map((request) => (request.json as { angle: number }).angle);

    // Park 90; nudge 85; min park 80 → 55; demo max 115; return to session center 85.
    expect(bodyMoves).toEqual(expect.arrayContaining([90, 85, 80, 55, 115, 85]));
  });

  it("restores the shown angle when a move fails", async () => {
    const { robot, click } = await startGuided();

    await click("Set center (90°)");
    await waitFor(() => expect(angle()).toBe("80"));

    robot.failing.add("/setup/servo");
    await click("+5°");

    await waitFor(() => expect(statusText()).toBe("robot said no"));
    expect(angle()).toBe("80");
  });

  it("rejects a body min at or above the session center", async () => {
    const { click } = await startGuided();

    await click("Set center (90°)");
    await waitFor(() => expect(angle()).toBe("80"));

    await click("+10°");
    await waitFor(() => expect(angle()).toBe("90"));
    await click("Set min · demo range");

    expect(statusText()).toBe("Min must be below the center so the mirrored max stays valid.");
    expect(screen.getByRole("heading", { name: "Body · minimum" })).toBeTruthy();
  });

  it("returns to the hub when backing from the first guided step", async () => {
    const { click } = await startGuided();

    await click("Back");

    expect(screen.getByRole("button", { name: "Calibrate ranges" })).toBeTruthy();
  });

  it("has no joint tabs during guided calibration", async () => {
    await startGuided();

    expect(document.querySelector(".joint-tabs")).toBeNull();
    expect(screen.queryByRole("button", { name: "Neck" })).toBeNull();
  });

  it("sets hand bounds in docs polarity order and saves mins/maxs only on finish", async () => {
    const { robot, click } = await startGuided();

    // Body/Neck/Head mid 90; min opens at saved 80 → nudge to 60 → max 120
    await waitFor(() => expect(angle()).toBe("90"));
    await click("Set center (90°)");
    await waitFor(() => expect(angle()).toBe("80"));
    await click("−10°");
    await click("−10°");
    await click("Set min · demo range");

    await screen.findByRole("heading", { name: "Neck · center" });
    await waitFor(() => expect(angle()).toBe("90"));
    await click("Set center (90°)");
    await waitFor(() => expect(angle()).toBe("80"));
    await click("−10°");
    await click("−10°");
    await click("Set min · demo range");

    await screen.findByRole("heading", { name: "Head · center" });
    await waitFor(() => expect(angle()).toBe("90"));
    await click("Set center (90°)");
    await waitFor(() => expect(angle()).toBe("80"));
    await click("−10°");
    await click("−10°");
    await click("Set min · demo range");

    // Right min (keyboard) — park at saved min 80; nudge to 35
    await screen.findByRole("heading", { name: "Right hand · keyboard" });
    await waitFor(() => expect(angle()).toBe("80"));
    await click("−10°");
    await click("−10°");
    await click("−10°");
    await click("−10°");
    await click("−5°");
    await click("Set min (35°)");

    // Left max (keyboard) — park at saved max 100; nudge to 135
    await screen.findByRole("heading", { name: "Left hand · keyboard" });
    await waitFor(() => expect(angle()).toBe("100"));
    await click("+10°");
    await click("+10°");
    await click("+10°");
    await click("+5°");
    await click("Set max (135°)");

    // Left min (upper) — park at saved min 80; nudge to 45
    await screen.findByRole("heading", { name: "Left hand · upper" });
    await waitFor(() => expect(angle()).toBe("80"));
    await click("−10°");
    await click("−10°");
    await click("−10°");
    await click("−5°");
    await click("Set min (45°)");

    // Right max (upper) — park at current max 100; nudge to 125
    await screen.findByRole("heading", { name: "Right hand · upper" });
    await waitFor(() => expect(angle()).toBe("100"));
    await click("+10°");
    await click("+10°");
    await click("+5°");
    await click("Finish · save ranges");

    await waitFor(() => expect(stepLabel()).toBe("Step 2 of 5 · Screen"));

    const saved = robot.lastPost("/settings")?.json as Record<string, unknown>;

    expect(saved).toEqual({
      servo_mins: [60, 60, 45, 35, 60],
      servo_maxs: [120, 120, 135, 125, 120],
    });
    expect(saved).not.toHaveProperty("servo_centers");
    expect(Object.keys(saved)).toEqual(["servo_mins", "servo_maxs"]);
  });

  it("restores the setup screen when leaving the screen step", async () => {
    const { robot, click } = await skipCalibration();

    await waitFor(() => expect(robot.lastPost("/setup/oled")?.json).toBeDefined());
    await click("Back");

    await waitFor(() => expect(robot.lastPost("/setup/oled")?.json).toBeUndefined());
    expect(stepLabel()).toBe("Step 1 of 5 · Servos");
    expect(screen.getByRole("button", { name: "Calibrate ranges" })).toBeTruthy();
  });

  it("saves the hostname, then tests the new network ahead of the saved ones", async () => {
    const { robot, user, click } = await skipCalibration({ wifi_networks: ["old"] });

    for (const next of ["Next: RGB mapping", "Next: Speaker", "Next: Connection"]) {
      await click(next);
    }

    await user.type(await screen.findByLabelText("Network name (SSID)"), "home");
    await user.type(screen.getByLabelText("Password"), "secret");
    await user.clear(screen.getByLabelText("Hostname"));
    await user.type(screen.getByLabelText("Hostname"), "desk-bot");

    const writesBefore = robot.requests.length;

    await click("Connect to WiFi");

    await screen.findByRole("navigation");

    const writes = robot.requests.slice(writesBefore).filter((request) => request.method !== "GET");

    expect(writes.map((request) => `${request.method} ${request.path}`)).toEqual([
      "POST /settings",
      "PUT /settings/wifi",
    ]);
    expect(writes[0].json).toEqual({ hostname: "desk-bot" });
    expect(writes[1].json).toEqual([{ ssid: "home", password: "secret" }, { ssid: "old" }]);
    expect(statusText()).toContain("WiFi connected");
  });

  it("saves USB-only mode from the connection step", async () => {
    const { robot, click, user } = await skipCalibration({
      wifi_networks: [],
      link_mode: "",
      wifi_configured: false,
    });

    await click("Next: RGB mapping");
    await click("Next: Speaker");
    await click("Next: Connection");

    expect(stepLabel()).toBe("Step 5 of 5 · Connection");

    await user.click(screen.getByRole("radio", { name: /USB connection only/i }));
    await click("Use USB only");

    await waitFor(() => expect(robot.lastPost("/settings")?.json).toEqual({ link_mode: "usb" }));
    expect(statusText()).toMatch(/USB-only/i);
    expect(statusText()).toMatch(/serial/i);
    expect(statusText()).not.toMatch(/power-cycle/i);
  });

  it("blocks a color mapping that uses a color twice", async () => {
    const { robot, click, user } = await skipCalibration();

    await click("Next: RGB mapping");
    await click("Change color mapping");

    expect(stepLabel()).toBe("Step 3 of 5 · RGB mapping");

    const byte = (name: string) => within(screen.getByRole("group", { name }));
    const next = () => screen.getByRole<HTMLButtonElement>("button", { name: "Next: Speaker" });

    await user.click(byte("First byte").getByRole("button", { name: "Red" }));

    expect(next().disabled).toBe(true);

    await user.click(byte("Second byte").getByRole("button", { name: "Green" }));

    expect(next().disabled).toBe(false);

    await click("Next: Speaker");

    await waitFor(() => expect(stepLabel()).toBe("Step 4 of 5 · Speaker"));
    expect(robot.lastPost("/settings")?.json).toEqual({ rgb_order: "RGB" });
  });
});
