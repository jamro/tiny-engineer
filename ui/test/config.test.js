import { beforeEach, describe, expect, it } from "vitest";
import { $, click, fakeRobot, loadApp, submit, type } from "./robot.js";

describe("config page", () => {
  beforeEach(() => sessionStorage.clear());

  it("fills the form from the robot's settings", async () => {
    fakeRobot({ settings: { hostname: "desk-bot", volume: 35, eyes_style: "dots" } });
    await loadApp("/config");

    expect($("#config-hostname").value).toBe("desk-bot");
    expect($("#config-volume-label").textContent).toBe("35%");
    expect($("#config-eyes-style").value).toBe("dots");
  });

  it("saves the form without touching a saved token", async () => {
    const robot = fakeRobot({ settings: { access_token_set: true } });

    await loadApp("/config");

    $("#config-sleep").value = "20";
    await submit("#config-form");

    const { params } = robot.lastPost("/settings");

    expect(params).toMatchObject({
      sleep_timeout: "20",
      hostname: "tiny-engineer",
      volume: "70",
      welcome: "1",
      serial_log: "0",
      wifi_enabled: "1",
      loading: "progress",
      eyes_style: "classic",
    });
    expect(params).not.toHaveProperty("access_token");
    expect($("#status").textContent).toBe("Settings saved.");
  });

  it("says when a hostname change needs a reboot", async () => {
    fakeRobot();
    await loadApp("/config");

    $("#config-hostname").value = "desk-bot";
    await submit("#config-form");

    expect($("#status").textContent).toBe("Saved. Hostname applies after reboot.");
  });

  it("shows the serial-only reboot gate when Wi-Fi is disabled", async () => {
    fakeRobot();
    await loadApp("/config");

    $("#config-wifi-enabled").checked = false;
    await submit("#config-form");

    expect($("#reboot-gate").classList.contains("show")).toBe(true);
    expect($("#reboot-gate-title").textContent).toBe("Wi-Fi disabled");
    expect(document.body.classList.contains("locked")).toBe(true);
  });

  it("sends an empty token to remove it and forgets the stored copy", async () => {
    const robot = fakeRobot({ settings: { access_token_set: true } });

    sessionStorage.setItem("te_access_token", "old");
    await loadApp("/config");

    await click("#config-access-token-toggle");
    await submit("#config-form");

    expect(robot.lastPost("/settings").params.access_token).toBe("");
    expect(sessionStorage.getItem("te_access_token")).toBeNull();
    expect($("#config-access-token-status").textContent).toBe("Auth disabled");
  });

  it("replaces the token with a typed one and keeps using it", async () => {
    const robot = fakeRobot({ settings: { access_token_set: true } });

    await loadApp("/config");

    type("#config-access-token", "new-token");
    await submit("#config-form");

    expect(robot.lastPost("/settings").params.access_token).toBe("new-token");
    expect(sessionStorage.getItem("te_access_token")).toBe("new-token");
  });
});
