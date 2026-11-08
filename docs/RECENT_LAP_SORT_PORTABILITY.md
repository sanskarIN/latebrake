# Recent-lap pace: strict C++ compiler portability

This is an isolated production-source fix for the up-to-eight lap summary
shown after a completed race.

On the experimental circuit branch, GCC 15 optimized a general std::sort
on an eight-double array and issued -Werror=array-bounds inside
libstdc++'s introsort implementation. The existing count was already
validated to 2..8. An actual out-of-range game access has NOT been
demonstrated, but the compiler diagnostic blocks strict portable CI.

The fixed, 2..8-entry occupied prefix now uses a bounded insertion sort,
whose size and indices are explicitly checked. There is no heap allocation,
new score authority or change to lap time/reward/persistence. Stable ordering
does not change the median, average, population standard deviation,
fastest/slowest or improvement streak.

The real RaceProgress-based test includes each count from 2 through 8
with unordered and tied durations. Strict C++20 Linux/macOS/Windows CI
must pass on the exact PR commit before merging.

This patch is independent of the future 1.70km circuit and experimental
AI/Chaos work. It does not produce a real Unreal build or playable executable.
