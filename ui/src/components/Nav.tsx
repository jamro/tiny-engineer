import { Link, useLocation } from "wouter-preact";

const NAV = [
  ["/", "Home"],
  ["/animations", "Animations"],
  ["/tests", "Tests"],
  ["/servo", "Servo"],
  ["/extensions", "Extensions"],
  ["/config", "Config"],
  ["/api", "API"],
] as const;

export function Nav() {
  const [location] = useLocation();

  return (
    <nav>
      {NAV.map(([path, label]) => {
        const current = location === path;

        return (
          <Link
            key={path}
            href={path}
            className={current ? "active" : undefined}
            aria-current={current ? "page" : undefined}
          >
            {label}
          </Link>
        );
      })}
    </nav>
  );
}
