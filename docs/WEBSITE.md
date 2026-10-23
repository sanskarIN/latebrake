# Maintaining the documentation website

Latebrake's public, searchable documentation is hosted on **[GitHub Pages](https://sanskarin.github.io/latebrake/)**. It is generated from the same `docs/` Markdown files contributors edit in the repository. The site is an information portal; it **does not host a playable game or certify Unreal builds**.

## How publication works

```text
docs/*.md + docs/assets + mkdocs.yml
            |
            v
GitHub Actions: docs-pages.yml
            |
            +--> Pull request: build and verify only
            |
            +--> main: build + check + upload artifact
                                |
                                v
                    GitHub Pages deployment
                    /latebrake/ (HTTPS)
```

The source workflow is [`.github/workflows/docs-pages.yml`](https://github.com/sanskarIN/latebrake/blob/main/.github/workflows/docs-pages.yml).

- **Pull requests:** The website renders with pinned MkDocs dependencies, and the generated pages/assets/status/navigation are checked. No Pages deployment occurs from a pull request.
- **Merged changes to `main`:** Only edits to `docs/**`, `mkdocs.yml`, `docs-requirements.txt`, the site checker or the Pages workflow trigger a new documentation publication. This avoids redeploying docs for every gameplay C++ commit.
- **Manual refresh:** A maintainer can run **Documentation website (GitHub Pages)** through GitHub Actions → Run workflow on `main`.
- **Deployment permissions:** The deploy job uses `pages: write`, `id-token: write`, the `github-pages` environment and GitHub's official Pages artifact deployment.
- **Source settings:** GitHub Pages should use **GitHub Actions** as its publishing source under repository Settings → Pages. No long-lived personal access token is required for the workflow.

## Editing content

| Want to change | Edit |
| --- | --- |
| Welcome screen and current source-only warning | `docs/index.md` |
| Features and project overview | `docs/features.md`, `docs/about.md` |
| Getting started and contributor paths | `docs/getting-started.md`, `docs/contributing.md` |
| Public release-vs-development status | `docs/development-status.md` |
| Authoritative deep engineering detail | `docs/STATUS.md`, `docs/TESTING.md`, `docs/EXECUTABLE_BUILD_PIPELINE.md` |
| Side navigation, theme, canonical URL | `mkdocs.yml` |
| Styling, mobile layout, accessible focus | `docs/assets/stylesheets/extra.css` |
| Static output acceptance checks | `scripts/verify_docs_site.py` |

New documentation pages must be added to the appropriate `nav` group in `mkdocs.yml` if they should be discoverable in site navigation. Existing historical `docs/releases/` pages are rendered but not all individually repeated in the sidebar; [release history](release-history.md) links to them.

## Build locally

Install Python 3.12 or newer, then:

```bash
python -m pip install -r docs-requirements.txt
python -m mkdocs serve
```

For the exact CI smoke checks:

```bash
python -m mkdocs build --clean
python scripts/verify_docs_site.py site
```

Built output is in `site/` and is **not** a game package. Use the URLs MkDocs prints for local previews. The public site is served from the repository subpath `/latebrake/`, not the owner-level domain root.

## If deployment fails

1. Open the [documentation Actions workflow](https://github.com/sanskarIN/latebrake/actions/workflows/docs-pages.yml).
2. Inspect whether **Build and verify docs** failed due to malformed Markdown, missing navigation targets, missing assets or inaccurate status copy.
3. If **Deploy official Pages documentation** failed, check Settings → Pages publishing source and repository/environment permissions.
4. After correcting a problem through a normal PR, merge into `main` so the official Pages workflow publishes new output.
5. Verify the **real public URL**, not merely a successful artifact-upload step.

## Accuracy requirements

Always distinguish a portable CI pass from licensed Unreal Editor/UHT compilation and installed gameplay. Do **not** add a download button for an EXE/APK unless a genuine packaged artifact has passed source/map identity checks and installed device QA. Keep both the landing page and [development status](development-status.md) accurate whenever source milestones or releases change.

[View the website](https://sanskarin.github.io/latebrake/) · [Source repository](https://github.com/sanskarIN/latebrake)
