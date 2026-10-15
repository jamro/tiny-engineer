import { Link } from "wouter-preact";
import { HealthStrip } from "./HealthStrip";

const CARDS = [
  ["/animations", "Animations", "Pick a gesture — typing, reading, thinking, talking, and more."],
  ["/servo", "Servo control", "Move servos and save limits and centers."],
  ["/tests", "Hardware tests", "Try the speaker, screen, LEDs, and servo sweep."],
  [
    "/config",
    "Config",
    "Device name, timeouts, volume, eyes style, boot behavior, serial logging, API token, saved WiFi networks, and hand servo assignment. Servo limits and centers live on Servo control. Screen orientation and RGB mapping are retuned in setup mode after factory reset.",
  ],
  ["/api", "API reference", "Full endpoint list, parameters, and curl-friendly docs."],
] as const;

export function Home() {
  return (
    <section>
      <div className="hero">
        <h1>Tiny Engineer</h1>
        <p>A desk robot that acts out your AI coding assistant while it works.</p>
      </div>
      <HealthStrip />
      <div className="cards">
        {CARDS.map(([href, title, text]) => (
          <Link key={href} className="card" href={href}>
            <h3>{title}</h3>
            <p>{text}</p>
          </Link>
        ))}
        <a
          className="card card-github"
          href="https://github.com/jamro/tiny-engineer"
          target="_blank"
          rel="noopener"
        >
          <h3>GitHub docs →</h3>
          <p>Build guide, wiring, and full project docs.</p>
        </a>
      </div>
    </section>
  );
}
