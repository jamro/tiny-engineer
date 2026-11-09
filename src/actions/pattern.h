#pragma once

// Glob match: only metacharacter is `*` (any sequence, including empty).
// Matches against the event type string only.
bool actionsPatternMatch(const char* pattern, const char* type);
