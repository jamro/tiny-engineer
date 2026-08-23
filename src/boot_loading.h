#pragma once

void bootShowProgress(
  int step,
  int totalSteps,
  const char* label
);

bool bootLoadingIsProgress();

void bootBeginSleepingFace();
bool bootSleepInertiaUsesServos();
void bootSnapSleepPose();
void bootRunSleepInertia();
