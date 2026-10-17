import clsx from "clsx";
import { useEffect, useState } from "preact/hooks";
import { Button } from "../components/Button";
import { ConfigSection } from "../components/ConfigSection";
import { useStatus } from "../status/status";
import { type LedMapping, releaseLed } from "./useLedMapping";

const CHANNELS = [
  ["R", "Red"],
  ["G", "Green"],
  ["B", "Blue"],
] as const;
const BYTE_LABELS = ["First byte", "Second byte", "Third byte"];

export function LedStep({ mapping }: { mapping: LedMapping }) {
  const { perform } = useStatus();
  const [completedAction, setCompletedAction] = useState<string>();

  // Leaving the step hands the LED back to the setup AP's dim blue.
  useEffect(() => () => void releaseLed(), []);

  async function preview(
    json: { color: string; rgb_order: string } | { byte: number },
    completed: string,
  ) {
    const result = await perform({
      path: "/setup/led",
      request: { method: "POST", json },
      failure: "LED preview failed",
    });

    if (result?.ok) {
      setCompletedAction(completed);
    }
  }

  function pickLook(byte: number, look: string) {
    mapping.setLooks(mapping.looks.map((each, index) => (index === byte ? look : each)));
  }

  return (
    <ConfigSection title="RGB LED">
      <p className="setup-copy">
        Tap Red, Green, and Blue. The LED should match. If a color looks wrong or stays off, change
        the mapping — some boards swap the LED wires.
      </p>
      <div className="led-test">
        {CHANNELS.map(([channel, name]) => (
          <Button
            key={channel}
            className={`led-${channel.toLowerCase()}`}
            onClick={() =>
              preview(
                {
                  color: channel,
                  rgb_order: mapping.previewOrder,
                },
                `${name} preview sent`,
              )
            }
          >
            {name}
          </Button>
        ))}
      </div>
      <p className="setup-state" data-testid="led-completion">
        {completedAction ? `✓ ${completedAction}` : "No LED test run yet."}
      </p>
      <Button id="setup-led-remap-toggle" onClick={() => mapping.setRemapOpen(true)}>
        Change color mapping
      </Button>
      {mapping.remapOpen && (
        <div>
          <p className="setup-copy">
            Light one channel at a time. Tap the color you <strong>actually</strong> see.
          </p>
          <p className="setup-copy">
            <strong>Current mapping</strong>
          </p>
          <div className="led-map">
            {BYTE_LABELS.map((label, byte) => (
              <div key={label} className="led-chip-wrap">
                <div className={`led-chip led-${mapping.looks[byte].toLowerCase()}`}></div>
                <span className="led-chip-letter">{mapping.looks[byte]}</span>
              </div>
            ))}
          </div>
          {BYTE_LABELS.map((label, byte) => (
            <div key={label} className="led-byte" role="group" aria-label={label}>
              <p className="led-byte-label">{label}</p>
              <Button className="led-light" onClick={() => preview({ byte }, `${label} lit`)}>
                Light
              </Button>
              <p className="hint">Looks like</p>
              <div className="led-looks">
                {CHANNELS.map(([channel, name]) => (
                  <Button
                    key={channel}
                    className={clsx(
                      `led-${channel.toLowerCase()}`,
                      mapping.looks[byte] === channel && "active",
                    )}
                    aria-pressed={mapping.looks[byte] === channel}
                    onClick={() => pickLook(byte, channel)}
                  >
                    {name}
                  </Button>
                ))}
              </div>
            </div>
          ))}
        </div>
      )}
    </ConfigSection>
  );
}
