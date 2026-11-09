import { useEffect, useState } from "preact/hooks";
import type { TargetedSubmitEvent } from "preact";
import { api } from "../api/client";
import type { ActionDo, ActionRow, ActionsReply } from "../api/types";
import { Button } from "../components/Button";
import { ConfigSection } from "../components/ConfigSection";
import { ANIMATIONS } from "../lib/animations";
import { useStatus } from "../status/status";

const MAX_ACTIONS = 16;

const DO_OPTIONS: { value: ActionDo; label: string }[] = [
  { value: "sound", label: "Sound" },
  { value: "anim", label: "Anim" },
];

const EVENT_PRESETS = [
  { value: "button.down", label: "down" },
  { value: "button.up", label: "up" },
  { value: "button.click", label: "click" },
  { value: "button.double_click", label: "double" },
  { value: "button.long_click", label: "long" },
  { value: "button.*", label: "button.*" },
] as const;

function emptyRow(): ActionRow {
  return { on: "button.click", do: "sound", sound: "click" };
}

function normalizeRow(row: ActionRow): ActionRow {
  if (row.do === "anim") {
    return { on: row.on, do: "anim", anim: row.anim ?? "welcome" };
  }

  return { on: row.on, do: "sound", sound: row.sound === "bell" ? "bell" : "click" };
}

function summaryFor(row: ActionRow): string {
  if (row.do === "anim") {
    const match = ANIMATIONS.find(([name]) => name === row.anim);

    return match ? match[1] : (row.anim ?? "");
  }

  return row.sound === "bell" ? "Bell" : "Click";
}

export function Actions() {
  const { perform, setStatus } = useStatus();
  const [rows, setRows] = useState<ActionRow[]>([]);
  const [savedRows, setSavedRows] = useState<ActionRow[]>([]);
  const [loaded, setLoaded] = useState(false);
  const [loadError, setLoadError] = useState<string | null>(null);
  const [loadToken, setLoadToken] = useState(0);
  const [simulateType, setSimulateType] = useState("button.click");

  const dirty = JSON.stringify(rows) !== JSON.stringify(savedRows);

  useEffect(() => {
    const controller = new AbortController();

    void (async () => {
      try {
        const reply = await api<ActionsReply>("/settings/actions", {
          signal: controller.signal,
        });

        if (!reply.ok) {
          setLoadError(reply.data.error);
          setLoaded(false);

          return;
        }

        const next = reply.data.actions.map(normalizeRow);

        setRows(next);
        setSavedRows(next);
        setLoadError(null);
        setLoaded(true);
      } catch (error) {
        if (controller.signal.aborted) {
          return;
        }

        setLoadError(error instanceof Error ? error.message : "Load failed");
        setLoaded(false);
      }
    })();

    return () => controller.abort();
  }, [loadToken]);

  function updateRow(index: number, patch: Partial<ActionRow>) {
    setRows((previous) =>
      previous.map((row, i) => (i === index ? normalizeRow({ ...row, ...patch }) : row)),
    );
  }

  function setKind(index: number, kind: ActionDo) {
    if (kind === "anim") {
      updateRow(index, { do: "anim", anim: "welcome", sound: undefined });
    } else {
      updateRow(index, { do: "sound", sound: "click", anim: undefined });
    }
  }

  async function save(event: TargetedSubmitEvent<HTMLFormElement>) {
    event.preventDefault();

    await perform<ActionsReply>({
      path: "/settings/actions",
      request: {
        method: "PUT",
        json: { actions: rows },
      },
      pending: "Saving…",
      failure: "Save failed",
      done: "Actions saved.",
      onSuccess: (saved) => {
        const next = saved.actions.map(normalizeRow);

        setRows(next);
        setSavedRows(next);
      },
    });
  }

  async function simulate(typeOverride?: string) {
    if (dirty) {
      setStatus("Save before Test or Fire — robot uses saved rules", "err");

      return;
    }

    const type = (typeOverride ?? simulateType).trim();

    if (!type) {
      setStatus("Enter an event type", "err");

      return;
    }

    if (typeOverride) {
      setSimulateType(type);
    }

    await perform({
      path: "/actions/test",
      request: {
        method: "POST",
        json: { type },
      },
      pending: "Simulating…",
      failure: "Simulate failed",
      done: `Dispatched ${type}.`,
    });
  }

  if (loadError) {
    return (
      <main>
        <h1 className="page-title">Actions</h1>
        <p className="lede">{loadError}</p>
        <Button
          type="button"
          variant="primary"
          onClick={() => {
            setLoadError(null);
            setLoadToken((token) => token + 1);
          }}
        >
          Retry
        </Button>
      </main>
    );
  }

  if (!loaded) {
    return null;
  }

  return (
    <main>
      <h1 className="page-title">Actions</h1>
      <p className="lede">
        When an event fires, every matching rule runs. Use <code>*</code> as a wildcard in the event
        pattern.
      </p>

      <form onSubmit={save}>
        <ConfigSection title="Rules" apply="now">
          <p className="config-section-desc">
            {rows.length}/{MAX_ACTIONS} rules · save to apply on the robot
          </p>
          {dirty ? (
            <p className="hint">
              Unsaved edits — robot still uses saved rules. Save before Test or Fire.
            </p>
          ) : null}

          {rows.length === 0 ? (
            <p className="hint actions-empty">
              No rules yet. Add one, or pick a common event below.
            </p>
          ) : (
            <div className="actions-table" role="table" aria-label="Action rules">
              <div className="actions-table-head" role="row">
                <span role="columnheader">Event</span>
                <span role="columnheader">Action</span>
                <span role="columnheader">Detail</span>
                <span role="columnheader" className="actions-col-tools">
                  <span className="visually-hidden">Tools</span>
                </span>
              </div>
              {rows.map((row, index) => {
                const hasWildcard = row.on.includes("*");
                const testDisabled = dirty || hasWildcard;
                const testTitle = dirty
                  ? "Save before Test — robot uses saved rules"
                  : hasWildcard
                    ? "Use Simulate with a concrete event"
                    : `Simulate ${row.on}`;

                return (
                  <div key={index} className="actions-table-row" role="row">
                    <label className="actions-cell" role="cell">
                      <span className="actions-mobile-label">Event</span>
                      <input
                        className="actions-event-input"
                        type="text"
                        value={row.on}
                        placeholder="button.click"
                        maxLength={47}
                        spellcheck={false}
                        aria-label={`Event pattern for rule ${index + 1}`}
                        onInput={(event) => updateRow(index, { on: event.currentTarget.value })}
                      />
                    </label>
                    <label className="actions-cell" role="cell">
                      <span className="actions-mobile-label">Action</span>
                      <select
                        value={row.do}
                        aria-label={`Action kind for rule ${index + 1}`}
                        onChange={(event) => setKind(index, event.currentTarget.value as ActionDo)}
                      >
                        {DO_OPTIONS.map((option) => (
                          <option key={option.value} value={option.value}>
                            {option.label}
                          </option>
                        ))}
                      </select>
                    </label>
                    <label className="actions-cell" role="cell">
                      <span className="actions-mobile-label">Detail</span>
                      {row.do === "sound" ? (
                        <select
                          value={row.sound ?? "click"}
                          aria-label={`Sound for rule ${index + 1}`}
                          onChange={(event) =>
                            updateRow(index, {
                              sound: event.currentTarget.value as "bell" | "click",
                            })
                          }
                        >
                          <option value="click">Click</option>
                          <option value="bell">Bell</option>
                        </select>
                      ) : (
                        <select
                          value={row.anim ?? "welcome"}
                          aria-label={`Animation for rule ${index + 1}`}
                          onChange={(event) =>
                            updateRow(index, { anim: event.currentTarget.value })
                          }
                        >
                          {ANIMATIONS.map(([name, title]) => (
                            <option key={name} value={name}>
                              {title}
                            </option>
                          ))}
                        </select>
                      )}
                    </label>
                    <div className="actions-cell actions-cell-tools" role="cell">
                      <Button
                        type="button"
                        className="actions-icon-btn"
                        title={testTitle}
                        aria-label={`Test rule ${index + 1}: ${row.on} → ${summaryFor(row)}`}
                        disabled={testDisabled}
                        onClick={() => void simulate(row.on)}
                      >
                        Test
                      </Button>
                      <Button
                        type="button"
                        variant="danger"
                        className="actions-icon-btn"
                        title="Remove rule"
                        aria-label={`Remove rule ${index + 1}`}
                        onClick={() =>
                          setRows((previous) => previous.filter((_, i) => i !== index))
                        }
                      >
                        ×
                      </Button>
                    </div>
                  </div>
                );
              })}
            </div>
          )}

          <div className="actions-presets" aria-label="Common event patterns">
            <span className="actions-presets-label">Quick add</span>
            <div className="actions-chips">
              {EVENT_PRESETS.map((preset) => (
                <button
                  key={preset.value}
                  type="button"
                  className="actions-chip"
                  disabled={rows.length >= MAX_ACTIONS}
                  onClick={() =>
                    setRows((previous) => [
                      ...previous,
                      normalizeRow({ on: preset.value, do: "sound", sound: "click" }),
                    ])
                  }
                >
                  {preset.label}
                </button>
              ))}
            </div>
          </div>

          <div className="actions-toolbar">
            <Button
              type="button"
              variant="secondary"
              className="actions-toolbar-btn"
              disabled={rows.length >= MAX_ACTIONS}
              onClick={() => setRows((previous) => [...previous, emptyRow()])}
            >
              Add rule
            </Button>
            <Button type="submit" variant="primary" className="actions-toolbar-btn">
              Save
            </Button>
          </div>
        </ConfigSection>
      </form>

      <ConfigSection title="Simulate event" apply="now">
        <p className="config-section-desc">
          Run matching rules without publishing on WebSocket or serial. Unsaved edits are not used —
          Save first.
        </p>
        <div className="actions-chips actions-chips-simulate">
          {EVENT_PRESETS.filter((preset) => !preset.value.includes("*")).map((preset) => (
            <button
              key={preset.value}
              type="button"
              className={
                simulateType === preset.value ? "actions-chip actions-chip-active" : "actions-chip"
              }
              disabled={dirty}
              title={dirty ? "Save before Fire — robot uses saved rules" : undefined}
              onClick={() => void simulate(preset.value)}
            >
              {preset.label}
            </button>
          ))}
        </div>
        <div className="actions-simulate">
          <input
            type="text"
            value={simulateType}
            placeholder="button.double_click"
            spellcheck={false}
            aria-label="Event type to simulate"
            onInput={(event) => setSimulateType(event.currentTarget.value)}
          />
          <Button
            type="button"
            variant="primary"
            className="actions-toolbar-btn"
            disabled={dirty}
            title={dirty ? "Save before Fire — robot uses saved rules" : undefined}
            onClick={() => void simulate()}
          >
            Fire
          </Button>
        </div>
      </ConfigSection>
    </main>
  );
}
