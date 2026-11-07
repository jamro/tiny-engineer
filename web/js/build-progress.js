/**
 * Remember which build stage (and assembly step) the visitor was on.
 * /build/ redirects here; assembly restores its step when no #s= is set.
 */
(function (global) {
  const STAGE_KEY = "tiny-engineer.build.stage";
  const STEP_KEY = "tiny-engineer.build.assemblyStep";

  const STAGES = [
    { id: "shop", path: "shop/" },
    { id: "print", path: "print/" },
    { id: "flash", path: "flash/" },
    { id: "assembly", path: "assembly/" },
    { id: "setup", path: "setup/" },
  ];

  const byId = new Map(STAGES.map((stage) => [stage.id, stage]));
  /** Retired stages → current resume target. */
  const LEGACY = { connect: "flash", use: "setup" };

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

  const saveStage = (id) => {
    if (byId.has(id)) write(STAGE_KEY, id);
  };

  const loadStage = () => {
    const raw = read(STAGE_KEY);
    const id = LEGACY[raw] || raw;
    if (LEGACY[raw] && byId.has(id)) write(STAGE_KEY, id);
    return byId.has(id) ? id : "shop";
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

  /** Absolute-from-web-root path for a stage, using `root` like "../" or "./". */
  const stageHref = (id, root = "./") => {
    const stage = byId.get(id) || byId.get("shop");
    return root + stage.path;
  };

  /** Where /build/ should send the visitor. */
  const resumeHref = (root = "../") => stageHref(loadStage(), root);

  global.TinyEngineerBuild = {
    STAGES,
    saveStage,
    loadStage,
    saveAssemblyStep,
    loadAssemblyStep,
    clearAssemblyStep,
    stageHref,
    resumeHref,
  };
})(window);
