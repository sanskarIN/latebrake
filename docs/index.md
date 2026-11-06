<div class="lb-hero">
  <div class="lb-eyebrow">OPEN-SOURCE RACING PROJECT · UNREAL ENGINE 5.8 · C++20</div>
  <h1>Race Beyond <span>Limits.</span></h1>
  <p>Build the foundations of a racing game with us: a physics-driven prototype, ordered circuit racing, career progression and a portable C++ core.</p>
  <div class="lb-hero-actions">
    <a class="lb-button lb-button-primary" href="getting-started/">Explore the source <span aria-hidden="true">↗</span></a>
    <a class="lb-button lb-button-secondary" href="features/">See the systems</a>
  </div>
  <div class="lb-hero-grid">
    <div><b>4</b><span>ordered circuit gates</span></div>
    <div><b>12</b><span>career events in source</span></div>
    <div><b>6</b><span>fictional garage vehicles</span></div>
    <div><b>C++20</b><span>portable test core</span></div>
  </div>
</div>

<div class="lb-status-panel" role="status">
  <div class="lb-status-title">Development preview — source only</div>
  <p>Latebrake is <strong>not a verified playable release</strong>. The latest GitHub source tag is <a href="https://github.com/sanskarIN/latebrake/releases/tag/v0.5.92">v0.5.92</a> at `13f5588b64462a6dcdcb7d7406c584c63837e87b`: an <strong>experimental source-only pre-release from unmerged draft PR #295 with zero executable attachments</strong>. It refines suspension, braking and shared acceleration/cornering grip. These physics changes are <strong>not verified Unreal runtime and are not merged into protected main</strong>. The previous experimental snapshot is v0.5.91; the preceding main-line tag is v0.5.90. <strong>UE5.8/UHT compilation, a cooked original map, installed Windows EXE and Android APK/AAB remain unverified.</strong> Source tests and documentation deployments are not installed driving QA.</p>
  <a href="development-status/">Read the verification status →</a>
</div>

## Explore Latebrake

<div class="lb-card-grid">
  <a class="lb-card" href="about/"><span class="lb-card-label">01 / PROJECT</span><strong>What is Latebrake?</strong><p>Its vision, current scope, and the distinction between implemented source and validated gameplay.</p><span>Project overview →</span></a>
  <a class="lb-card" href="features/"><span class="lb-card-label">02 / GAMEPLAY</span><strong>Race systems</strong><p>Starter circuit, car controls, experimental rival, offline garage, championships and challenges.</p><span>Explore features →</span></a>
  <a class="lb-card" href="getting-started/"><span class="lb-card-label">03 / DEVELOPMENT</span><strong>Build and test</strong><p>Repository setup, Unreal Engine 5.8 prerequisites, portable CMake tests and documented limitations.</p><span>Developer guide →</span></a>
  <a class="lb-card" href="contributing/"><span class="lb-card-label">04 / COMMUNITY</span><strong>Help shape the game</strong><p>Find a focused issue, reproduce a bug, improve documentation or propose a tested source change.</p><span>Contribute →</span></a>
</div>

## What is real today?

The repository contains **gameplay source code**, not distributed playable builds. Its portable C++20 algorithms are exercised in GitHub Actions on Windows, macOS and Ubuntu, while Unreal integration still needs a licensed engine build and physical-device qualification.

- **Driving prototype:** a simulated rigid-body starter car, configurable handling, keyboard/gamepad input and experimental Android touch controls.
- **Racing logic:** four directional checkpoint gates, lap timers, sectors, recovery penalty, time attack and race completion.
- **Offline progression:** fictional vehicles, upgrades, credits, 12 career events, two championships, and locally persisted profiles.
- **Engineering safeguards:** collision/startup audits, save rollback rules, race clock readiness, gated rewards and input safety tests.

[Full architecture →](ARCHITECTURE.md){ .lb-text-link } · [Roadmap →](ROADMAP.md){ .lb-text-link } · [Source releases →](release-history.md){ .lb-text-link }

---

<small>This website documents the repository; it is not a browser-playable game or a download page for an executable. No third-party vehicle trademarks or unofficial Unreal Engine binaries are distributed here.</small>
