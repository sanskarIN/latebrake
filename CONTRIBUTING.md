# Contributing to Latebrake

Thanks for helping build an original open-source racing game.

## Before you begin

Read [README.md](README.md), [docs/STATUS.md](docs/STATUS.md), [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) and the [BSD-3-Clause license](LICENSE). Please open a focused issue to discuss substantial changes to race architecture, licensing, driving physics, or platform support.

## Workflow

1. Fork or create a feature branch from `main` and use concise, descriptive branch names.
2. Keep each change independently understandable. Use imperative commit subjects (for example `fix(racing): reject repeated finish gate`).
3. Preserve C++20 and Unreal Engine 5.8 compatibility; gameplay logic should remain testable outside Unreal when possible.
4. Run the portable tests documented in the README and clearly state whether a **real** Unreal editor build / platform package was executed.
5. Document new inputs, save formats, assets, UI strings, known limitations and license notices.
6. Create a PR linking its issue, screenshots/videos only when genuinely produced, and a checklist of tests.
7. Do not bypass checks or label unverified features as production ready.

## Copyright / assets

Only submit content you own or are authorized to distribute under license-compatible terms. Never add restricted Unreal source, Epic Marketplace assets, unauthorized manufacturer logos, copied commercial-game tracks, scraped/unauthorized soundtracks or personal information. Document art provenance and attribution in [ASSET_LICENSES.md](ASSET_LICENSES.md).

The original project source is BSD-3-Clause. Contributions accepted to that source will be distributed under BSD-3-Clause unless the maintainers explicitly agree otherwise in writing.

## Commits

Use your own genuine Git author name/email for contributions; do not impersonate maintainers or fabricate commit histories. Do not split one trivial edit into many empty commits merely to increase GitHub contribution counts.

## Reports and community

- Report defects using the bug report template.
- Discuss features through issues before a large implementation.
- Follow [CODE_OF_CONDUCT.md](CODE_OF_CONDUCT.md).
- Disclose security issues privately using [SECURITY.md](SECURITY.md).
