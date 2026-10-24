import { act, cleanup, fireEvent, render, screen } from "@testing-library/preact";
import { afterEach, beforeEach, describe, expect, it, vi } from "vitest";
import { StatusBar } from "../src/components/StatusBar";
import { STATUS_OK_MS, StatusProvider, useStatus } from "../src/status/status";

function StatusControls() {
  const { setStatus } = useStatus();

  return (
    <div>
      <button type="button" onClick={() => setStatus("Saved.", "ok")}>
        Show ok
      </button>
      <button type="button" onClick={() => setStatus("Failed.", "err")}>
        Show err
      </button>
    </div>
  );
}

function renderStatus() {
  return render(
    <StatusProvider>
      <StatusBar />
      <StatusControls />
    </StatusProvider>,
  );
}

describe("status auto-dismiss", () => {
  beforeEach(() => {
    vi.useFakeTimers();
  });

  afterEach(() => {
    cleanup();
    vi.useRealTimers();
  });

  it("clears ok after 3 seconds", async () => {
    renderStatus();

    fireEvent.click(screen.getByRole("button", { name: "Show ok" }));
    expect(screen.getByRole("status").textContent).toBe("Saved.");

    await act(async () => {
      await vi.advanceTimersByTimeAsync(STATUS_OK_MS - 1);
    });
    expect(screen.getByRole("status").textContent).toBe("Saved.");

    await act(async () => {
      await vi.advanceTimersByTimeAsync(1);
    });
    expect(screen.queryByRole("status")).toBeNull();
  });

  it("keeps err until dismissed", async () => {
    renderStatus();

    fireEvent.click(screen.getByRole("button", { name: "Show err" }));
    expect(screen.getByRole("status").textContent).toBe("Failed.");

    await act(async () => {
      await vi.advanceTimersByTimeAsync(STATUS_OK_MS * 2);
    });
    expect(screen.getByRole("status").textContent).toBe("Failed.");

    fireEvent.click(screen.getByRole("button", { name: "Dismiss" }));
    expect(screen.queryByRole("status")).toBeNull();
  });

  it("cancels the ok timer when a newer status arrives", async () => {
    renderStatus();

    fireEvent.click(screen.getByRole("button", { name: "Show ok" }));
    await act(async () => {
      await vi.advanceTimersByTimeAsync(STATUS_OK_MS / 2);
    });
    fireEvent.click(screen.getByRole("button", { name: "Show err" }));

    await act(async () => {
      await vi.advanceTimersByTimeAsync(STATUS_OK_MS);
    });
    expect(screen.getByRole("status").textContent).toBe("Failed.");
  });
});
