#pragma once

// Run every Action whose `on` pattern matches `type`, in list order.
// Does not publish an outbound event envelope.
void actionsDispatch(const char* type);
