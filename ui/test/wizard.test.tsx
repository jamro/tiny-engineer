import { screen, waitFor, within } from "@testing-library/preact";
import { describe, expect, it } from "vitest";
import { fakeRobot, renderApp } from "./robot";

const angle = () => screen.getByTestId("calib-angle").textContent;
const stepLabel = () => screen.getByText(/^Step \d of 5/).textContent;
const statusText = () => screen.queryByRole("status")?.textContent;

async function startCalibration(settings = {}) {
  const robot = fakeRobot({ provisioning: true, settings });
  const app = renderApp("/");

  await app.click("Parts are on →");

  return { robot, ...app };
}

describe("setup wizard", () => {
  it("takes over the config page while the robot is provisioning", async () => {
    fakeRobot({ provisioning: true });
    renderApp("/");

    await screen.findByRole("heading", { name: "Tiny Engineer setup" });
    expect(screen.queryByRole("navigation")).toBeNull();
    expect(stepLabel()).toBe("Step 1 of 5 · Servos");
  });

  it("offers no control that moves a servo from the calibration bar", async () => {
    await startCalibration();

    const guide = document.querySelector(".calib-guide");

    expect(guide?.getAttribute("aria-hidden")).toBe("true");
    expect(guide?.querySelector("input, button")).toBeNull();
  });

  it("moves the selected joint in steps and stops at 180°", async () => {
    const { robot, click } = await startCalibration();

    for (let i = 0; i < 10; i++) {
      await click("+10°");
    }

    await waitFor(() => expect(robot.sent("POST", "/setup/servo")).toHaveLength(9));
    expect(angle()).toBe("180");
    expect(
      robot
        .sent("POST", "/setup/servo")
        .map((request) => (request.json as { angle: number }).angle),
    ).toEqual([100, 110, 120, 130, 140, 150, 160, 170, 180]);
  });

  it("restores the shown angle when a move fails", async () => {
    const { robot, click } = await startCalibration();

    robot.failing.add("/setup/servo");
    await click("+5°");

    await waitFor(() => expect(statusText()).toBe("robot said no"));
    expect(angle()).toBe("90");
  });

  it("keeps each joint's angle when switching tabs", async () => {
    const { robot, click } = await startCalibration();

    await click("−10°");
    await click("Neck");

    expect(angle()).toBe("90");

    await click("+1°");

    await waitFor(() =>
      expect(robot.lastPost("/setup/servo")?.json).toEqual({ index: 1, angle: 91 }),
    );

    await click("Head");

    expect(angle()).toBe("80");
  });

  it("refuses a min at or above the max", async () => {
    const { click } = await startCalibration();

    await click("Set max (130°)");
    await click("+5°");
    await click("Set min (60°)");

    expect(statusText()).toBe("Min must be less than max.");
    expect(screen.getByRole("button", { name: "Set min (60°)" })).toBeTruthy();
    expect(screen.getByRole("button", { name: "Set max (90°)" })).toBeTruthy();
  });

  it("saves the calibrated ranges and previews the screen on Next", async () => {
    const { robot, click } = await startCalibration();

    await click("−10°");
    await click("Set min (60°)");
    await click("Next: Screen");

    await waitFor(() => expect(stepLabel()).toBe("Step 2 of 5 · Screen"));
    expect(robot.lastPost("/settings")?.json).toEqual({
      servo_mins: [80, 40, 45, 35, 40],
      servo_maxs: [130, 130, 135, 125, 130],
    });
    await waitFor(() => expect(robot.lastPost("/setup/oled")?.json).toEqual({ rotate_180: false }));
  });

  it("restores the setup screen when leaving the screen step", async () => {
    const { robot, click } = await startCalibration();

    await click("Next: Screen");
    await waitFor(() => expect(robot.lastPost("/setup/oled")?.json).toBeDefined());
    await click("Back");

    await waitFor(() => expect(robot.lastPost("/setup/oled")?.json).toBeUndefined());
    expect(stepLabel()).toBe("Step 1 of 5 · Servos");
  });

  it("saves the hostname, then tests the new network ahead of the saved ones", async () => {
    const { robot, user, click } = await startCalibration({ wifi_networks: ["old"] });

    for (const next of ["Next: Screen", "Next: RGB mapping", "Next: Speaker", "Next: Connection"]) {
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
    const { robot, click, user } = await startCalibration({
      wifi_networks: [],
      link_mode: "",
      wifi_configured: false,
    });

    await click("Next: Screen");
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
    const { robot, click, user } = await startCalibration();

    await click("Next: Screen");
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
