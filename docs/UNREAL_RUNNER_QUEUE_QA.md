# Troubleshooting queued UE5.8 manual GitHub Actions runs

The [manual draft-PR validation workflow](https://github.com/sanskarIN/latebrake/actions/workflows/validate-draft-unreal-pr.yml)
is **not a GitHub-hosted Unreal build service**. It executes only when an
owner-controlled, real Windows x64 GitHub Actions self-hosted runner is
registered, online, available, accessible to this repository, and has the
custom label `unreal-5-8`.

A queued workflow with zero started job steps does **not** mean Unreal
compiled, that the selected pull request was authenticated, or that
an executable is being created. Never report an Unreal result from a
queued job. The separate portable C++20/Windows source CI does not
provide the Epic Engine, Editor/UHT, cooked .umap or installed gameplay.

## Status diagnosis (read-only, no rerun required)

1. Open the [manual workflow runs](https://github.com/sanskarIN/latebrake/actions/workflows/validate-draft-unreal-pr.yml)
   and select the run. Inspect the *job* as well as the workflow. Distinguish
   `queued`, `in_progress` and `completed`; don't infer an outcome from
   a workflow_dispatch timestamp.
2. Open the repository **Settings → Actions → Runners**, or the **Actions →
   Management → Runners** page if that is how GitHub presents runner access.
   Verify a real Windows x64 runner is listed, **Online** and **Idle**.
   An offline, busy or missing runner cannot pick up this job.
3. The runner must match every required label, without dropping the
   trust boundary: `self-hosted`, `Windows`, `X64`, `unreal-5-8`.
   The last label is **custom**, not automatically provided by GitHub.
   Configure it on an existing authorized runner in its GitHub labels.
   Do **not** replace the workflow's self-hosted requirement with
   `windows-latest` just to unblock the queue; public runners lack
   the protected licensed UE toolchain and cannot verify this engine.
4. If no runner exists, use the official repository **New self-hosted
   runner** wizard (Windows x64) on an isolated machine you manage.
   Follow GitHub's current download, authentication and setup
   instructions **locally**. Never put a registration token, access
   token or runner credentials in a repository, chat message, CI log
   or PR. Use a dedicated non-admin runner account where practical.
5. Ensure the runner application/service is running. GitHub's runner
   terminal normally reports `Connected to GitHub` and `Listening for Jobs`.
   When using Windows runner-as-a-service, the service runs under its
   configured **service account**; installing Unreal for one interactive
   Windows user does not automatically install it for that service.
   PowerShell's read-only `Get-Service 'actions.runner.*'` may help on
   a Windows machine with runner services installed. Interactive runners
   won't appear as a running service and instead need their console open.
6. Set the environment variable `LATEBRAKE_UNREAL_ROOT` **on the
   actual runner process/service**, pointing to the owned UE **5.8**
   installation. Do not confuse it with the GitHub-hosted portable
   CI machine's variables. Confirm Windows PowerShell **7**, Git,
   MSVC/Visual Studio C++ tools, Windows SDK and sufficient free disk
   exist on the runner. The existing read-only source script
   `scripts/Test-UnrealWorkstationPrerequisites.ps1` checks only basic
   engine/project paths and the UE Build.version metadata; it does
   **not** prove UHT, C++, UAT or hardware readiness.
7. If a runner appears Online/Idle but the job is still queued,
   check its labels, its repository/organization group access, and
   whether another job is occupying it. If your organization restricts
   self-hosted access to a public repository, review that policy;
   do not weaken access rules without understanding the security
   consequences. GitHub typically keeps a job queued until a matching
   available runner is online, or eventually expires the queued job.
8. The workflow accepts **only an owner-dispatched, main-targeted,
   same-repository, open PR with an exact reviewed 40-character head SHA**.
   For the current six-car candidate, see
   [draft PR #341](https://github.com/sanskarIN/latebrake/pull/341).
   Review the *current head* before dispatching; do not assume any
   SHA printed in old documentation is still correct. Select
   `validation=Editor` first. The workflow checks the PR number,
   base and commit **after the runner accepts the job**; a queue is
   not evidence that these inputs were valid.
9. If an Editor run actually finishes successfully, review the real
   UHT/Editor logs and exact checked-out source hash before manually
   requesting `Win64`. Even a successfully uploaded artifact is
   explicitly `UNVERIFIED` until a real installed Windows driving
   test qualifies gameplay and the original cooked circuit.

## Invariants and evidence

- A queued job produces **no actual UE5.8 compile result**, and a
  source-only GitHub-hosted test is never evidence of a licensed UE build.
- Do not edit the in-progress or queued pinned candidate branch, invent
  an executable link, use fallback asset stubs, or publish a release.
- Keep owner review, PR head pinning and main-only workflow dispatch.
  Do not enable automatic `pull_request` jobs on a trusted private
  machine, run unreviewed forks, or share registration tokens.
- After genuine engine execution, keep separate evidence for
  UE Editor/UHT, native Win64 cook, original `.umap`, installed
  gameplay, six-car AI/physics and 30/60/120 FPS. The former qualification issue #148 has been deleted. Use the maintained
  [native merge-evidence checklist](UNREAL_NATIVE_MERGE_READINESS.md) and
  the active [six-car review candidate PR #341](https://github.com/sanskarIN/latebrake/pull/341)
  to record evidence; a deleted issue cannot satisfy gameplay approval.
- GitHub documents:
  [adding self-hosted runners](https://docs.github.com/en/actions/how-tos/manage-runners/self-hosted-runners/add-runners),
  [runner labels](https://docs.github.com/en/actions/how-tos/manage-runners/self-hosted-runners/use-in-a-workflow),
  and [runner selection](https://docs.github.com/en/actions/reference/runners/self-hosted-runners).

See [manual draft-PR validation](UNREAL_DRAFT_PR_VALIDATION.md) for
the underlying workflow's authentication and no-publishing boundaries.
