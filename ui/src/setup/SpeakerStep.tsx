import { useState } from "preact/hooks";
import { Button } from "../components/Button";
import { ConfigSection } from "../components/ConfigSection";
import { useStatus } from "../status/status";

export function SpeakerStep() {
  const { perform } = useStatus();
  const [played, setPlayed] = useState(false);

  async function play() {
    const result = await perform({
      path: "/setup/audio",
      pending: "Playing…",
      failure: "Playback failed",
      done: "Done.",
    });

    if (result?.ok) {
      setPlayed(true);
    }
  }

  return (
    <ConfigSection title="Speaker">
      <p className="setup-copy">
        Play the welcome clip. You should hear speech from the speaker. Skip if you want — you can
        test again later from Tests.
      </p>
      <Button id="setup-audio-play" variant="primary" onClick={play}>
        {played ? "Play again" : "Play"}
      </Button>
      {played && (
        <p className="setup-state" data-testid="speaker-completion">
          <strong>✓ Welcome clip played</strong>
        </p>
      )}
    </ConfigSection>
  );
}
