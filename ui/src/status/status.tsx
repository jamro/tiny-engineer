import { type ComponentChildren, createContext } from "preact";
import { useCallback, useContext, useEffect, useMemo, useRef, useState } from "preact/hooks";
import { api, type ApiResult, type Reply, type Request } from "../api/client";

type StatusType = "loading" | "ok" | "err";

export const STATUS_OK_MS = 3000;

export interface Action<T extends Reply> {
  path: string;
  request?: Request;
  // Shown while the request runs. Without it the action is a quick preview: it neither shows
  // progress nor locks the controls, and it is dropped while a locking action runs.
  pending?: string;
  failure: string;
  // Shown on success; without it the status bar clears.
  done?: string;
  // Runs before the controls unlock, for follow-up requests that belong to the same action.
  onSuccess?: (data: T) => unknown;
}

export interface StatusMessage {
  message: string;
  type?: StatusType;
}

interface StatusApi {
  busy: boolean;
  setStatus: (message: string, type?: StatusType) => void;
  clearStatus: () => void;
  // Resolves to the result, or null when another action is running or the request failed to send.
  perform: <T extends Reply = Reply>(action: Action<T>) => Promise<ApiResult<T> | null>;
}

const StatusContext = createContext<StatusApi | null>(null);
// Separate so a new message re-renders the status bar alone, not every button.
const StatusMessageContext = createContext<StatusMessage | null>(null);

export const useStatusMessage = () => useContext(StatusMessageContext);

export function useStatus(): StatusApi {
  const value = useContext(StatusContext);

  if (!value) {
    throw new Error("useStatus outside StatusProvider");
  }

  return value;
}

export function StatusProvider({ children }: { children: ComponentChildren }) {
  const [statusMessage, setStatusMessage] = useState<StatusMessage | null>(null);
  const [busy, setBusy] = useState(false);
  // The ref blocks a second action in the same tick, before the busy state renders.
  const runningRef = useRef(false);
  const okTimerRef = useRef<ReturnType<typeof setTimeout> | null>(null);
  const statusGenerationRef = useRef(0);

  const clearOkTimer = useCallback(() => {
    if (okTimerRef.current !== null) {
      clearTimeout(okTimerRef.current);
      okTimerRef.current = null;
    }
  }, []);

  const clearStatus = useCallback(() => {
    clearOkTimer();
    statusGenerationRef.current += 1;
    setStatusMessage(null);
  }, [clearOkTimer]);

  const setStatus = useCallback(
    (message: string, type?: StatusType) => {
      clearOkTimer();
      const generation = ++statusGenerationRef.current;

      setStatusMessage({ message, type });

      if (type === "ok") {
        okTimerRef.current = setTimeout(() => {
          if (statusGenerationRef.current === generation) {
            setStatusMessage(null);
          }

          okTimerRef.current = null;
        }, STATUS_OK_MS);
      }
    },
    [clearOkTimer],
  );

  useEffect(() => () => clearOkTimer(), [clearOkTimer]);

  const perform = useCallback(
    async <T extends Reply = Reply>({
      path,
      request = { method: "POST" },
      pending,
      failure,
      done,
      onSuccess,
    }: Action<T>) => {
      if (runningRef.current) {
        return null;
      }

      const locks = pending !== undefined;

      if (locks) {
        runningRef.current = true;
        setBusy(true);
        setStatus(pending, "loading");
      }

      try {
        const result = await api<T>(path, request);

        if (!result.ok) {
          setStatus(result.data.error || failure, "err");

          return result;
        }

        if (done) {
          setStatus(done, "ok");
        } else {
          clearStatus();
        }

        await onSuccess?.(result.data);

        return result;
      } catch {
        setStatus("Network error", "err");

        return null;
      } finally {
        if (locks) {
          runningRef.current = false;
          setBusy(false);
        }
      }
    },
    [setStatus, clearStatus],
  );

  const value = useMemo(
    () => ({ busy, setStatus, clearStatus, perform }),
    [busy, setStatus, clearStatus, perform],
  );

  return (
    <StatusContext.Provider value={value}>
      <StatusMessageContext.Provider value={statusMessage}>
        {children}
      </StatusMessageContext.Provider>
    </StatusContext.Provider>
  );
}
