import { beforeEach, describe, expect, it } from "vitest";
import { $, click, fakeRobot, loadApp } from "./robot.js";

const angle = () => $("#setup-calib-angle").textContent;
const stepLabel = () => $("#setup-progress-label").textContent;

async function startCalibration() {
  const robot = fakeRobot({ provisioning: true });

  await loadApp("/");
  await click("#setup-horns-done");

  return robot;
}

describe("setup wizard", () => {
  beforeEach(() => sessionStorage.clear());

  it("takes over the config page while the robot is provisioning", async () => {
    fakeRobot({ provisioning: true });
    await loadApp("/");

    expect(document.body.classList.contains("setup-mode")).toBe(true);
    expect($("#view-config").classList.contains("active")).toBe(true);
    expect(stepLabel()).toBe("Step 1 of 5 · Servos");
  });

  it("offers no control that moves a servo from the calibration bar", async () => {
    await startCalibration();

    expect($(".calib-guide").getAttribute("aria-hidden")).toBe("true");
    expect($(".calib-guide").querySelector("input, button")).toBeNull();
  });

  it("moves the selected joint in steps and stops at 180°", async () => {
    const robot = await startCalibration();

    for (let i = 0; i < 10; i++) {
      await click('[data-nudge="10"]');
    }

    expect(angle()).toBe("180");
    expect(robot.posts("/setup/servo").map((request) => request.params.angle)).toEqual([
      "100",
      "110",
      "120",
      "130",
      "140",
      "150",
      "160",
      "170",
      "180",
    ]);
  });

  it("restores the shown angle when a move fails", async () => {
    const robot = await startCalibration();

    robot.failing.add("/setup/servo");
    await click('[data-nudge="5"]');

    expect(angle()).toBe("90");
    expect($("#status").textContent).toBe("robot said no");
  });

  it("keeps each joint's angle when switching tabs", async () => {
    const robot = await startCalibration();

    await click('[data-nudge="-10"]');
    await click('#setup-joint-tabs [data-joint="1"]');

    expect(angle()).toBe("90");

    await click('[data-nudge="1"]');

    expect(robot.lastPost("/setup/servo").params).toEqual({ index: "1", angle: "91" });

    await click('#setup-joint-tabs [data-joint="0"]');

    expect(angle()).toBe("80");
  });

  it("refuses a min at or above the max", async () => {
    await startCalibration();

    await click("#setup-set-max");
    await click('[data-nudge="5"]');
    await click("#setup-set-min");

    expect($("#status").textContent).toBe("Min must be less than max.");
    expect($("#setup-min-label").textContent).toBe("60");
    expect($("#setup-max-label").textContent).toBe("90");
  });

  it("saves the calibrated ranges and previews the screen on Next", async () => {
    const robot = await startCalibration();

    await click('[data-nudge="-10"]');
    await click("#setup-set-min");
    await click("#setup-next");

    expect(robot.lastPost("/settings").params).toEqual({
      servo_mins: "80,40,45,35,40",
      servo_maxs: "130,130,135,125,130",
    });
    expect(robot.lastPost("/setup/oled").params).toEqual({ rotate_180: "0" });
    expect(stepLabel()).toBe("Step 2 of 5 · Screen");
  });

  it("saves the hostname, then tests the new network ahead of the saved ones", async () => {
    const robot = fakeRobot({ provisioning: true, settings: { wifi_networks: ["old"] } });

    await loadApp("/");
    $("#config-wifi-ssid").value = "home";
    $("#config-wifi-password").value = "secret";
    $("#config-wifi-hostname").value = "desk-bot";
    await click("#config-wifi-connect");

    const settingsWrites = robot.requests
      .filter((request) => request.method !== "GET" && request.path.startsWith("/settings"))
      .map((request) => `${request.method} ${request.path}`);
    const put = robot.requests.find((request) => request.method === "PUT");

    expect(settingsWrites).toEqual(["POST /settings", "PUT /settings/wifi"]);
    expect(robot.lastPost("/settings").params).toEqual({ hostname: "desk-bot" });
    expect(put.json).toEqual([{ ssid: "home", password: "secret" }, { ssid: "old" }]);
    expect($("#status").textContent).toContain("WiFi connected");
  });

  it("finishes setup as USB serial only without joining Wi-Fi", async () => {
    const robot = fakeRobot({ provisioning: true });

    await loadApp("/");
    await click("#setup-serial-only");

    expect(robot.lastPost("/settings").params).toEqual({ wifi_enabled: "0" });
    expect($("#reboot-gate").classList.contains("show")).toBe(true);
    expect($("#reboot-gate-title").textContent).toBe("Wi-Fi disabled");
    expect(document.body.classList.contains("locked")).toBe(true);
  });

  it("blocks a color mapping that uses a color twice", async () => {
    const robot = await startCalibration();

    await click("#setup-next");
    await click("#setup-next");

    expect(stepLabel()).toBe("Step 3 of 5 · RGB mapping");

    await click("#setup-led-remap-toggle");
    await click('.led-looks[data-led-byte="0"] [data-look="R"]');

    expect($("#setup-next").disabled).toBe(true);

    await click('.led-looks[data-led-byte="1"] [data-look="G"]');

    expect($("#setup-next").disabled).toBe(false);

    await click("#setup-next");

    expect(robot.lastPost("/settings").params).toEqual({ rgb_order: "RGB" });
    expect(stepLabel()).toBe("Step 4 of 5 · Speaker");
  });
});
