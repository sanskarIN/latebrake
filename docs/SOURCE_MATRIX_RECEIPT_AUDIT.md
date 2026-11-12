# Offline exact-head source CI receipt audit

The maintained source CI matrix is relevant when deciding whether to
promote a portable C++ foundation into \`main\`. A GitHub PR can say
"mergeable" even though a required platform check is **queued**.
Do not convert such a queue into a passing native validation.

Run with a locally saved JSON response from the official GitHub
\`GET /repos/sanskarIN/latebrake/commits/{40SHA}/check-runs\` endpoint:

\`\`\`powershell
python scripts/audit_saved_source_matrix.py --checks checks.json --expected-head YOUR_FULL_40_DIGIT_SHA
\`\`\`

The command rejects duplicate keys, wrong head SHAs, invalid status values,
unfinished check results with invented success conclusions, missing source
matrix jobs, failed portable C++ builds on any desktop OS, failed Python
editor fixtures, PowerShell, release guards, and documentation checks.
The optional Pages deploy may be explicitly skipped on pull requests.

It prints actionable pending/failed/missing lists and returns code 2 when
the saved report is incomplete. Even a structurally all-green result
always includes:

- \`ci_authenticity_verified=false\` — the local JSON is untrusted data.
- \`native_unreal_build_verified=false\` — no Epic engine was invoked.
- \`installed_gameplay_verified=false\` — no actual hardware was tested.

Never use this tool alone to authorize a draft native-gameplay merge.
Verify the live GitHub workflow/source identity independently, then use
\`UNREAL_NATIVE_MERGE_READINESS.md\` for the owner-controlled UE5.8
Editor/UHT, Win64 cook and installed-playability gates.
