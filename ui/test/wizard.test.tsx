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

  await ctx.click("Skip");
  await waitFor(() => expect(stepLabel()).toBe("Step 2 of 5 · Screen"));

  return ctx;
}

async function startGuided(settings = {}) {
  const ctx = await openWizard(settings);

  await ctx.click("Calibrate servos");
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

    const calibrate = screen.getByRole("button", { name: "Calibrate servos" });
    const assembly = screen.getByRole("button", { name: "Assembly support" });
    const skip = screen.getByRole("button", { name: "Skip" });

    expect(calibrate.classList.contains("btn-primary")).toBe(true);
    expect(assembly.classList.contains("btn-secondary")).toBe(true);
    expect(skip.compareDocumentPosition(assembly) & Node.DOCUMENT_POSITION_PRECEDING).toBeTruthy();
    expect(document.querySelector(".setup-hub-when")).toBeNull();
    expect(
      within(calibrate).getByText("Set safe movement limits on the assembled robot"),
    ).toBeTruthy();
    expect(
      within(assembly).getByText("Center shafts and identify plugs before seating parts"),
    ).toBeTruthy();
    expect(within(skip).getByText("Keep the servo limits already loaded")).toBeTruthy();
    expect(screen.getByText(/80°–100°/)).toBeTruthy();
    expect(screen.queryByRole("heading", { name: "Body · center" })).toBeNull();
    expect(screen.queryByRole("button", { name: "Move all to 90°" })).toBeNull();
  });

  it("skips calibration, saves loaded ranges, and advances to Screen", async () => {
    const { robot, click } = await skipCalibration();

    expect(robot.lastPost("/settings")?.json).toEqual({
      servo_mins: [80, 80, 80, 80, 80],
      servo_maxs: [100, 100, 100, 100, 100],
    });
    await waitFor(() => expect(robot.lastPost("/setup/oled")?.json).toEqual({ rotate_180: false }));

    const rotate = screen.getByRole("button", { name: "Rotate 180°" });

    expect(rotate.classList.contains("btn-secondary")).toBe(true);
    expect(screen.getByTestId("screen-orientation").textContent).toBe("Current preview: Normal");

    await click("Rotate 180°");

    expect(screen.getByTestId("screen-orientation").textContent).toBe(
      "Current preview: Rotated 180°",
    );
  });

  it("centers all servos and identifies one from assembly, then returns to the hub", async () => {
    const { robot, click } = await openWizard();

    await click("Assembly support");

    for (const name of ["Head", "Neck", "Left hand", "Right hand", "Body"]) {
      expect(screen.getByRole("button", { name: `Identify ${name}` })).toHaveProperty(
        "disabled",
        true,
      );
    }

    await click("Move all to 90°");

    await waitFor(() => expect(robot.lastPost("/setup/servo")?.json).toEqual({ all: 90 }));

    expect(screen.getByRole("button", { name: "Identify Head" })).toHaveProperty("disabled", false);

    await click("Identify Head");

    await waitFor(() =>
      expect(
        robot
          .sent("POST", "/setup/servo")
          .filter((request) => (request.json as { index?: number }).index === 0)
          .map((request) => (request.json as { angle: number }).angle),
      ).toEqual([93, 87, 90]),
    );

    expect(screen.getByRole("button", { name: "✓ Head" }).classList.contains("done")).toBe(true);

    await click("Done · back to hub");

    expect(screen.getByRole("button", { name: "Calibrate servos" })).toBeTruthy();
    expect(screen.queryByRole("heading", { name: "Body · center" })).toBeNull();

    await click("Assembly support");
    expect(screen.getByRole("button", { name: "Identify Head" })).toHaveProperty("disabled", true);
  });

  it("keeps Identify disabled when move-all to 90° fails", async () => {
    const { robot, click } = await openWizard();

    await click("Assembly support");
    robot.failing.add("/setup/servo");
    await click("Move all to 90°");

    await waitFor(() => expect(statusText()).toBe("robot said no"));
    expect(screen.getByRole("button", { name: "Identify Head" })).toHaveProperty("disabled", true);
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

    expect(screen.getByTestId("calib-progress").textContent).toBe("Servo calibration · 1 of 10");
    expect(
      screen
        .getByRole("progressbar", { name: "Servo calibration progress" })
        .getAttribute("aria-valuenow"),
    ).toBe("1");
    expect(screen.getByRole("button", { name: "Confirm center" })).toBeTruthy();
    expect(document.querySelector(".setup-copy strong")?.textContent).toMatch(
      /centered in the chair/,
    );
    expect(document.querySelector(".calib-range-label")?.textContent).toBe("Saved range: 80°–100°");
    expect(document.querySelector(".calib-target")?.textContent).toBe(
      "Target: Body centered in the chair",
    );

    // Stock 80–100 → default park at midpoint 90°.
    await waitFor(() => expect(angle()).toBe("90"));
    await click("−5°");
    await waitFor(() => expect(angle()).toBe("85"));
    await click("Confirm center");

    await screen.findByRole("heading", { name: "Body · minimum" });
    // Park at current min 80°; nudge to 55°, mirror max = 115.
    await waitFor(() => expect(angle()).toBe("80"));
    await click("−10°");
    await click("−10°");
    await click("−5°");
    await waitFor(() => expect(angle()).toBe("55"));
    await click("Confirm min");

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

    await click("Confirm center");
    await waitFor(() => expect(angle()).toBe("80"));

    robot.failing.add("/setup/servo");
    await click("+5°");

    await waitFor(() => expect(statusText()).toBe("robot said no"));
    expect(angle()).toBe("80");
  });

  it("stays on the hub when the first guided park fails", async () => {
    const { robot, click } = await openWizard();

    robot.failing.add("/setup/servo");
    await click("Calibrate servos");

    await waitFor(() => expect(statusText()).toBe("robot said no"));
    expect(screen.getByRole("button", { name: "Calibrate servos" })).toBeTruthy();
    expect(screen.queryByRole("heading", { name: "Body · center" })).toBeNull();
  });

  it("stays on the current guided step when the next park fails", async () => {
    const { robot, click } = await startGuided();

    robot.failing.add("/setup/servo");
    await click("Confirm center");

    await waitFor(() => expect(statusText()).toBe("robot said no"));
    expect(screen.getByRole("heading", { name: "Body · center" })).toBeTruthy();
    expect(screen.queryByRole("heading", { name: "Body · minimum" })).toBeNull();
  });

  it("stays on the current guided step when Back park fails", async () => {
    const { robot, click } = await startGuided();

    await click("Confirm center");
    await screen.findByRole("heading", { name: "Body · minimum" });

    robot.failing.add("/setup/servo");
    await click("Back");

    await waitFor(() => expect(statusText()).toBe("robot said no"));
    expect(screen.getByRole("heading", { name: "Body · minimum" })).toBeTruthy();
    expect(screen.queryByRole("heading", { name: "Body · center" })).toBeNull();
  });

  it("rejects a body min at or above the session center", async () => {
    const { click } = await startGuided();

    await click("Confirm center");
    await waitFor(() => expect(angle()).toBe("80"));

    await click("+10°");
    await waitFor(() => expect(angle()).toBe("90"));
    await click("Confirm min");

    expect(statusText()).toBe("Min must be below the center so the mirrored max stays valid.");
    expect(screen.getByRole("heading", { name: "Body · minimum" })).toBeTruthy();
  });

  it("rejects a mirrored max above 180 without changing the range or running the demo", async () => {
    const { robot, click } = await startGuided();

    for (let i = 0; i < 5; i++) {
      await click("+10°");
    }

    await click("Confirm center");
    await waitFor(() => expect(angle()).toBe("80"));

    const movesBeforeConfirm = robot.sent("POST", "/setup/servo").length;

    await click("Confirm min");

    expect(statusText()).toBe(
      "Mirrored max must not exceed 180°. Move the center lower or min closer to center.",
    );
    expect(screen.getByRole("heading", { name: "Body · minimum" })).toBeTruthy();
    expect(document.querySelector(".calib-range-label")?.textContent).toBe("Saved range: 80°–100°");
    expect(robot.sent("POST", "/setup/servo")).toHaveLength(movesBeforeConfirm);
  });

  it("returns to the hub when backing from the first guided step", async () => {
    const { click } = await startGuided();

    const back = screen.getByRole("button", { name: "Back" });

    expect(back.classList.contains("btn-primary")).toBe(false);
    expect(back.classList.contains("btn-secondary")).toBe(false);

    await click("Back");

    expect(screen.getByRole("button", { name: "Calibrate servos" })).toBeTruthy();
  });

  it("restores guided substep drafts with Back and clears them after leaving", async () => {
    const { robot, click } = await startGuided();

    await click("−5°");
    await click("Confirm center");
    await click("−10°");
    await click("Confirm min");

    await screen.findByRole("heading", { name: "Neck · center" });
    await click("Back");

    await screen.findByRole("heading", { name: "Body · minimum" });
    await waitFor(() => expect(angle()).toBe("70"));
    expect(robot.lastPost("/setup/servo")?.json).toEqual({ index: 4, angle: 70 });

    await click("Back");
    await screen.findByRole("heading", { name: "Body · center" });
    await waitFor(() => expect(angle()).toBe("85"));

    await click("+5°");
    await waitFor(() => expect(angle()).toBe("90"));
    await click("Back");
    await click("Calibrate servos");
    await waitFor(() => expect(angle()).toBe("85"));
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
    await click("Confirm center");
    await waitFor(() => expect(angle()).toBe("80"));
    await click("−10°");
    await click("−10°");
    await click("Confirm min");

    await screen.findByRole("heading", { name: "Neck · center" });
    await waitFor(() => expect(angle()).toBe("90"));
    await click("Confirm center");
    await waitFor(() => expect(angle()).toBe("80"));
    await click("−10°");
    await click("−10°");
    await click("Confirm min");

    await screen.findByRole("heading", { name: "Head · center" });
    await waitFor(() => expect(angle()).toBe("90"));
    await click("Confirm center");
    await waitFor(() => expect(angle()).toBe("80"));
    await click("−10°");
    await click("−10°");
    await click("Confirm min");

    // Right min (keyboard) — park at saved min 80; nudge to 35
    await screen.findByRole("heading", { name: "Right hand · keyboard" });
    await waitFor(() => expect(angle()).toBe("80"));
    await click("−10°");
    await click("−10°");
    await click("−10°");
    await click("−10°");
    await click("−5°");
    await click("Confirm min");

    // Left max (keyboard) — park at saved max 100; nudge to 135
    await screen.findByRole("heading", { name: "Left hand · keyboard" });
    await waitFor(() => expect(angle()).toBe("100"));
    await click("+10°");
    await click("+10°");
    await click("+10°");
    await click("+5°");
    await click("Confirm max");

    // Left min (upper) — park at saved min 80; nudge to 45
    await screen.findByRole("heading", { name: "Left hand · upper" });
    await waitFor(() => expect(angle()).toBe("80"));
    await click("−10°");
    await click("−10°");
    await click("−10°");
    await click("−5°");
    await click("Confirm min");

    // Right max (upper) — park at current max 100; nudge to 125
    await screen.findByRole("heading", { name: "Right hand · upper" });
    await waitFor(() => expect(angle()).toBe("100"));
    await click("+10°");
    await click("+10°");
    await click("+5°");

    expect(screen.getByRole("button", { name: "Save and continue" })).toBeTruthy();

    await click("Save and continue");

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
    expect(screen.getByRole("button", { name: "Calibrate servos" })).toBeTruthy();
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

  it("shows completion feedback for LED previews and speaker playback", async () => {
    const { click } = await skipCalibration();

    await click("Next: RGB mapping");
    await click("Red");

    await waitFor(() =>
      expect(screen.getByTestId("led-completion").textContent).toBe("✓ Red preview sent"),
    );

    await click("Next: Speaker");
    await click("Play");

    await waitFor(() =>
      expect(screen.getByTestId("speaker-completion").textContent).toBe("✓ Welcome clip played"),
    );
    expect(screen.getByRole("button", { name: "Play again" })).toBeTruthy();
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
