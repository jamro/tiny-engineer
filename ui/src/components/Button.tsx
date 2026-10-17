import clsx from "clsx";
import type { ComponentProps } from "preact";
import { useStatus } from "../status/status";

type ButtonProps = ComponentProps<"button"> & { variant?: "primary" | "secondary" | "danger" };

// Every button locks while an action runs.
export function Button({ variant, className, disabled, ...props }: ButtonProps) {
  const { busy } = useStatus();

  return (
    <button
      type="button"
      {...props}
      className={clsx("btn", variant && `btn-${variant}`, className)}
      disabled={busy || Boolean(disabled)}
    />
  );
}
