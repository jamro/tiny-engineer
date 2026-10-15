import { screen, waitFor } from "@testing-library/preact";
import { describe, expect, it } from "vitest";
import { fakeRobot, renderApp } from "./robot";

async function typeAngle(user: ReturnType<typeof renderApp>["user"], value: string) {
  const angle = await screen.findByRole("spinbutton", { name: "Angle" });

  await user.clear(angle);
  await user.type(angle, value);
}

async function typeLimit(user: ReturnType<typeof renderApp>["user"], name: string, value: string) {
  const field = await screen.findByRole("spinbutton", { name });

  await user.clear(field);

  if (value) {
    await user.type(field, value);
  }
}

describe("servo page", () => {
  it("bounds the slider by the selected joint's saved range", async () => {
    fakeRobot({
      settings: { servo_mins: [70, 40, 45, 35, 40], servo_maxs: [110, 130, 135, 125, 130] },
    });
    renderApp("/servo");

    const slider = await screen.findByLabelText("Angle slider");

    await waitFor(() => expect(slider.getAttribute("min")).toBe("70"));
    expect(slider.getAttribute("max")).toBe("110");
  });

  it("warns when a typed angle is outside the safe range", async () => {
    fakeRobot();
    const { user } = renderApp("/servo");

    await typeAngle(user, "150");

    expect(screen.getByText(/firmware clamps to 60–130°/).classList.contains("warn")).toBe(true);
  });

  it("moves only when the form is submitted", async () => {
    const robot = fakeRobot();
    const { user, click } = renderApp("/servo");

    await typeAngle(user, "100");

    expect(robot.sent("POST", "/test/servo")).toHaveLength(0);

    await click("Move servo");

    await waitFor(() =>
      expect(robot.lastPost("/test/servo")?.json).toEqual({ index: 0, angle: 100 }),
    );
  });

  it("moves to the saved center instead of the range midpoint", async () => {
    const robot = fakeRobot({ settings: { servo_centers: [102.5, 85, 90, 80, 85] } });
    const { click } = renderApp("/servo");

    await click("Center");

    await waitFor(() =>
      expect(robot.lastPost("/test/servo")?.json).toEqual({ index: 0, angle: 102.5 }),
    );
  });

  it("saves only the selected joint's center without moving it or changing limits", async () => {
    const robot = fakeRobot();
    const { user, click } = renderApp("/servo");

    await user.selectOptions(await screen.findByLabelText("Servo"), "4");
    await typeAngle(user, "92.5");
    await click("Save angle as center");

    await waitFor(() =>
      expect(robot.lastPost("/settings")?.json).toEqual({
        servo_centers: [95, 85, 90, 80, 92.5],
      }),
    );
    expect(robot.sent("POST", "/test/servo")).toHaveLength(0);
    expect(screen.getByText("Saved center for Body: 92.5°")).toBeTruthy();
  });

  it("disables saving a center on or outside a limit", async () => {
    fakeRobot();
    const { user } = renderApp("/servo");

    for (const angle of ["60", "130", "150"]) {
      await typeAngle(user, angle);
      expect(
        screen.getByRole<HTMLButtonElement>("button", { name: "Save angle as center" }).disabled,
      ).toBe(true);
    }
  });

  it("keeps the saved center when saving fails", async () => {
    const robot = fakeRobot();
    const { user, click } = renderApp("/servo");

    await typeAngle(user, "100");
    robot.failing.add("/settings");
    await click("Save angle as center");

    await waitFor(() => expect(screen.getByRole("status").textContent).toBe("robot said no"));
    expect(screen.getByText("Saved center for Head: 95°")).toBeTruthy();
  });

  it("resets only the selected center to the midpoint", async () => {
    const robot = fakeRobot({ settings: { servo_centers: [102.5, 85, 90, 80, 92] } });
    const { click } = renderApp("/servo");

    await click("Reset center to midpoint");

    await waitFor(() => expect(robot.settings.servo_centers).toEqual([95, 85, 90, 80, 92]));
  });
});

describe("servo limits", () => {
  it("saves the selected joint's limits and preserves the other calibration", async () => {
    const robot = fakeRobot();
    const { user, click } = renderApp("/servo");

    await user.selectOptions(await screen.findByLabelText("Servo"), "4");
    await typeLimit(user, "Min (°)", "50");
    await typeLimit(user, "Max (°)", "120");

    expect(robot.sent("POST", "/settings")).toHaveLength(0);

    await click("Save limits");

    await waitFor(() =>
      expect(robot.lastPost("/settings")?.json).toEqual({
        servo_mins: [60, 40, 45, 35, 50],
        servo_maxs: [130, 130, 135, 125, 120],
        servo_centers: [95, 85, 90, 80, 85],
      }),
    );
    expect(robot.sent("POST", "/test/servo")).toHaveLength(0);
    expect(robot.settings.swap_hands).toBe(false);
  });

  it("updates the slider only after saving and previews a recentered midpoint", async () => {
    const robot = fakeRobot();
    const { user, click } = renderApp("/servo");

    await typeLimit(user, "Min (°)", "100");
    await typeLimit(user, "Max (°)", "121");

    expect(
      screen.getByText("Center will reset to 110.5° to stay between the new limits."),
    ).toBeTruthy();
    expect(screen.getByLabelText("Angle slider").getAttribute("min")).toBe("60");

    await click("Save limits");

    await waitFor(() =>
      expect(screen.getByLabelText("Angle slider").getAttribute("min")).toBe("100"),
    );
    expect(screen.getByLabelText("Angle slider").getAttribute("max")).toBe("121");
    expect(robot.settings.servo_centers).toEqual([110.5, 85, 90, 80, 85]);
    expect(screen.getByText("Saved center for Head: 110.5°")).toBeTruthy();
  });

  it("blocks empty, fractional, reversed, equal, and out-of-range limits", async () => {
    const robot = fakeRobot();
    const { user } = renderApp("/servo");

    for (const min of ["", "70.5", "130", "140"]) {
      await typeLimit(user, "Min (°)", min);
      expect(screen.getByRole<HTMLButtonElement>("button", { name: "Save limits" }).disabled).toBe(
        true,
      );
    }

    await typeLimit(user, "Min (°)", "70");
    await typeLimit(user, "Max (°)", "181");
    expect(screen.getByRole<HTMLButtonElement>("button", { name: "Save limits" }).disabled).toBe(
      true,
    );
    expect(robot.sent("POST", "/settings")).toHaveLength(0);
  });

  it("keeps the active limits when saving fails and allows retry", async () => {
    const robot = fakeRobot();
    const { user, click } = renderApp("/servo");

    await typeLimit(user, "Min (°)", "70");
    robot.failing.add("/settings");
    await click("Save limits");

    await waitFor(() => expect(screen.getByRole("status").textContent).toBe("robot said no"));
    expect(screen.getByLabelText("Angle slider").getAttribute("min")).toBe("60");
    expect(screen.getByLabelText<HTMLInputElement>("Min (°)").value).toBe("70");

    robot.failing.delete("/settings");
    await click("Save limits");

    await waitFor(() =>
      expect(screen.getByLabelText("Angle slider").getAttribute("min")).toBe("70"),
    );
  });

  it("loads the selected joint's limits when switching servos", async () => {
    fakeRobot();
    const { user } = renderApp("/servo");

    await typeLimit(user, "Min (°)", "70");
    await user.selectOptions(screen.getByLabelText("Servo"), "3");

    expect(screen.getByLabelText<HTMLInputElement>("Min (°)").value).toBe("35");
    expect(screen.getByLabelText<HTMLInputElement>("Max (°)").value).toBe("125");
    expect(screen.getByRole<HTMLButtonElement>("button", { name: "Save limits" }).disabled).toBe(
      true,
    );
  });
});
