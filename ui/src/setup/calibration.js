import { $, $$ } from "../dom.js";
import {
  cloneRanges,
  DEFAULT_SERVO_RANGES,
  onServoRangesChange,
  servoRanges,
} from "../servo-ranges.js";
import { clearStatus, isBusy, perform, setStatus } from "../status.js";

const JOINT_COPY = [
  "Pitch only. Watch cable slack to the head. Stop before the head hits the neck piece. Down is toward the laptop; up is away.",
  "Yaw left and right. Stop when the cables pull taut. Do not twist until the loom binds.",
  "Lowest is forearm horizontal. Highest is upper arm horizontal. On this servo, higher angle is up.",
  "Same physical stops, inverted scale: higher angle is down. Physical lowest (forearm horizontal) is toward the high end of the bar; physical highest (upper arm horizontal) is toward the low end.",
  "Rotate until the right hand sits over the bell at the extreme. Prefer a band symmetric about 90° (hint only — stock 40–130 is fine).",
];
const BODY_JOINT = 4;
const CENTER = 90;

const marker = $("#setup-calib-marker");
const readout = $("#setup-calib-angle");

async function postSetupServo(params) {
  const result = await perform({
    path: "/setup/servo",
    params,
    pending: "Moving servos…",
    failure: "Move failed",
  });

  if (result?.ok) {
    clearStatus();
  }

  return result;
}

// Servo step state: "horns" while the printed parts go on, then "ranges" to find each joint's limits.
export const calibration = {
  phase: "horns",
  joint: 0,
  angles: Array(DEFAULT_SERVO_RANGES.length).fill(CENTER),
  ranges: cloneRanges(servoRanges),

  get angle() {
    return this.angles[this.joint];
  },

  get valid() {
    return this.ranges.every(([min, max]) => min < max);
  },

  reset() {
    this.phase = "horns";
    this.joint = 0;
    this.angles.fill(CENTER);
    this.ranges = cloneRanges(servoRanges);
  },

  showAngle() {
    readout.textContent = String(this.angle);
    marker.style.left = `${(this.angle / 180) * 100}%`;
  },

  async nudge(delta) {
    const { joint } = this;
    const previous = this.angle;
    const next = Math.max(0, Math.min(180, previous + delta));

    if (isBusy() || next === previous) {
      return;
    }

    this.angles[joint] = next;
    this.showAngle();
    const result = await postSetupServo({ index: joint, angle: next });

    if (result?.ok) {
      return;
    }

    this.angles[joint] = previous;
    this.showAngle();
  },

  setLimit(bound) {
    const range = this.ranges[this.joint];

    if (bound === "min" && this.angle >= range[1]) {
      setStatus("Min must be less than max.", "err");

      return;
    }

    if (bound === "max" && this.angle <= range[0]) {
      setStatus("Max must be greater than min.", "err");

      return;
    }

    range[bound === "min" ? 0 : 1] = this.angle;
    this.render();
  },

  render() {
    const [min, max] = this.ranges[this.joint];
    const band = $("#setup-calib-band");

    band.style.left = `${(min / 180) * 100}%`;
    band.style.width = `${((max - min) / 180) * 100}%`;
    $("#setup-min-label").textContent = min;
    $("#setup-max-label").textContent = max;
    $("#setup-joint-copy").textContent = JOINT_COPY[this.joint];

    for (const tab of $$("#setup-joint-tabs [data-joint]")) {
      tab.classList.toggle("active", Number(tab.dataset.joint) === this.joint);
    }

    const symmetry = $("#setup-body-sym");

    symmetry.hidden = this.joint !== BODY_JOINT;
    symmetry.textContent = `Distance below 90°: ${CENTER - min}° · above 90°: ${max - CENTER}°`;
    $("#setup-next").disabled = !this.valid;
    this.showAngle();
  },
};

onServoRangesChange(() => {
  calibration.ranges = cloneRanges(servoRanges);
});

$("#setup-move-90").addEventListener("click", async () => {
  const result = await postSetupServo({ all: CENTER });

  if (result?.ok) {
    calibration.angles.fill(CENTER);
  }
});

for (const tab of $$("#setup-joint-tabs [data-joint]")) {
  tab.addEventListener("click", () => {
    if (isBusy()) {
      return;
    }

    calibration.joint = Number(tab.dataset.joint);
    calibration.render();
  });
}

for (const button of $$(".calib-nudge [data-nudge]")) {
  button.addEventListener("click", () => calibration.nudge(Number(button.dataset.nudge)));
}

$("#setup-set-min").addEventListener("click", () => calibration.setLimit("min"));
$("#setup-set-max").addEventListener("click", () => calibration.setLimit("max"));
$("#setup-reset-joint").addEventListener("click", () => {
  calibration.ranges[calibration.joint] = [...DEFAULT_SERVO_RANGES[calibration.joint]];
  calibration.render();
});
