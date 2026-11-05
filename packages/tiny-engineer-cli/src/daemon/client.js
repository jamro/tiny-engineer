/** Stable barrel for the host-side daemon client API. */
export {
  writeYield,
  yieldActive,
  clearYield,
  writeDisable,
  disableActive,
  clearDisable,
  supervisorInstalled,
} from "./flags.js";

export {
  tryConnect,
  spawnDaemonProcess,
  startWrapperProcess,
  daemonServeArgs,
} from "./connect.js";

export { ensureDaemon } from "./ensure.js";

export {
  daemonRobotRequest,
  daemonPlay,
  subscribeOnce,
  followDaemonChannel,
} from "./rpc.js";

export { daemonStatusText } from "./status.js";

export {
  releaseDaemon,
  stopDaemon,
  shutdownDaemon,
  disableDaemon,
} from "./control.js";
