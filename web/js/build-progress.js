/**
 * Remember the assembly step the visitor was on.
 * Assembly restores that step when no #s= is set.
 * "Start from scratch" clears saved progress and opens Shop.
 */
(function (global) {
  const STEP_KEY = "tiny-engineer.build.assemblyStep";

  const read = (key) => {
    try {
      return global.localStorage?.getItem(key) ?? null;
    } catch {
      return null;
    }
  };

  const write = (key, value) => {
    try {
      if (value == null) global.localStorage?.removeItem(key);
      else global.localStorage?.setItem(key, String(value));
    } catch {
      /* private mode / quota — ignore */
    }
  };

  const saveAssemblyStep = (index) => {
    if (!Number.isInteger(index) || index < 0) return;
    write(STEP_KEY, index);
  };

  const loadAssemblyStep = () => {
    const raw = read(STEP_KEY);
    if (raw == null) return null;
    const index = Number(raw);
    return Number.isInteger(index) && index >= 0 ? index : null;
  };

  const clearAssemblyStep = () => write(STEP_KEY, null);

  /** Checklist / progress keys cleared when restarting the guide from scratch. */
  const RESTART_KEYS = [
    "tiny-engineer.build.stage",
    STEP_KEY,
    "tiny-engineer.shop.checked",
    "tiny-engineer.print.checked",
  ];

  /** Wipe saved progress and checklist marks, then open Shop. */
  const restartGuide = (root = "../") => {
    for (const key of RESTART_KEYS) write(key, null);
    global.location.href = root + "shop/";
  };

  // Older timeline scripts called saveStage. A missing function throws and
  // leaves the stage nav empty when that script is still cached.
  const saveStage = () => {};

  global.TinyEngineerBuild = {
    saveStage,
    saveAssemblyStep,
    loadAssemblyStep,
    clearAssemblyStep,
    restartGuide,
  };
})(window);
