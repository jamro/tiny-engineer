import { screen, waitFor } from "@testing-library/preact";
import { describe, expect, it } from "vitest";
import { fakeRobot, renderApp } from "./robot";

async function typeAngle(user: ReturnType<typeof renderApp>["user"], value: string) {
  const angle = await screen.findByRole("spinbutton", { name: "Angle" });

  await user.clear(angle);
  await user.type(angle, value);
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

    expect(screen.getByText(/firmware clamps to 80–100°/).classList.contains("warn")).toBe(true);
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
});
