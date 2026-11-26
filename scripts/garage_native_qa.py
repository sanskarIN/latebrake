"""Deterministic manual acceptance matrix for the native six-car Garage.

Pure source policy: no network, engine execution, purchase, save mutation,
or release approval. Human observations and editable receipts are not
independent verification of an installed game.
"""
from __future__ import annotations

import re

SCHEMA = 1
STATUS = "MANUAL_NATIVE_GARAGE_QA_UNVERIFIED"
SUMMARY_STATUS = "HASH_BOUND_NATIVE_GARAGE_OBSERVATIONS_UNVERIFIED"
WARNING = (
    "SELF-REPORTED observations; hashes bind local files, not their origin "
    "or gameplay authenticity. No release or merge is authorized."
)
RESULTS = frozenset(("pass", "fail", "not_tested"))
MAX_NOTE_LENGTH = 500
ENVIRONMENT_FIELDS = ("windows_version", "cpu", "gpu", "driver", "controller")
CAR_IDS = tuple(range(6))
PAINT_IDS = tuple(range(8))
STRIPE_IDS = tuple(range(3))
RIM_IDS = tuple(range(3))
ORBIT_IDS = tuple(range(4))
VIEWPORTS = ("900x600", "1280x720", "1366x768", "1920x1080", "2560x1080", "3840x2160")


def scenarios() -> dict[str, dict[str, str]]:
    """Stable IDs and explicit instructions; no fake observations."""
    result = {}
    for car in CAR_IDS:
        key = f"car_{car}"
        result[f"{key}_browse"] = {
            "group": "identity",
            "observe": "Viewed identity and actual car silhouette; ownership and equipped identity remain unchanged when browsing.",
        }
        for paint in PAINT_IDS:
            result[f"{key}_paint_{paint}"] = {
                "group": "paint",
                "observe": "Select this paint; compare body and trim with chooser swatch; confirm persistence and no mesh rebuild.",
            }
        for stripe in STRIPE_IDS:
            result[f"{key}_stripe_{stripe}"] = {
                "group": "stripe",
                "observe": "Inspect exactly zero, one or two bands as selected; no gaps, floating bars or z-fighting; no mesh rebuild.",
            }
        for rim in RIM_IDS:
            result[f"{key}_rim_{rim}"] = {
                "group": "rim",
                "observe": "Check all four wheels, correct 7/5/12-spoke style, roll, steer, reverse and unchanged contact physics.",
            }
        for orbit in ORBIT_IDS:
            result[f"{key}_orbit_{orbit}"] = {
                "group": "camera",
                "observe": "Inspect actual camera view; no body clipping, obscured controls or stale racing camera on exit.",
            }
    for viewport in VIEWPORTS:
        result[f"viewport_{viewport}"] = {
            "group": "layout",
            "observe": "Inspect labels, focus, chooser, purchase strip, quick-view and orbit controls at this actual viewport.",
        }
    for key, description in (
        ("locked_browse", "Locked car may be viewed but not equipped or styled."),
        ("parked_owned_style", "Style a parked owned car without changing equipped ID, upgrades or credits."),
        ("mouse_direct_choice", "Choose each cosmetic layer directly by mouse; no hidden button admission."),
        ("keyboard_navigation", "Open and switch chooser; move focus, confirm a visibly focused option and cancel."),
        ("controller_navigation", "Repeat layer selection and visible focus with actual controller hardware."),
        ("stale_focus", "Rapid move/confirm and resize cannot commit a previously drawn option."),
        ("purchase_consent", "First Q/U/I or click stages; second matching visible confirmation spends only once."),
        ("purchase_cancel_on_style", "Opening free chooser cancels pending purchase; closing does not restore consent."),
        ("purchase_expiry_focus", "Expired, changed, focus-lost and page-switched confirmations cannot spend."),
        ("insufficient_credits", "Unaffordable, maxed and blocked offers cannot mutate credits."),
        ("read_only_guard", "Future-schema and suspended/career contexts reject edits but retain permitted navigation."),
        ("save_rollback", "Induce a controlled save failure; ownership, credits, tuning and appearance are restored."),
        ("save_reload", "Restart and verify actual selected car, looks and credits persisted."),
        ("factory_noop", "Reset factory appearance twice; second reset is a no-op without another save."),
        ("fallback_visual", "Verify visible non-colliding legacy fallback in controlled missing-material/mesh fixture."),
        ("finish_capabilities", "Inspect actual shader capabilities; compare supported finishes or record tint-only behavior."),
        ("race_return", "Return to racing; restore equipped car, camera and controls; no residual preview inputs."),
        ("performance", "Observe responsiveness, stability and frame times across car/rim changes and racing."),
    ):
        result[key] = {"group": "integration", "observe": description}
    return result


def checked_source_sha(value: object) -> str:
    if not isinstance(value, str) or not re.fullmatch(r"[0-9a-fA-F]{40}", value):
        raise ValueError("Provide the exact reviewed 40-character source SHA.")
    return value.lower()


def empty_environment() -> dict[str, str]:
    return {name: "" for name in ENVIRONMENT_FIELDS}


def empty_checks() -> dict[str, dict[str, str]]:
    return {
        key: {**definition, "result": "not_tested", "notes": ""}
        for key, definition in scenarios().items()
    }


def evaluate_observations(environment: object, checks: object) -> dict:
    """Validate a complete, unchanged matrix and count manual observations."""
    if not isinstance(environment, dict) or set(environment) != set(ENVIRONMENT_FIELDS):
        raise ValueError("Garage QA environment fields are incomplete or unknown.")
    for value in environment.values():
        if not isinstance(value, str) or len(value) > 250:
            raise ValueError("Environment fields must be bounded text.")
    expected = scenarios()
    if not isinstance(checks, dict) or set(checks) != set(expected):
        raise ValueError("Garage scenario IDs cannot be missing, added or replaced.")
    counts = {name: 0 for name in ("pass", "fail", "not_tested")}
    groups = {}
    unresolved = []
    for key, definition in expected.items():
        record = checks[key]
        if not isinstance(record, dict) or set(record) != {"group", "observe", "result", "notes"}:
            raise ValueError(f"Malformed Garage observation: {key}")
        if record["group"] != definition["group"] or record["observe"] != definition["observe"]:
            raise ValueError(f"Garage scenario definition changed: {key}")
        result, notes = record["result"], record["notes"]
        if not isinstance(result, str) or result not in RESULTS:
            raise ValueError(f"Invalid Garage result: {key}")
        if not isinstance(notes, str) or len(notes) > MAX_NOTE_LENGTH:
            raise ValueError(f"Invalid Garage observation notes: {key}")
        if result != "not_tested" and len(notes.strip()) < 12:
            raise ValueError(f"Manual {result} requires a descriptive observation: {key}")
        counts[result] += 1
        group = groups.setdefault(definition["group"], {name: 0 for name in counts})
        group[result] += 1
        if result != "pass":
            unresolved.append(key)
    return {
        "scenario_count": len(expected),
        "counts": counts,
        "groups": groups,
        "unresolved_scenarios": unresolved,
        "environment_complete": all(value.strip() for value in environment.values()),
        "all_checks_self_reported_pass": counts["pass"] == len(expected),
    }

MAX_SESSIONS = 16


def merge_session_observations(sessions: list[dict]) -> dict:
    """Combine separately validated sessions of one exact build.

    A later pass never hides an earlier failure. The owner must inspect
    contradictory observations rather than automatically certify a result.
    """
    if not 1 <= len(sessions) <= MAX_SESSIONS:
        raise ValueError("Garage aggregation requires one to sixteen sessions.")
    expected = scenarios()
    seen = set()
    merged = {
        key: {"pass": [], "fail": [], "not_tested": []}
        for key in expected
    }
    for session in sessions:
        identity = session["identity"]
        digest = identity["receipt_sha256"]
        if digest in seen:
            raise ValueError("Garage aggregation cannot repeat a runtime receipt.")
        seen.add(digest)
        evaluate_observations(session["environment"], session["checks"])
        for key, record in session["checks"].items():
            merged[key][record["result"]].append(digest)
    counts = {result: 0 for result in ("pass", "fail", "not_tested")}
    groups = {}
    unresolved = []
    conflicts = []
    outcomes = {}
    for key, evidence in merged.items():
        result = "fail" if evidence["fail"] else "pass" if evidence["pass"] else "not_tested"
        counts[result] += 1
        group = groups.setdefault(expected[key]["group"], {name: 0 for name in counts})
        group[result] += 1
        if result != "pass":
            unresolved.append(key)
        if evidence["pass"] and evidence["fail"]:
            conflicts.append(key)
        outcomes[key] = {"result": result, "evidence": evidence}
    return {
        "session_count": len(sessions),
        "scenario_count": len(expected),
        "counts": counts,
        "groups": groups,
        "unresolved_scenarios": unresolved,
        "conflicting_scenarios": conflicts,
        "all_checks_self_reported_pass": counts["pass"] == len(expected),
        "outcomes": outcomes,
    }
