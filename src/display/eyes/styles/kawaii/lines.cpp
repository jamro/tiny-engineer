#include "display/eyes/styles/kawaii/lines.h"

#include "display/eyes/styles/kawaii/util.h"

namespace kawaii {

namespace {

constexpr float kPercent = 100.0f;

// Mouths move a touch before the sound they make, and the animator's easing
// adds a little lag; leading the track keeps the words looking in sync.
constexpr uint32_t kLeadMs = 40;

// "Hello, human. What are we building today?"
constexpr Viseme kHelloHuman[] = {
  // "Hel-lo"
  {0, Vowel::Ah, 0},
  {40, Vowel::Ah, 55},
  {80, Vowel::Ah, 65},
  {125, Vowel::Ah, 25},  // l
  {170, Vowel::Oo, 80},
  {230, Vowel::Oo, 85},
  {300, Vowel::Oo, 30},
  {330, Vowel::Oo, 0},
  // "hu-man"
  {470, Vowel::Oo, 35},
  {560, Vowel::Oo, 55},
  {620, Vowel::Oo, 20},
  {635, Vowel::Ah, 0},  // m
  {665, Vowel::Ah, 85},
  {705, Vowel::Ah, 80},
  {740, Vowel::Ah, 30},  // n
  {800, Vowel::Ah, 0},
  // pause (held shut), then "What are"
  {1300, Vowel::Oo, 0},
  {1340, Vowel::Oo, 20},
  {1390, Vowel::Ah, 90},
  {1430, Vowel::Ah, 85},
  {1460, Vowel::Ah, 10},  // t
  {1500, Vowel::Ah, 70},
  {1540, Vowel::Ah, 80},
  {1600, Vowel::Ah, 30},
  // "we buil-ding"
  {1660, Vowel::Oo, 35},
  {1720, Vowel::Ee, 45},
  {1770, Vowel::Ee, 15},
  {1800, Vowel::Ee, 0},  // b
  {1850, Vowel::Ee, 80},
  {1910, Vowel::Ee, 70},
  {1965, Vowel::Ee, 10},  // d
  {2010, Vowel::Ee, 60},
  {2080, Vowel::Ee, 45},
  {2140, Vowel::Ee, 5},  // t
  // "to-day?"
  {2200, Vowel::Oo, 40},
  {2260, Vowel::Oo, 65},
  {2300, Vowel::Oo, 5},  // d
  {2360, Vowel::Ah, 85},
  {2420, Vowel::Ah, 70},
  {2480, Vowel::Ee, 55},
  {2560, Vowel::Ee, 25},
  {2640, Vowel::Ee, 0},
};

// "Psst... human... you might want to take a look."
constexpr Viseme kPsstHuman[] = {
  // "Psst": lips together, then a small toothy hiss, shut on the t
  {100, Vowel::Ee, 0},
  {150, Vowel::Ee, 0},  // p
  {180, Vowel::Ee, 30},
  {230, Vowel::Ee, 35},
  {560, Vowel::Ee, 25},
  {620, Vowel::Ee, 0},  // t
  // "hu-man"
  {650, Vowel::Oo, 25},
  {760, Vowel::Oo, 50},
  {840, Vowel::Oo, 35},
  {865, Vowel::Ah, 0},  // m
  {900, Vowel::Ah, 85},
  {930, Vowel::Ah, 75},
  {970, Vowel::Ah, 20},  // n
  {1010, Vowel::Ah, 0},
  // pause (held shut), then "you"
  {1480, Vowel::Oo, 0},
  {1520, Vowel::Oo, 30},
  {1600, Vowel::Oo, 45},
  {1650, Vowel::Oo, 15},
  {1665, Vowel::Ah, 0},  // m
  // "might"
  {1700, Vowel::Ah, 40},
  {1780, Vowel::Ah, 90},
  {1830, Vowel::Ah, 70},
  {1890, Vowel::Ah, 15},
  {1925, Vowel::Ah, 0},  // t
  // "want"
  {1960, Vowel::Oo, 30},  // w
  {2040, Vowel::Ah, 80},
  {2080, Vowel::Ah, 75},
  {2120, Vowel::Ah, 10},
  {2145, Vowel::Ah, 0},  // t
  // "to"
  {2200, Vowel::Oo, 45},
  {2230, Vowel::Oo, 40},
  {2260, Vowel::Oo, 0},  // t
  // "take a"
  {2320, Vowel::Ah, 70},
  {2360, Vowel::Ee, 70},
  {2400, Vowel::Ee, 40},
  {2445, Vowel::Ee, 0},  // k
  {2490, Vowel::Ah, 35},
  {2510, Vowel::Ah, 25},
  // "look"
  {2540, Vowel::Oo, 60},
  {2600, Vowel::Oo, 50},
  {2650, Vowel::Oo, 75},
  {2700, Vowel::Oo, 50},
  {2760, Vowel::Oo, 0},  // k
};

// "Uh-oh. Human, we have a problem."
constexpr Viseme kUhOhHuman[] = {
  // "Uh-oh"
  {0, Vowel::Ah, 0},
  {30, Vowel::Ah, 70},
  {80, Vowel::Ah, 65},
  {125, Vowel::Ah, 15},
  {150, Vowel::Ah, 0},  // glottal stop
  {200, Vowel::Oo, 60},
  {260, Vowel::Oo, 70},
  {380, Vowel::Oo, 45},
  {440, Vowel::Oo, 10},
  {480, Vowel::Oo, 0},
  // "hu-man": a breathy h, then the u
  {540, Vowel::Oo, 15},
  {700, Vowel::Oo, 25},
  {790, Vowel::Oo, 55},
  {870, Vowel::Oo, 30},
  {900, Vowel::Ah, 0},  // m
  {935, Vowel::Ah, 80},
  {970, Vowel::Ah, 70},
  {1010, Vowel::Ah, 20},  // n
  {1060, Vowel::Ah, 0},
  // a breath (held shut), then "we have a"
  {1290, Vowel::Ee, 0},
  {1340, Vowel::Ee, 55},
  {1380, Vowel::Ee, 45},
  {1420, Vowel::Ah, 25},  // h
  {1490, Vowel::Ah, 70},
  {1520, Vowel::Ah, 50},
  {1550, Vowel::Ah, 10},  // v
  {1590, Vowel::Ah, 60},
  {1615, Vowel::Ah, 40},
  {1640, Vowel::Ah, 0},  // p
  {1690, Vowel::Ah, 0},
  // "pro-blem"
  {1720, Vowel::Oo, 40},
  {1790, Vowel::Ah, 90},
  {1840, Vowel::Ah, 75},
  {1880, Vowel::Ah, 10},
  {1905, Vowel::Ah, 0},  // b
  {1960, Vowel::Ee, 50},
  {2005, Vowel::Ee, 65},
  {2060, Vowel::Ee, 30},
  {2110, Vowel::Ee, 0},  // m
};

// "Fine. I didn't want to finish it anyway."
constexpr Viseme kFineAnyway[] = {
  // "Fine": lip on teeth, then "fa-in"
  {0, Vowel::Ee, 0},
  {60, Vowel::Ee, 10},  // f
  {190, Vowel::Ee, 10},
  {230, Vowel::Ah, 80},
  {300, Vowel::Ah, 70},
  {360, Vowel::Ee, 45},
  {440, Vowel::Ee, 15},
  {500, Vowel::Ee, 0},
  // a sigh through slightly parted lips
  {620, Vowel::Oo, 15},
  {850, Vowel::Oo, 15},
  {880, Vowel::Oo, 0},
  // "I di-dn't"
  {910, Vowel::Ah, 80},
  {950, Vowel::Ee, 60},
  {990, Vowel::Ee, 10},
  {1020, Vowel::Ee, 0},  // d
  {1065, Vowel::Ee, 50},
  {1100, Vowel::Ee, 30},
  {1130, Vowel::Ee, 0},  // d
  {1200, Vowel::Ee, 15},  // n
  {1260, Vowel::Ee, 5},
  // "want to"
  {1300, Vowel::Oo, 30},  // w
  {1350, Vowel::Ah, 80},
  {1385, Vowel::Ah, 75},
  {1420, Vowel::Ah, 15},
  {1450, Vowel::Ah, 0},  // t
  {1500, Vowel::Oo, 20},
  {1550, Vowel::Oo, 40},
  {1580, Vowel::Oo, 5},
  // "fi-nish it"
  {1620, Vowel::Ee, 10},  // f
  {1700, Vowel::Ee, 55},
  {1740, Vowel::Ee, 20},  // n
  {1800, Vowel::Ee, 45},
  {1860, Vowel::Oo, 20},  // sh
  {1920, Vowel::Oo, 20},
  {1955, Vowel::Ee, 50},
  {1985, Vowel::Ee, 20},
  {2005, Vowel::Ee, 0},  // t
  // "a-ny-way"
  {2050, Vowel::Ah, 35},
  {2110, Vowel::Ee, 55},
  {2160, Vowel::Ee, 25},
  {2200, Vowel::Oo, 35},  // w
  {2250, Vowel::Ah, 40},
  {2300, Vowel::Ah, 20},
  {2345, Vowel::Ah, 60},
  {2400, Vowel::Ee, 45},
  {2460, Vowel::Ee, 15},
  {2500, Vowel::Ee, 0},
};

// "Insufficient resources... shutting doooowwwnnn"
constexpr Viseme kShuttingDown[] = {
  // "In-suf-fi-cient"
  {60, Vowel::Ee, 0},
  {110, Vowel::Ee, 40},
  {170, Vowel::Ee, 35},
  {220, Vowel::Ee, 10},  // n
  {260, Vowel::Ee, 20},  // s
  {310, Vowel::Ee, 20},
  {350, Vowel::Ah, 60},
  {380, Vowel::Ah, 30},
  {410, Vowel::Ee, 10},  // f
  {460, Vowel::Ee, 10},
  {510, Vowel::Ee, 55},
  {550, Vowel::Ee, 40},
  {590, Vowel::Oo, 20},  // sh
  {650, Vowel::Oo, 20},
  {690, Vowel::Ee, 30},
  {730, Vowel::Ee, 10},
  {745, Vowel::Ee, 0},  // t
  // "re-sour-ces"
  {790, Vowel::Oo, 25},  // r
  {840, Vowel::Ee, 55},
  {900, Vowel::Ee, 40},
  {940, Vowel::Ee, 15},
  {980, Vowel::Ee, 20},  // s
  {1030, Vowel::Ee, 20},
  {1080, Vowel::Oo, 70},
  {1110, Vowel::Oo, 50},
  {1150, Vowel::Ee, 25},
  {1200, Vowel::Ee, 20},  // s
  {1270, Vowel::Ee, 20},
  {1330, Vowel::Ee, 50},
  {1390, Vowel::Ee, 35},
  {1440, Vowel::Ee, 15},
  {1480, Vowel::Ee, 20},  // s
  {1550, Vowel::Ee, 15},
  {1590, Vowel::Ee, 0},
  // a breath (held shut), then "shut-ting"
  {1770, Vowel::Oo, 0},
  {1820, Vowel::Oo, 25},  // sh
  {1880, Vowel::Oo, 30},
  {1925, Vowel::Ah, 80},
  {1970, Vowel::Ah, 70},
  {2005, Vowel::Ah, 10},
  {2030, Vowel::Ah, 0},  // tt
  {2070, Vowel::Ee, 15},
  {2110, Vowel::Ee, 50},
  {2200, Vowel::Ee, 40},
  {2280, Vowel::Ee, 15},
  {2305, Vowel::Ee, 0},  // d
  // "doooowwwwnnnn": wide open, rounding as it winds down, then a fading n
  {2360, Vowel::Ah, 85},
  {2480, Vowel::Ah, 90},
  {2560, Vowel::Oo, 75},
  {2700, Vowel::Oo, 60},
  {2790, Vowel::Oo, 45},
  {2850, Vowel::Ee, 20},
  {3000, Vowel::Ee, 10},
  {3060, Vowel::Ee, 0},
};

constexpr size_t kMaxCues = 4;

struct Script {
  const Viseme* track;
  size_t count;
  uint32_t stockCues[kMaxCues];  // the stock clip's .cue values, in field order
  size_t cueCount;
};

template <size_t N>
constexpr Script script(
  const Viseme (&track)[N], uint32_t a, uint32_t b, uint32_t c, uint32_t d, size_t cueCount
) {
  return {track, N, {a, b, c, d}, cueCount};
}

// Indexed by Line. Stock cues are the defaults in audio_cues.cpp.
constexpr Script kScripts[] = {
  script(kHelloHuman, 800, 1340, 2400, 2680, 4),  // greeting, pause, question, end
  script(kPsstHuman, 640, 1540, 2960, 0, 3),      // pst, human, end
  script(kUhOhHuman, 460, 1060, 2140, 2220, 4),   // uhoh, human, problem, end
  script(kFineAnyway, 520, 1350, 2050, 2500, 4),  // fine, didnt_want, finish, end
  script(kShuttingDown, 1700, 3410, 0, 0, 2),     // shutdown, end
};

static_assert(
  sizeof(kScripts) / sizeof(kScripts[0]) == (size_t)Line::Count,
  "every line needs a script"
);

}  // namespace

Voice mouthLine(const Viseme* track, size_t count, uint32_t elapsedMs) {
  if (count == 0 || elapsedMs < track[0].atMs || elapsedMs >= track[count - 1].atMs) {
    return {0.0f, count > 0 ? track[count - 1].vowel : Vowel::Ah};
  }

  size_t i = 0;
  while (i + 2 < count && elapsedMs >= track[i + 1].atMs) {
    i++;
  }
  const Viseme& from = track[i];
  const Viseme& to = track[i + 1];
  const float t = (float)(elapsedMs - from.atMs) / (float)(to.atMs - from.atMs);
  const float level = lerp(from.openPct, to.openPct, t) / kPercent;
  // Opening shapes toward the next sound; closing keeps the current one.
  const Vowel vowel = to.openPct > from.openPct ? to.vowel : from.vowel;
  return {level, vowel};
}

Voice say(Line line, uint32_t elapsedMs) {
  const size_t index = (size_t)line;
  if (index >= (size_t)Line::Count) {
    return {0.0f, Vowel::Ah};
  }
  const Script& s = kScripts[index];
  return mouthLine(s.track, s.count, elapsedMs + kLeadMs);
}

bool fitsStock(Line line, const uint32_t* cues, size_t count) {
  const size_t index = (size_t)line;
  if (index >= (size_t)Line::Count || cues == nullptr || count != kScripts[index].cueCount) {
    return false;
  }
  for (size_t i = 0; i < count; i++) {
    if (cues[i] != kScripts[index].stockCues[i]) {
      return false;
    }
  }
  return true;
}

}  // namespace kawaii
