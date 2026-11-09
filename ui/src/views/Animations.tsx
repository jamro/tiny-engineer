import { useEffect, useState } from "preact/hooks";
import { api } from "../api/client";
import type { AnimationReply } from "../api/types";
import { Button } from "../components/Button";
import { ANIMATIONS } from "../lib/animations";
import { useStatus } from "../status/status";

export function Animations() {
  const { perform, setStatus } = useStatus();
  const [current, setCurrent] = useState("…");

  useEffect(() => {
    const controller = new AbortController();

    void (async () => {
      try {
        const reply = await api<AnimationReply>("/anim", { signal: controller.signal });

        if (reply.ok) {
          setCurrent(reply.data.animation);
        }
      } catch {
        // The badge keeps its last value.
      }
    })();

    return () => controller.abort();
  }, []);

  async function play(name: string) {
    await perform<AnimationReply>({
      path: "/anim",
      request: { method: "POST", params: { name } },
      pending: "Setting animation…",
      failure: "Failed",
      onSuccess: ({ animation }) => {
        setStatus(`Animation: ${animation}`, "ok");
        setCurrent(animation);
      },
    });
  }

  return (
    <section>
      <h2 className="page-title">Animations</h2>
      <p className="page-desc">
        Set the robot’s current gesture. Continuous animations keep going until changed or timed
        out.
      </p>
      <div id="anim-current" className="badge">
        Current: <strong>{current}</strong>
      </div>
      <div className="btn-grid" id="anim-buttons">
        {ANIMATIONS.map(([name, title, hint]) => (
          <Button key={name} onClick={() => play(name)}>
            <span className="btn-title">{title}</span>
            <span className="btn-hint">{hint}</span>
          </Button>
        ))}
      </div>
    </section>
  );
}
