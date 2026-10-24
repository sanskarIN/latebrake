#!/usr/bin/env python3
"""Fail closed on broken structure or misleading release claims in Pages output.

This lightweight smoke check validates only static documentation. It is NOT
an Unreal build, package verifier or functional browser accessibility audit.
"""
from __future__ import annotations

import argparse
from html.parser import HTMLParser
from pathlib import Path
from urllib.parse import urlparse


class Links(HTMLParser):
    def __init__(self) -> None:
        super().__init__()
        self.links: list[str] = []
        self.main_count = 0

    def handle_starttag(self, tag: str, attrs: list[tuple[str, str | None]]) -> None:
        attributes = dict(attrs)
        if tag == "a" and attributes.get("href"):
            self.links.append(str(attributes["href"]))
        if tag == "main":
            self.main_count += 1


REQUIRED_HTML = (
    "index.html",
    "about/index.html",
    "features/index.html",
    "getting-started/index.html",
    "development-status/index.html",
    "contributing/index.html",
    "release-history/index.html",
    "BUILDING/index.html",
    "ARCHITECTURE/index.html",
    "WEBSITE/index.html",
    "TOUCH_CONTROLS/index.html",
    "STATUS/index.html",
    "ROADMAP/index.html",
    "releases/v0.5.22/index.html",
    "releases/v0.5.32/index.html",
)
REQUIRED_ASSETS = (
    "assets/stylesheets/extra.css",
    "assets/images/latebrake-mark.svg",
)
HOME_LINKS = (
    "about/",
    "features/",
    "getting-started/",
    "contributing/",
    "development-status/",
)


def verify(site_dir: Path) -> None:
    if not site_dir.is_dir():
        raise ValueError(f"no rendered site directory at {site_dir}")
    for rel in REQUIRED_HTML + REQUIRED_ASSETS:
        path = site_dir / rel
        if not path.is_file() or path.stat().st_size == 0:
            raise ValueError(f"missing/empty rendered page or asset: {rel}")

    home = (site_dir / "index.html").read_text(encoding="utf-8")
    home_low = home.lower()
    for phrase in ("source only", "v0.5.32", "unreal engine 5.8", "c++20"):
        if phrase not in home_low:
            raise ValueError(f"home page missing accurate core/status copy: {phrase}")
    status = (site_dir / "development-status/index.html").read_text(
        encoding="utf-8"
    ).lower()
    for phrase in (
        "not yet verified",
        "windows playable binary",
        "android apk",
        "v0.5.32",
    ):
        if phrase not in status:
            raise ValueError(f"status page lost current qualification detail: {phrase}")

    parser = Links()
    parser.feed(home)
    for link in HOME_LINKS:
        if link not in parser.links:
            raise ValueError(f"homepage lacks navigation to {link}")
        target = site_dir / link / "index.html"
        if not target.is_file():
            raise ValueError(f"homepage navigation target missing: {link}")
    for href in parser.links:
        parsed = urlparse(href)
        if parsed.scheme and parsed.scheme not in ("https", "http", "mailto"):
            raise ValueError(f"unexpected external link scheme: {href}")
        if href.startswith("javascript:"):
            raise ValueError("javascript: URLs are never valid navigation")

    # GitHub Pages builds MUST serve rendered documentation, not the source
    # repository. Do not publish C++ or PowerShell source files as web assets.
    forbidden = (".cpp", ".h", ".ps1", ".uproject", ".umap", ".apk", ".exe")
    leaked = [
        path.relative_to(site_dir).as_posix()
        for path in site_dir.rglob("*")
        if path.is_file() and path.name.lower().endswith(forbidden)
    ]
    if leaked:
        raise ValueError(f"repository build/game source leaked into Pages: {leaked[:5]}")
    print(f"Documentation Pages structure verified: {len(REQUIRED_HTML)} rendered pages")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("site_dir", type=Path, nargs="?", default=Path("site"))
    args = parser.parse_args()
    verify(args.site_dir)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
