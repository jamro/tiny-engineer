import { type ComponentChildren, createContext } from "preact";
import { useCallback, useContext, useEffect, useMemo, useState } from "preact/hooks";
import { api, token } from "./api/client";
import type { AuthInfo, Settings } from "./api/types";
import { useStatus } from "./status/status";

type Gate = "booting" | "auth" | "reboot" | null;
export type SettingsLoad = "loaded" | "unauthorized" | "offline";

interface RobotApi {
  gate: Gate;
  // Setup AP mode, or no WiFi saved yet: the config page becomes the setup wizard.
  inSetup: boolean;
  version: string | null;
  settings: Settings | null;
  applySettings: (settings: Settings) => void;
  unlock: (candidate: string) => Promise<SettingsLoad>;
  showRebootGate: () => void;
  leaveSetup: () => void;
}

const RobotContext = createContext<RobotApi | null>(null);

export function useRobot(): RobotApi {
  const value = useContext(RobotContext);

  if (!value) {
    throw new Error("useRobot outside RobotProvider");
  }

  return value;
}

export function RobotProvider({ children }: { children: ComponentChildren }) {
  const { setStatus } = useStatus();
  const [gate, setGate] = useState<Gate>("booting");
  const [provisioning, setProvisioning] = useState(false);
  const [linkMode, setLinkMode] = useState<string>("");
  const [version, setVersion] = useState<string | null>(null);
  const [settings, setSettings] = useState<Settings | null>(null);

  const applySettings = useCallback((next: Settings) => {
    setSettings(next);
    setLinkMode(next.link_mode);
  }, []);

  // GET /settings also tells whether the current token is accepted.
  const loadSettings = useCallback(async (): Promise<SettingsLoad> => {
    try {
      const reply = await api<Settings>("/settings");

      if (reply.ok) {
        applySettings(reply.data);

        return "loaded";
      }

      return reply.data.error === "unauthorized" ? "unauthorized" : "offline";
    } catch {
      return "offline";
    }
  }, [applySettings]);

  const unlock = useCallback(
    async (candidate: string) => {
      token.set(candidate);

      const loaded = await loadSettings();

      if (loaded === "loaded") {
        setGate(null);
      }

      if (loaded === "unauthorized") {
        token.set("");
      }

      return loaded;
    },
    [loadSettings],
  );

  useEffect(() => {
    void (async () => {
      let auth: AuthInfo | null = null;

      try {
        const reply = await api<AuthInfo>("/auth");

        auth = reply.ok ? reply.data : null;
      } catch {
        // The settings request below shows whether the robot answers at all.
      }

      if (auth) {
        setVersion(auth.version);
        setProvisioning(auth.provisioning);
        setLinkMode(auth.link_mode);
      }

      if (auth?.required && !token.get()) {
        setGate("auth");

        return;
      }

      const loaded = await loadSettings();

      if (loaded === "unauthorized") {
        token.set("");
      }

      // A stored token that could not be checked is kept, and asked for again.
      if (auth?.required && loaded !== "loaded") {
        setGate("auth");

        return;
      }

      if (loaded === "offline") {
        setStatus("Could not load settings", "err");
      }

      setGate(null);
    })();
  }, [loadSettings, setStatus]);

  const provisioned = linkMode === "wifi" || linkMode === "usb";

  const value = useMemo<RobotApi>(
    () => ({
      gate,
      inSetup: provisioning || !provisioned,
      version,
      settings,
      applySettings,
      unlock,
      showRebootGate: () => setGate("reboot"),
      leaveSetup: () => {
        setProvisioning(false);
        setLinkMode((prev) => (prev === "usb" ? "usb" : "wifi"));
      },
    }),
    [gate, provisioning, provisioned, version, settings, applySettings, unlock],
  );

  return <RobotContext.Provider value={value}>{children}</RobotContext.Provider>;
}
