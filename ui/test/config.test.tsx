import { screen, waitFor } from "@testing-library/preact";
import { describe, expect, it } from "vitest";
import { fakeRobot, renderApp } from "./robot";

const field = (label: string) => screen.getByLabelText<HTMLInputElement>(label);
const statusText = () => screen.queryByRole("status")?.textContent;

// The form renders once the settings have loaded.
async function openConfig(settings = {}) {
  const robot = fakeRobot({ settings });
  const app = renderApp("/config");

  await screen.findByLabelText("Hostname");

  return { robot, ...app };
}

describe("config page", () => {
  it("fills the form from the robot's settings", async () => {
    await openConfig({ hostname: "desk-bot", volume: 35, eyes_style: "dots" });

    expect(field("Hostname").value).toBe("desk-bot");
    expect(screen.getByText("35%")).toBeTruthy();
    expect(field("Eyes style").value).toBe("dots");
  });

  it("saves the form without touching a saved token", async () => {
    const { robot, user, click } = await openConfig({ access_token_set: true });

    await user.clear(field("Sleep timeout (minutes)"));
    await user.type(field("Sleep timeout (minutes)"), "20");
    await click("Save settings");

    await waitFor(() => expect(statusText()).toBe("Settings saved."));
    expect(robot.lastPost("/settings")?.json).toEqual({
      sleep_timeout: 20,
      hostname: "tiny-engineer",
      volume: 70,
      welcome: true,
      serial_log: false,
      continuous_timeout: 5,
      loading: "progress",
      eyes_style: "classic",
    });
  });

  it("says when a hostname change needs a reboot", async () => {
    const { user, click } = await openConfig();

    await user.clear(field("Hostname"));
    await user.type(field("Hostname"), "desk-bot");
    await click("Save settings");

    await waitFor(() => expect(statusText()).toBe("Saved. Hostname applies after reboot."));
  });

  it("says when a loading screen change needs a reboot", async () => {
    const { user, click } = await openConfig();

    await user.selectOptions(field("Boot loading screen"), "sleep_inertia");
    await click("Save settings");

    await waitFor(() => expect(statusText()).toBe("Saved. Loading screen applies after reboot."));
  });

  it("sends an empty token to remove it and forgets the stored copy", async () => {
    sessionStorage.setItem("te_access_token", "old");

    const { robot, click } = await openConfig({ access_token_set: true });

    await click("Remove token");
    await click("Save settings");

    await screen.findByText("Auth disabled");
    expect(robot.lastPost("/settings")?.json).toHaveProperty("access_token", "");
    expect(sessionStorage.getItem("te_access_token")).toBeNull();
  });

  it("replaces the token with a typed one and keeps using it", async () => {
    const { robot, user, click } = await openConfig({ access_token_set: true });

    await user.type(field("Access token"), "new-token");
    await click("Save settings");

    await waitFor(() => expect(field("Access token").value).toBe("********"));
    expect(robot.lastPost("/settings")?.json).toHaveProperty("access_token", "new-token");
    expect(sessionStorage.getItem("te_access_token")).toBe("new-token");
  });

  it("saves swapped hand channels without moving servos", async () => {
    const { robot, user, click } = await openConfig();

    await user.selectOptions(await screen.findByLabelText("Left hand servo channel"), "3");
    await click("Save hand assignment");

    await waitFor(() => expect(robot.lastPost("/settings")?.json).toEqual({ swap_hands: true }));
    expect(robot.sent("POST", "/test/servo")).toHaveLength(0);
    expect(robot.settings.servo_centers).toEqual([95, 85, 90, 80, 85]);
    expect(screen.getByRole("status").textContent).toContain("Restart the robot");
  });

  it("can restore the default hand assignment", async () => {
    const { robot, user, click } = await openConfig({ swap_hands: true });

    await user.selectOptions(await screen.findByLabelText("Left hand servo channel"), "2");
    await click("Save hand assignment");

    await waitFor(() => expect(robot.lastPost("/settings")?.json).toEqual({ swap_hands: false }));
  });

  it("keeps the saved hand assignment when saving fails", async () => {
    const { robot, user, click } = await openConfig();

    await user.selectOptions(await screen.findByLabelText("Left hand servo channel"), "3");
    robot.failing.add("/settings");
    await click("Save hand assignment");

    await waitFor(() => expect(screen.getByRole("status").textContent).toBe("robot said no"));
    expect(robot.settings.swap_hands).toBe(false);
    expect(screen.getByText(/Saved assignment: left on 2, right on 3/)).toBeTruthy();
  });
});
