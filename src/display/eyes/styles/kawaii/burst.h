#pragma once

#include <cstdint>

#include "display/eyes/styles/kawaii/face.h"

namespace kawaii {

// Stars that burst out of the outer sides of the eyes on the bell's ding and
// swirl off-screen, trailing dots that shorten as the ring fades. `ring` is
// the bell's loudness (0..1); at 0 the burst is over. Appends to
// face.sparks, using the eyes' positions in `face`.
void addStarBurst(Face& face, uint32_t sinceDingMs, float ring);

}  // namespace kawaii
