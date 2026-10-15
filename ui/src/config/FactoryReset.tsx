import { token } from "../api/client";
import { Button } from "../components/Button";
import { ConfigSection } from "../components/ConfigSection";
import { useRobot } from "../robot";
import { useStatus } from "../status/status";

export function FactoryReset() {
  const { showRebootGate } = useRobot();
  const { perform } = useStatus();

  async function reset() {
    const confirmed = confirm(
      "Reset settings to factory defaults? WiFi credentials will be cleared. Servo ranges and centers, hand assignment, RGB LED mapping, and screen rotation stay. Power-cycle the device to reopen setup AP mode and configure WiFi again.",
    );

    if (!confirmed) {
      return;
    }

    await perform({
      path: "/settings/reset",
      pending: "Resetting…",
      failure: "Factory reset failed",
      onSuccess: () => {
        token.set("");
        showRebootGate();
      },
    });
  }

  return (
    <ConfigSection title="Factory reset" className="config-danger">
      <p className="hint">
        Erases saved settings and restores defaults. WiFi credentials are cleared. Servo ranges and
        centers, hand assignment, RGB LED mapping, and screen rotation stay. Power-cycle the device
        to reopen setup AP mode and configure WiFi again.
      </p>
      <Button variant="danger" onClick={reset}>
        Factory reset
      </Button>
    </ConfigSection>
  );
}
