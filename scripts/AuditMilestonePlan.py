#!/usr/bin/env python3
"""Read-only LateBrake GitHub release-milestone planner and gap inspector.

The plan is PROPOSED until an owner creates the corresponding milestones.
This tool NEVER creates/moves a milestone, changes an issue/PR or publishes
a release. Offline schema validation works without a network connection.
"""
from __future__ import annotations

import argparse
import json
import os
from pathlib import Path
from urllib.request import Request, urlopen

ROOT = Path(__file__).resolve().parents[1]
PLAN_PATH = ROOT / "scripts/releases/latebrake-milestones.json"
BASE_API = "https://api.github.com/repos/sanskarIN/latebrake"


class PlanError(ValueError):
    pass


def validate_plan(plan: dict) -> dict:
    """Prove a complete, disjoint, consistent snapshot; no inflated scope."""
    if plan.get("schema_version") != 1 or plan.get("repository") != "sanskarIN/latebrake":
        raise PlanError("Unsupported release milestone schema/repository")
    entries = plan.get("milestones")
    if not isinstance(entries, list) or len(entries) != 3:
        raise PlanError("Exactly three staged milestones are required")
    seen_names = set()
    assigned = {}
    for entry in entries:
        title = entry.get("title")
        if not isinstance(title, str) or not title.strip() or title in seen_names:
            raise PlanError("Milestone names must be distinct nonempty strings")
        seen_names.add(title)
        criteria = entry.get("acceptance")
        if not isinstance(criteria, list) or len(criteria) < 3 or any(
            not isinstance(x, str) or len(x.strip()) < 15 for x in criteria
        ):
            raise PlanError("Each milestone needs three meaningful exit criteria")
        for kind in ("pr", "issues"):
            nums = entry.get(kind)
            if not isinstance(nums, list):
                raise PlanError("Malformed milestone assignments")
            for number in nums:
                if type(number) is not int or number < 1:
                    raise PlanError("Invalid PR/issue number")
                key = (kind, number)
                if key in assigned:
                    raise PlanError(f"Duplicate milestone assignment: {key}")
                assigned[key] = title

    backlog = plan.get("backlog")
    if not isinstance(backlog, list):
        raise PlanError("Missing explicitly deferred backlog")
    for entry in backlog:
        number, kind = entry.get("number"), entry.get("kind")
        if type(number) is not int or number < 1 or kind not in ("issue", "pr"):
            raise PlanError("Malformed backlog item")
        key = ("pr" if kind == "pr" else "issues", number)
        if key in assigned:
            raise PlanError("Issue/PR cannot be both a milestone and backlog")
        if not isinstance(entry.get("reason"), str) or len(entry["reason"]) < 15:
            raise PlanError("Backlog item needs a concrete deferral reason")
        assigned[key] = "Deferred — no active release milestone"

    for collection, kind in (("checked_open_prs", "pr"),
                             ("checked_open_issues", "issues")):
        nums = plan.get(collection)
        if not isinstance(nums, list) or not nums or len(set(nums)) != len(nums):
            raise PlanError("Malformed snapshot inventory")
        if any(type(n) is not int or n < 1 for n in nums):
            raise PlanError("Snapshot has invalid item IDs")
        actual = {n for (k, n) in assigned if k == kind}
        if actual != set(nums):
            missing = sorted(set(nums) - actual)
            extra = sorted(actual - set(nums))
            raise PlanError(f"Plan must explain all {kind}: missing={missing} extra={extra}")

    edges = plan.get("dependencies")
    if not isinstance(edges, list):
        raise PlanError("Missing PR dependency graph")
    graph = {n: [] for n in plan["checked_open_prs"]}
    for entry in edges:
        child, parent = entry.get("pr"), entry.get("requires")
        if child not in graph or parent not in graph or child == parent:
            raise PlanError("Dependency must reference distinct audited open PRs")
        if parent in graph[child]:
            raise PlanError("Repeated dependency")
        graph[child].append(parent)
    visiting, done = set(), set()

    def visit(node: int) -> None:
        if node in visiting:
            raise PlanError("Cyclic PR dependency")
        if node in done:
            return
        visiting.add(node)
        for parent in graph[node]:
            visit(parent)
        visiting.remove(node)
        done.add(node)

    for node in graph:
        visit(node)

    aggregation = plan.get("aggregation")
    if not isinstance(aggregation, dict) or aggregation.get("pr") != 335 or \
            sorted(aggregation.get("includes", [])) != [329, 330, 331, 332, 333]:
        raise PlanError("Combined real-UE candidate must retain exact component list")
    if assigned.get(("pr", 334)) != entries[0]["title"] or \
            assigned.get(("pr", 295)) != entries[0]["title"] or \
            assigned.get(("pr", 335)) != entries[1]["title"] or \
            assigned.get(("issues", 148)) != entries[1]["title"]:
        raise PlanError("Source publisher/real-UE evidence boundaries must not drift")
    if not entries[0]["title"].startswith("v0.5.98") or \
            not entries[2]["title"].startswith("v0.6.0"):
        raise PlanError("Milestone phase ordering drifted")
    return {"repository": plan["repository"],
            "milestone_count": len(entries),
            "planned_prs": len(plan["checked_open_prs"]),
            "planned_issues": len(plan["checked_open_issues"]),
            "assigned": assigned}


def github_pages(path: str, token: str) -> list[dict]:
    """GET only. Repository host and endpoint paths are not user-supplied."""
    items = []
    for page in range(1, 21):
        url = f"{BASE_API}/{path}&per_page=100&page={page}"
        headers = {"Accept": "application/vnd.github+json",
                   "User-Agent": "LateBrake-ReadOnly-Milestone-Audit"}
        if token:
            headers["Authorization"] = "Bearer " + token
        with urlopen(Request(url, headers=headers, method="GET"), timeout=20) as response:
            data = json.load(response)
        if not isinstance(data, list):
            raise PlanError("GitHub milestone endpoint returned a non-array")
        items.extend(data)
        if len(data) < 100:
            return items
    raise PlanError("GitHub pagination exceeds safety limit")


def compare_live(plan: dict, milestones: list, prs: list, issues: list) -> dict:
    """Recommend changes, NEVER perform them. Closed PRs remain historical."""
    validate_plan(plan)
    existing = {m.get("title"): m for m in milestones}
    expected = {}
    for milestone in plan["milestones"]:
        for kind, field in (("pr", "pr"), ("issues", "issue")):
            for number in milestone[kind]:
                expected[(field, number)] = milestone["title"]
    open_prs = {p["number"]: p for p in prs}
    open_issues = {i["number"]: i for i in issues if "pull_request" not in i}
    mismatches = []
    for (kind, number), title in sorted(expected.items()):
        source = open_prs if kind == "pr" else open_issues
        actual = source.get(number)
        if actual is None:
            # Do not demand that a successfully closed or merged PR reopen.
            continue
        linked_title = (actual.get("milestone") or {}).get("title")
        if linked_title != title:
            mismatches.append({
                "kind": kind, "number": number,
                "expected": title, "current": linked_title,
            })
    unplanned = {
        "prs": sorted(set(open_prs) - set(plan["checked_open_prs"])),
        "issues": sorted(set(open_issues) - set(plan["checked_open_issues"])),
    }
    missing = [m["title"] for m in plan["milestones"] if m["title"] not in existing]
    return {
        "open_pr_count": len(open_prs), "open_issue_count": len(open_issues),
        "existing_milestones": sorted(existing),
        "missing_proposed_milestones": missing,
        "assignment_changes_needed": mismatches,
        "new_open_items_since_snapshot": unplanned,
        "read_only": True,
        "release_publication_verified": False,
        "real_unreal_gameplay_verified": False,
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--live", action="store_true",
                        help="Read current GitHub milestone/PR/issue metadata; GET only")
    parser.add_argument("--json", action="store_true", help="Print JSON report")
    args = parser.parse_args()
    plan = json.loads(PLAN_PATH.read_text(encoding="utf-8"))
    summary = validate_plan(plan)
    result = {
        "status": "VALID PROPOSAL — no milestones created or changed",
        "milestone_titles": [x["title"] for x in plan["milestones"]],
        "planned_open_prs": summary["planned_prs"],
        "planned_open_issues": summary["planned_issues"],
        "read_only": True,
    }
    if args.live:
        token = os.getenv("GH_TOKEN") or os.getenv("GITHUB_TOKEN") or ""
        result["live"] = compare_live(
            plan,
            github_pages("milestones?state=all", token),
            github_pages("pulls?state=open", token),
            github_pages("issues?state=open", token),
        )
    if args.json:
        print(json.dumps(result, indent=2, sort_keys=True))
    else:
        print(result["status"])
        for title in result["milestone_titles"]:
            print("  - " + title)
        if args.live:
            print("Missing milestones:", result["live"]["missing_proposed_milestones"])
            print("Open PR/issue changes needed:",
                  len(result["live"]["assignment_changes_needed"]))
            print("New untriaged items:",
                  result["live"]["new_open_items_since_snapshot"])
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
