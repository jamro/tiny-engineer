import { $ } from "./dom.js";
import { onServoRangesChange, servoRanges } from "./servo-ranges.js";
import { perform, setStatus } from "./status.js";

const servoSelect = $("#servo-index");
const slider = $("#servo-slider");
const angleInput = $("#servo-angle");
const rangeHint = $("#servo-range-hint");

const currentRange = () => servoRanges[Number(servoSelect.value)] ?? servoRanges[0];

function inRange(angle) {
  const [min, max] = currentRange();

  return angle >= min && angle <= max;
}

function currentMid() {
  const [min, max] = currentRange();

  return (min + max) / 2;
}

function updateRangeHint() {
  const [min, max] = currentRange();
  const safe = inRange(parseFloat(angleInput.value));

  rangeHint.textContent = safe
    ? `Safe range: ${min}–${max}°`
    : `Outside safe range — firmware clamps to ${min}–${max}°`;
  rangeHint.classList.toggle("warn", !safe);
}

function setAngle(angle) {
  slider.value = angle;
  angleInput.value = angle;
  updateRangeHint();
}

function refreshServoPage() {
  const [min, max] = currentRange();

  for (const input of [slider, angleInput]) {
    input.min = min;
    input.max = max;
  }

  const angle = parseFloat(angleInput.value);

  setAngle(inRange(angle) ? angle : currentMid());
  $("#servo-scale-min").textContent = `${min}°`;
  $("#servo-scale-mid").textContent = `${Math.round(currentMid())}°`;
  $("#servo-scale-max").textContent = `${max}°`;
}

async function moveServo() {
  const index = servoSelect.value;
  const angle = angleInput.value;
  const result = await perform({
    path: "/test/servo",
    params: { index, angle },
    pending: "Moving servo…",
    failure: "Move failed",
  });

  if (result?.ok) {
    setStatus(`Servo ${index} moved to ${angle}°.`, "ok");
  }
}

slider.addEventListener("input", () => {
  angleInput.value = slider.value;
  updateRangeHint();
});
angleInput.addEventListener("input", () => {
  slider.value = angleInput.value;
  updateRangeHint();
});
servoSelect.addEventListener("change", refreshServoPage);
$("#servo-center").addEventListener("click", () => {
  setAngle(currentMid());
  moveServo();
});
$("#servo-form").addEventListener("submit", (event) => {
  event.preventDefault();
  moveServo();
});

onServoRangesChange(refreshServoPage);
refreshServoPage();
