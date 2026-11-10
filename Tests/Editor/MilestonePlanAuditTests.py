"""Contract tests for read-only milestone planning and live snapshot diff.

All fixtures are synthetic. These tests make NO GitHub API requests and
cannot create/change any milestone, PR, issue, release, or tag.
"""
import copy
import json
import sys
import unittest
from pathlib import Path
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts"))

from AuditMilestonePlan import (  # noqa: E402
    PLAN_PATH, PlanError, compare_live, github_pages, validate_plan,
)


class MilestonePlanAuditTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.plan = json.loads(PLAN_PATH.read_text(encoding="utf-8"))
        cls.source = (ROOT / "scripts/AuditMilestonePlan.py").read_text(
            encoding="utf-8"
        )
        cls.doc = (ROOT / "docs/RELEASE_MILESTONE_PLAN.md").read_text(
            encoding="utf-8"
        )
        cls.nav = (ROOT / "mkdocs.yml").read_text(encoding="utf-8")
        cls.workflow = (
            ROOT / ".github/workflows/audit-milestones.yml"
        ).read_text(encoding="utf-8")

    def test_every_current_open_pr_and_issue_explained_once(self):
        summary = validate_plan(self.plan)
        self.assertEqual(summary["milestone_count"], 3)
        self.assertEqual(summary["planned_prs"], 15)
        self.assertEqual(summary["planned_issues"], 4)
        self.assertEqual(summary["assigned"][("pr", 295)],
                         self.plan["milestones"][0]["title"])
        self.assertEqual(summary["assigned"][("pr", 334)],
                         self.plan["milestones"][0]["title"])
        self.assertEqual(summary["assigned"][("issues", 148)],
                         self.plan["milestones"][1]["title"])
        self.assertEqual(summary["assigned"][("pr", 335)],
                         self.plan["milestones"][1]["title"])
        for number in range(329, 334):
            self.assertEqual(summary["assigned"][("pr", number)],
                             self.plan["milestones"][2]["title"])
        self.assertEqual(summary["assigned"][("issues", 233)],
                         "Deferred — no active release milestone")
        self.assertEqual(summary["assigned"][("issues", 234)],
                         "Deferred — no active release milestone")
        self.assertEqual(summary["assigned"][("issues", 13)],
                         "Deferred — no active release milestone")
        self.assertEqual(summary["assigned"][("pr", 336)],
                         "Deferred — no active release milestone")

    def test_fail_if_item_is_double_counted_between_branch_and_aggregate(self):
        bad = copy.deepcopy(self.plan)
        bad["milestones"][2]["pr"].append(335)
        with self.assertRaisesRegex(PlanError, "Duplicate milestone"):
            validate_plan(bad)
        bad = copy.deepcopy(self.plan)
        bad["backlog"].append({
            "number": 148, "kind": "issue",
            "reason": "This important real Unreal QA gate is not done",
        })
        with self.assertRaisesRegex(PlanError, "both"):
            validate_plan(bad)

    def test_fail_missing_item_or_unknown_new_pr(self):
        bad = copy.deepcopy(self.plan)
        bad["milestones"][1]["issues"] = []
        with self.assertRaisesRegex(PlanError, "missing"):
            validate_plan(bad)
        bad = copy.deepcopy(self.plan)
        bad["checked_open_prs"].append(999999)
        with self.assertRaisesRegex(PlanError, "missing"):
            validate_plan(bad)

    def test_failure_if_pr_dependency_is_cyclic_or_untracked(self):
        bad = copy.deepcopy(self.plan)
        bad["dependencies"].append({"pr": 329, "requires": 333})
        with self.assertRaisesRegex(PlanError, "Cyclic"):
            validate_plan(bad)
        bad = copy.deepcopy(self.plan)
        bad["dependencies"].append({"pr": 335, "requires": 9})
        with self.assertRaisesRegex(PlanError, "distinct audited"):
            validate_plan(bad)

    def test_source_preview_stays_separate_from_real_unreal_qa(self):
        bad = copy.deepcopy(self.plan)
        bad["milestones"][1]["pr"].remove(335)
        bad["milestones"][0]["pr"].append(335)
        with self.assertRaisesRegex(PlanError, "boundaries"):
            validate_plan(bad)
        self.assertIn("zero attached executable", self.doc.replace("\n", " ").replace("**", ""))
        self.assertIn("200,000", self.doc)
        self.assertIn("PR #335", self.doc)
        self.assertIn("RELEASE_MILESTONE_PLAN.md", self.nav)

    def test_read_only_live_comparison_reports_missing_and_wrong_assignments(self):
        source_only, engine, alpha = self.plan["milestones"]
        milestones = [{"title": source_only["title"]}, {"title": engine["title"]}]
        prs = [
            {"number": 295, "milestone": {"title": source_only["title"]}},
            {"number": 334, "milestone": None},
            {"number": 335, "milestone": {"title": engine["title"]}},
            {"number": 333, "milestone": {"title": alpha["title"]}},
            {"number": 800, "milestone": None},
        ]
        issues = [
            {"number": 148, "milestone": None},
            {"number": 13, "milestone": None},
            {"number": 900, "milestone": None},
            {"number": 333, "pull_request": {}, "milestone": None},
        ]
        result = compare_live(self.plan, milestones, prs, issues)
        self.assertEqual(result["missing_proposed_milestones"], [alpha["title"]])
        self.assertEqual(
            {(x["kind"], x["number"]) for x in result["assignment_changes_needed"]},
            {("pr", 334), ("issue", 148)},
        )
        self.assertEqual(result["new_open_items_since_snapshot"],
                         {"prs": [800], "issues": [900]})
        self.assertTrue(result["read_only"])
        self.assertFalse(result["real_unreal_gameplay_verified"])

    def test_closed_pr_does_not_request_reopening(self):
        out = compare_live(self.plan, [], [], [])
        self.assertEqual(out["assignment_changes_needed"], [])
        self.assertEqual(len(out["missing_proposed_milestones"]), 3)

    def test_actions_is_owner_dispatched_read_only_no_release(self):
        w = self.workflow
        for required in (
            "workflow_dispatch:",
            "contents: read",
            "issues: read",
            "pull-requests: read",
            "github.actor == 'sanskarIN'",
            "github.ref == 'refs/heads/main'",
            "persist-credentials: false",
            "AuditMilestonePlan.py --live --json",
            "GITHUB_STEP_SUMMARY",
        ):
            self.assertIn(required, w)
        for forbidden in (
            "contents: write", "issues: write", "pull-requests: write",
            "pull_request_target:", "schedule:", "upload-artifact@",
            "createRelease(", "createRef(", "Build-PlayableCandidate",
            "Compile-UnrealEditor", "update_issue(",
        ):
            self.assertNotIn(forbidden, w)

    def test_get_only_never_mutate_api_or_write_back(self):
        for forbidden in (
            'method="POST"', 'method="PATCH"', 'method="DELETE"',
            "create_milestone", "update_issue(", "merge_pull_request(",
            "createRelease(", "createRef(",
        ):
            self.assertNotIn(forbidden, self.source)
        with patch("AuditMilestonePlan.urlopen") as open_api:
            response = open_api.return_value.__enter__.return_value
            response.read.return_value = b"[]"
            # A real JSON decoder reads via response.read.
            self.assertEqual(github_pages("milestones?state=all", ""), [])
            request = open_api.call_args[0][0]
            self.assertEqual(request.get_method(), "GET")
            self.assertIn("/milestones?state=all&per_page=100&page=1",
                          request.full_url)


if __name__ == "__main__":
    unittest.main()
