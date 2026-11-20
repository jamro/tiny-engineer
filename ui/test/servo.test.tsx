import { screen, waitFor } from "@testing-library/preact";
import { describe, expect, it, vi } from "vitest";
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

const faceAngle = () => screen.getByTestId("face-angle").textContent;

describe("face me section", () => {
  it("starts at the default pose and says no pose is set", async () => {
    fakeRobot();
    renderApp("/servo");

    expect(await screen.findByText("Not set (default pose)")).toBeTruthy();
    // Head 1/7 of the half-span above mid 90 is 91.4°, shown in whole degrees.
    expect(faceAngle()).toBe("91");
    expect(screen.getByRole("button", { name: "Neck 90°" })).toBeTruthy();
  });

  it("starts at the saved pose", async () => {
    fakeRobot({ settings: { face_pose: [0.2, -0.5, 0.25] } });
    renderApp("/servo");

    expect(await screen.findByRole("button", { name: "Neck 85°" })).toBeTruthy();
    expect(screen.getByText("Set")).toBeTruthy();
    expect(screen.getByText(/Check it again after a servo calibration/)).toBeTruthy();
  });

  it("nudges the picked joint in degrees inside its saved range", async () => {
    const robot = fakeRobot({
      settings: { servo_mins: [70, 40, 45, 35, 60], servo_maxs: [110, 130, 135, 125, 120] },
    });
    const { click } = renderApp("/servo");

    await click(/^Body/);
    await click("+5°");

    await waitFor(() =>
      expect(robot.lastPost("/test/servo")?.json).toEqual({ index: 4, angle: 95 }),
    );

    for (let i = 0; i < 6; i++) {
      await click("+5°");
    }

    // Body max is 120: the last nudges stop there.
    await waitFor(() => expect(faceAngle()).toBe("120"));
    expect(robot.lastPost("/test/servo")?.json).toEqual({ index: 4, angle: 120 });
    expect(screen.queryByRole("button", { name: "+10°" })).toBeNull();
  });

  it("shows the picked joint's whole saved range on the bar", async () => {
    fakeRobot({
      settings: { servo_mins: [70, 40, 45, 35, 60], servo_maxs: [110, 130, 135, 125, 120] },
    });
    const { click } = renderApp("/servo");

    await click(/^Neck/);

    expect(document.querySelector(".face-pose .servo-scale")?.textContent).toBe("40°85°130°");
    // Neck at mid 85 sits in the middle of 40..130.
    expect(document.querySelector<HTMLElement>(".face-pose .calib-marker")?.style.left).toBe("50%");
  });

  it("nudges whole degrees from a rounded start", async () => {
    const robot = fakeRobot();
    const { click } = renderApp("/servo");

    await screen.findByText("Not set (default pose)");
    await click("+1°");

    // Head starts at 91.4°, shown as 91; +1 goes to 92.
    await waitFor(() =>
      expect(robot.lastPost("/test/servo")?.json).toEqual({ index: 0, angle: 92 }),
    );
    expect(faceAngle()).toBe("92");
  });

  it("keeps the pose when a move fails", async () => {
    const robot = fakeRobot();
    const { click } = renderApp("/servo");

    robot.failing.add("/test/servo");
    await click(/^Neck/);
    await click("−5°");

    await waitFor(() => expect(robot.sent("POST", "/test/servo")).toHaveLength(1));
    expect(faceAngle()).toBe("90");
  });

  it("saves the current pose", async () => {
    const robot = fakeRobot();
    const { click } = renderApp("/servo");

    await click(/^Neck/);
    await click("−1°");
    await click("Save face pose");

    // Neck 89° in the stock 80–100 range is -0.1.
    await waitFor(() =>
      expect(robot.lastPost("/settings")?.json).toEqual({ face_pose: [1 / 7, -0.1, 0] }),
    );
    expect(await screen.findByText("Set")).toBeTruthy();
  });

  it("clears the pose after a confirm", async () => {
    const robot = fakeRobot({ settings: { face_pose: [0.2, -0.5, 0.25] } });

    vi.spyOn(window, "confirm").mockReturnValueOnce(true);
    const { click } = renderApp("/servo");

    await click("Clear");

    await waitFor(() => expect(robot.lastPost("/settings")?.json).toEqual({ face_pose: [] }));
    expect(await screen.findByText("Not set (default pose)")).toBeTruthy();
  });

  it("moves all three joints to the shown pose before the first nudge", async () => {
    const robot = fakeRobot();
    const { click } = renderApp("/servo");

    await click(/^Neck/);
    await click("−1°");

    await waitFor(() => expect(robot.sent("POST", "/test/servo")).toHaveLength(4));
    const moves = robot.sent("POST", "/test/servo").map((request) => request.json);

    expect(moves.slice(0, 3).map((json) => (json as { index: number }).index)).toEqual([0, 1, 4]);
    expect(moves[3]).toEqual({ index: 1, angle: 89 });

    await click("−1°");

    // Synced once: the next nudge moves only the picked joint.
    await waitFor(() => expect(robot.sent("POST", "/test/servo")).toHaveLength(5));
    expect(robot.lastPost("/test/servo")?.json).toEqual({ index: 1, angle: 88 });
  });

  it("sends no move when stopping the preview fails", async () => {
    const robot = fakeRobot();
    const { click } = renderApp("/servo");

    await click("Preview talking");
    await waitFor(() => expect(robot.lastPost("/anim")?.params).toEqual({ name: "talking" }));

    robot.failing.add("/anim");
    await click("+1°");

    await waitFor(() => expect(robot.lastPost("/anim")?.params).toEqual({ name: "none" }));
    expect(robot.sent("POST", "/test/servo")).toHaveLength(0);
    expect(faceAngle()).toBe("91");
  });

  it("resets the controls to the default pose after Clear", async () => {
    fakeRobot({ settings: { face_pose: [0.2, -0.5, 0.25] } });

    vi.spyOn(window, "confirm").mockReturnValueOnce(true);
    const { click } = renderApp("/servo");

    expect(await screen.findByRole("button", { name: "Neck 85°" })).toBeTruthy();
    await click("Clear");

    expect(await screen.findByRole("button", { name: "Neck 90°" })).toBeTruthy();
    expect(faceAngle()).toBe("91");
  });

  it("moves all joints to the shown pose before Save, also without a nudge", async () => {
    const robot = fakeRobot();
    const { click } = renderApp("/servo");

    await screen.findByText("Not set (default pose)");
    await click("Save face pose");

    await waitFor(() =>
      expect(robot.lastPost("/settings")?.json).toEqual({ face_pose: [1 / 7, 0, 0] }),
    );
    expect(robot.sent("POST", "/test/servo").map((request) => request.json)).toEqual([
      { index: 0, angle: 91 },
      { index: 1, angle: 90 },
      { index: 4, angle: 90 },
    ]);
  });

  it("does not save when a move before Save fails", async () => {
    const robot = fakeRobot();
    const { click } = renderApp("/servo");

    await screen.findByText("Not set (default pose)");
    robot.failing.add("/test/servo");
    await click("Save face pose");

    await waitFor(() => expect(robot.sent("POST", "/test/servo")).toHaveLength(1));
    expect(robot.sent("POST", "/settings")).toHaveLength(0);
    expect(screen.getByText("Not set (default pose)")).toBeTruthy();
  });

  it("syncs all joints again after the Servo control form moves a joint", async () => {
    const robot = fakeRobot();
    const { user, click } = renderApp("/servo");

    await click(/^Neck/);
    await click("−1°");
    await waitFor(() => expect(robot.sent("POST", "/test/servo")).toHaveLength(4));

    await typeAngle(user, "100");
    await click("Move servo");
    expect(await screen.findByText("Servo 0 moved to 100°.")).toBeTruthy();

    await click("−1°");

    // Head was moved by the form: the nudge first sends all three joints again.
    await waitFor(() => expect(robot.sent("POST", "/test/servo")).toHaveLength(9));
    expect(robot.sent("POST", "/test/servo")[5].json).toEqual({ index: 0, angle: 91 });
    expect(robot.lastPost("/test/servo")?.json).toEqual({ index: 1, angle: 88 });
  });

  it("syncs all joints again after a preview start and stop", async () => {
    const robot = fakeRobot();
    const { click } = renderApp("/servo");

    await click(/^Neck/);
    await click("−1°");
    await waitFor(() => expect(robot.sent("POST", "/test/servo")).toHaveLength(4));

    await click("Preview talking");
    await waitFor(() => expect(robot.lastPost("/anim")?.params).toEqual({ name: "talking" }));
    await click("Stop preview");
    await waitFor(() => expect(robot.lastPost("/anim")?.params).toEqual({ name: "none" }));

    await click("−1°");

    // Stopping the preview parks neck and body: the nudge first sends all joints again.
    await waitFor(() => expect(robot.sent("POST", "/test/servo")).toHaveLength(8));
    expect(robot.lastPost("/test/servo")?.json).toEqual({ index: 1, angle: 88 });
  });

  it("previews talking and stops it", async () => {
    const robot = fakeRobot();
    const { click } = renderApp("/servo");

    await click("Preview talking");
    await waitFor(() => expect(robot.lastPost("/anim")?.params).toEqual({ name: "talking" }));

    await click("Stop preview");
    await waitFor(() => expect(robot.lastPost("/anim")?.params).toEqual({ name: "none" }));
  });
});
