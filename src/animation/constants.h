#pragma once

#include <cstdint>

namespace anim {

// Pose units are -1..1 (saved min / mid / max). Speeds stay deg/s.

constexpr float TYPING_HAND_BAND = 1.0f / 3.0f;
constexpr float TYPING_HEAD_BAND = 2.0f / 7.0f;
constexpr float TYPING_SWAY = 1.0f / 9.0f;
constexpr float READING_HEAD_BAND = 2.0f / 7.0f;
constexpr float READING_NECK_SWAY = 2.0f / 9.0f;

constexpr float TRANSITION_TORSO_SPEED_DEG_S = 25.0f;
constexpr float TRANSITION_HAND_SPEED_DEG_S = 90.0f;

constexpr float TYPING_RIGHT_LOW = -1.0f;
constexpr float TYPING_RIGHT_HIGH = TYPING_RIGHT_LOW + TYPING_HAND_BAND;
constexpr float TYPING_LEFT_HIGH = 1.0f;
constexpr float TYPING_LEFT_LOW = TYPING_LEFT_HIGH - TYPING_HAND_BAND;

constexpr float TYPING_HEAD_LOW = -1.0f;
constexpr float TYPING_HEAD_HIGH = TYPING_HEAD_LOW + TYPING_HEAD_BAND;

constexpr float TYPING_BODY_MID = 0.0f;
constexpr float TYPING_NECK_MID = 0.0f;

constexpr float READING_HEAD_LOW = -1.0f;
constexpr float READING_HEAD_HIGH = READING_HEAD_LOW + READING_HEAD_BAND;
constexpr float READING_NECK_MID = 0.0f;

constexpr float RING_HEAD_START = 2.0f / 7.0f;
constexpr float RING_BODY_START = -1.0f;
constexpr float RING_RIGHT_START = 1.0f;
constexpr float RING_LEFT_START = 7.0f / 9.0f;
constexpr float RING_NECK_START = 0.0f;
constexpr float RING_RIGHT_STRIKE = -8.0f / 9.0f;
constexpr float RING_RIGHT_BOUNCE = -5.0f / 9.0f;
constexpr float RING_HEAD_STRIKE = -1.0f;

constexpr float WELCOME_HEAD_UP = 2.0f / 7.0f;
constexpr float WELCOME_HEAD_MID = 0.0f;
constexpr float WELCOME_HAND_REST = -1.0f;
constexpr float WELCOME_HAND_RAISED = 1.0f;
constexpr float WELCOME_HAND_WIGGLE = 8.0f / 45.0f;
constexpr float WELCOME_HEAD_NOD = 3.0f / 35.0f;

constexpr float ATTENTION_BODY_MID = 0.0f;
constexpr float ATTENTION_NECK_MID = 0.0f;
constexpr float ATTENTION_HEAD_READY = 6.0f / 35.0f;
constexpr float ATTENTION_HAND_LEFT_PARKED = 1.0f;
constexpr float ATTENTION_HAND_RIGHT_REST = -1.0f;
constexpr float ATTENTION_HAND_RIGHT_RAISED = 1.0f / 15.0f;
constexpr float ATTENTION_HAND_POINT = 1.0f / 3.0f;
constexpr float ATTENTION_HUMAN_NECK_GLANCE = 4.0f / 45.0f;
constexpr float ATTENTION_HUMAN_HEAD_NOD = 2.0f / 35.0f;
constexpr float ATTENTION_LOOK_NECK_GLANCE = 2.0f / 15.0f;
constexpr float ATTENTION_LOOK_HEAD_NOD = 2.0f / 35.0f;
constexpr float ATTENTION_WAIT_HEAD_NOD = 3.0f / 35.0f;
constexpr float ATTENTION_WAIT_NECK_TILT = 4.0f / 45.0f;
constexpr float ATTENTION_WAIT_HAND_WAVE = 7.0f / 45.0f;

// Attention/error wait after audio before auto-finish to none.
constexpr uint32_t NON_CONTINUOUS_HOLD_MS = 60UL * 1000UL;

constexpr float ERROR_BODY_TASK_SIDE = -8.0f / 45.0f;
constexpr float ERROR_NECK_TASK_SIDE = -2.0f / 9.0f;
constexpr float ERROR_HEAD_CONCERNED = -12.0f / 35.0f;
constexpr float ERROR_HAND_RIGHT_PRESENT = 13.0f / 45.0f;
constexpr float ERROR_HAND_LEFT_TASK_POINT = 23.0f / 45.0f;
constexpr float ERROR_HELP_NECK_GLANCE = 2.0f / 9.0f;
constexpr float ERROR_AWAY_NECK_GLANCE = 4.0f / 45.0f;
constexpr float ERROR_HELP_HEAD_GLANCE = 3.0f / 35.0f;
constexpr float ERROR_WORRY_HEAD_NOD = 2.0f / 35.0f;

constexpr float ABORT_BODY_DISMISSIVE = -1.0f / 9.0f;
constexpr float ABORT_NECK_SIDE = 14.0f / 45.0f;
constexpr float ABORT_NECK_OTHER_SIDE = -2.0f / 15.0f;
constexpr float ABORT_HEAD_UP = 16.0f / 35.0f;
constexpr float ABORT_HEAD_DIP = 1.0f / 7.0f;
constexpr float ABORT_HEAD_DIDNT_WANT = 12.0f / 35.0f;
constexpr float ABORT_HEAD_ANYWAY = 18.0f / 35.0f;

// Idle sleep / boot sleep-inertia: chin down, then rise to mid on wake.
constexpr float SLEEP_HEAD_DOWN = -4.0f / 7.0f;
constexpr float SLEEP_HEAD_AWAKE = 0.0f;
constexpr float ABORT_HAND_RIGHT_UP = 1.0f;
constexpr float ABORT_HAND_RIGHT_SHRUG = 7.0f / 9.0f;
constexpr float ABORT_HAND_LEFT_UP = -2.0f / 9.0f;
constexpr float ABORT_HAND_LEFT_SHRUG = 2.0f / 45.0f;

// Power-loss collapse: chin fully down, hands parked.
constexpr float DEAD_HEAD_DOWN = -1.0f;
constexpr float DEAD_COLLAPSE_SPEED_DEG_S = 48.0f;

}  // namespace anim
