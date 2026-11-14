'use strict';

// READ-ONLY owner-invoked mainline source-release readiness auditor.
// No tag creation, release creation, workflow dispatch or branch mutation.
// SOURCE-ONLY identity evidence is NOT licensed Unreal or installed-game QA.
const TAG = 'v0.5.100';
const PREVIOUS_TAG = 'v0.5.99';
const PREVIOUS_SHA = 'a3104ea76809ac86d315a8a31550d3215e79f824';
const NOTES_PATH = 'docs/releases/v0.5.100.md';
const REQUIRED_MERGED_PR = 377;
const SHA40 = /^[0-9a-f]{40}$/;
const NAME = 'LateBrake v0.5.100 — 3D Drafting Continuity (SOURCE-ONLY Pre-release)';
const WARNINGS = Object.freeze([
  'SOURCE-ONLY PRE-RELEASE',
  'NO PLAYABLE WINDOWS EXECUTABLE',
  'NOT a playable game build',
  'No compiled Win64 EXE',
  'Android APK/AAB',
  'not installers',
  'zero executable binaries',
  'zero attached game assets',
  'v0.5.99',
  'not integrated into real Unreal gameplay',
]);
const REQUIRED_CHECKS = Object.freeze([
  'Portable C++20 tests / windows-latest',
  'Portable C++20 tests / ubuntu-latest',
  'Portable C++20 tests / macos-latest',
  'Unreal editor-only Python syntax (not Unreal runtime)',
  'Release PowerShell syntax',
  'Offline source-release publication guards (NO GitHub writes)',
  'Build and verify docs',
]);

function requireState(value, message) {
  if (!value) throw new Error(message);
}
function requireContext(context, candidateSha) {
  requireState(context?.repo?.owner === 'sanskarIN' &&
    context.repo.repo === 'latebrake' &&
    context.actor === 'sanskarIN' &&
    context.eventName === 'workflow_dispatch' &&
    context.ref === 'refs/heads/main' &&
    typeof candidateSha === 'string' && SHA40.test(candidateSha) &&
    context.sha === candidateSha,
    'Read-only release audit requires owner manual dispatch on exact protected main SHA');
}
function validateNotes(notes) {
  requireState(typeof notes === 'string' &&
    Buffer.byteLength(notes, 'utf8') > 1100 &&
    notes.includes(TAG) && notes.includes('PR #377'),
    'Release notes must identify exact v0.5.100 source scope');
  for (const warning of WARNINGS) {
    requireState(notes.includes(warning), 'Source-only warning absent: ' + warning);
  }
  requireState(!notes.includes('REPLACE BEFORE PUBLICATION') &&
    !notes.includes('DRAFT NOTES PLACEHOLDER'),
    'Unfinished release-note placeholder cannot qualify');
}
function decodeFile(blob, label) {
  requireState(blob?.type === 'file' && blob.encoding === 'base64' &&
    typeof blob.content === 'string',
    'Missing regular source file: ' + label);
  return Buffer.from(blob.content.replace(/\s/g, ''), 'base64').toString('utf8');
}
function directTag(ref, sha) {
  requireState(ref?.object?.type === 'commit' && ref.object.sha === sha,
    'Source tag is missing, annotated or moved: ' + sha);
}
function verifiedRelease(release, tag, sha, name, notes) {
  requireState(release?.draft === false && release.prerelease === true &&
    release.tag_name === tag && release.target_commitish === sha &&
    Array.isArray(release.assets) && release.assets.length === 0,
    'Pre-release identity, exact SHA or zero-assets constraint failed');
  if (notes !== undefined) {
    requireState(release.name === name && release.body === notes,
      'Existing release title or exact UTF-8 note bytes differ');
  }
}
async function optional(get) {
  try { return (await get()).data; }
  catch (error) {
    if (error.status === 404) return null;
    throw error;
  }
}
function requirePassingChecks(response, expectedSha) {
  requireState(SHA40.test(expectedSha), 'Expected exact check-run SHA is invalid');
  const rows = response?.check_runs;
  requireState(Array.isArray(rows), 'Exact-head checks response is unavailable');
  for (const name of REQUIRED_CHECKS) {
    // A superseded passing run cannot mask a newer failed/requeued run.
    const matches = rows.filter(row => row.name === name)
      .sort((a, b) => Number(b.id || 0) - Number(a.id || 0));
    requireState(matches.length > 0 && matches[0].app?.slug === 'github-actions' &&
      matches[0].head_sha === expectedSha,
      'Required exact-head check has untrusted app or commit identity: ' + name);
    requireState(matches[0].status === 'completed' &&
      matches[0].conclusion === 'success',
      'Required latest exact-head source check not successful: ' + name);
  }
}
async function inspectReadiness({github, context, candidateSha, notes}) {
  requireContext(context, candidateSha);
  validateNotes(notes);
  const api = github.rest;
  const {owner, repo} = context.repo;
  const main = (await api.repos.getBranch({owner, repo, branch:'main'})).data;
  requireState(main.protected === true && main.commit?.sha === candidateSha,
    'Protected main has moved, is unprotected or differs from candidate SHA');

  // Require exact final release notes from the same source commit.
  const noteBlob = (await api.repos.getContent({
    owner, repo, path:NOTES_PATH, ref:candidateSha,
  })).data;
  requireState(decodeFile(noteBlob, NOTES_PATH) === notes,
    'Prepared source-release notes do not match the pinned commit bytes');

  // PR #377 is Unreal runtime-affecting; do not treat an open draft as shipped.
  const pr = (await api.pulls.get({
    owner, repo, pull_number:REQUIRED_MERGED_PR,
  })).data;
  requireState(pr.state === 'closed' && pr.merged === true &&
    pr.base?.ref === 'main' &&
    pr.head?.repo?.full_name === 'sanskarIN/latebrake' &&
    SHA40.test(pr.merge_commit_sha),
    'Required live 3D drafting PR #377 is not merged into main');
  const ancestry = (await api.repos.compareCommits({
    owner, repo, base:pr.merge_commit_sha, head:candidateSha,
  })).data;
  requireState((ancestry.status === 'identical' || ancestry.status === 'ahead') &&
    ancestry.behind_by === 0,
    'Exact candidate main is not a descendant of PR #377 merge');

  // Static binding is an additional gate, not proof that real UE compiled.
  const prefix = 'Source/Latebrake/';
  const policy = decodeFile((await api.repos.getContent({
    owner, repo, path:prefix + 'Public/Racing/SlipstreamDraftBoost.h',
    ref:candidateSha,
  })).data, 'SlipstreamDraftBoost.h');
  const pawn = decodeFile((await api.repos.getContent({
    owner, repo, path:prefix + 'Private/Vehicles/LatebrakeCarPawn.cpp',
    ref:candidateSha,
  })).data, 'LatebrakeCarPawn.cpp');
  const mode = decodeFile((await api.repos.getContent({
    owner, repo, path:prefix + 'Private/Game/LatebrakeRaceGameMode.cpp',
    ref:candidateSha,
  })).data, 'LatebrakeRaceGameMode.cpp');
  const tests = decodeFile((await api.repos.getContent({
    owner, repo, path:'Tests/Standalone/SlipstreamDraftBoostTests.cpp',
    ref:candidateSha,
  })).data, 'SlipstreamDraftBoostTests.cpp');
  requireState(
    policy.includes('FollowerVerticalSpeedCmPerSecond') &&
    policy.includes('LeadVerticalSpeedCmPerSecond') &&
    policy.includes('dz - predictedZ') &&
    policy.includes('!std::isfinite(followerVertical)') &&
    policy.includes('MaxVerticalMotionResidualCm = 75.0') &&
    policy.includes('std::abs(verticalResidual) > MaxVerticalMotionResidualCm') &&
    policy.includes('std::uint64_t LeadIdentity = 0') &&
    policy.includes('sample.LeadIdentity == 0') &&
    policy.includes('PreviousLeadIdentity != sample.LeadIdentity') &&
    policy.includes('FollowerWorldX') &&
    policy.includes('FollowerWorldY') &&
    policy.includes('FollowerWorldZ') &&
    policy.includes('!std::isfinite(followerWorldX)') &&
    policy.includes('!std::isfinite(followerWorldY)') &&
    policy.includes('!std::isfinite(followerWorldZ)') &&
    policy.includes('PreviousFollowerWorldX = followerWorldX') &&
    policy.includes('PreviousFollowerWorldY = followerWorldY') &&
    policy.includes('PreviousFollowerWorldZ = followerWorldZ') &&
    policy.includes('followerResidual > MaxRelativeMotionResidualCm') &&
    policy.includes('std::abs(followerVerticalResidual)') &&
    policy.includes('const double leadWorldX = followerWorldX + dx;') &&
    policy.includes('const double leadWorldY = followerWorldY + dy;') &&
    policy.includes('const double leadWorldZ = followerWorldZ + dz;') &&
    policy.includes('const double predictedLeadX =') &&
    policy.includes('const double predictedLeadY =') &&
    policy.includes('const double predictedLeadZ =') &&
    policy.includes('leadResidual > MaxRelativeMotionResidualCm') &&
    policy.includes('std::abs(leadVerticalResidual)') &&
    policy.includes('const bool hadContinuityBaseline = HavePreviousOffset;') &&
    policy.includes('hadContinuityBaseline ? dt : 0.0') &&
    policy.includes('PredictedFollowerWorldX += dt * followerVx;') &&
    policy.includes('PredictedLeadWorldX += dt * leadVx;') &&
    policy.includes('followerWindowResidual > MaxRelativeMotionResidualCm') &&
    policy.includes('leadWindowResidual > MaxRelativeMotionResidualCm') &&
    policy.includes('MotionWindowSeconds >= ConfirmSeconds') &&
    pawn.includes('static_cast<double>(Position.X)') &&
    pawn.includes('static_cast<double>(Position.Y)') &&
    pawn.includes('static_cast<double>(Position.Z)') &&
    pawn.includes('static_cast<double>(Velocity.Z)') &&
    pawn.includes('static_cast<double>(RivalVelocity.Z)') &&
    pawn.includes('static_cast<std::uint64_t>(RivalIdentity)') &&
    mode.includes('RivalCar->GetUniqueID()') &&
    mode.includes('OutIdentity = RivalIdentity;') &&
    tests.includes('same-lane vertical relocation') &&
    tests.includes('identical-geometry replacement') &&
    tests.includes('rapid opponent swaps cannot inherit drafting charge') &&
    tests.includes('common-mode X relocation') &&
    tests.includes('common-mode lateral relocation') &&
    tests.includes('common-mode vertical relocation') &&
    tests.includes('genuine measured follower world movement') &&
    tests.includes('hidden 255cm lead relocation denied') &&
    tests.includes('combined 120cm rival Z jump rejected') &&
    tests.includes('independent rival continuity rejects rearward hidden relocation') &&
    tests.includes('physically predicted lead and follower motion retains drafting') &&
    tests.includes('initial drafting sample establishes baseline with zero time') &&
    tests.includes('stale world positions cannot earn drafting at 24m/s') &&
    tests.includes('stale lead chassis cannot hide behind moving follower') &&
    tests.includes('30/60/120 FPS physically integrated drafting follows elapsed time'),
    'Reviewed physical-3D/identity-aware draft code or regression absent from main');

  // Only exact main-source checks count. Never accept PR-head or branch-name
  // jobs as substitute for the release candidate's tested commit.
  const checks = (await api.checks.listForRef({
    owner, repo, ref:candidateSha, per_page:100,
  })).data;
  requirePassingChecks(checks, candidateSha);

  const predecessor = (await api.repos.getReleaseByTag({
    owner, repo, tag:PREVIOUS_TAG,
  })).data;
  verifiedRelease(predecessor, PREVIOUS_TAG, PREVIOUS_SHA);
  directTag((await api.git.getRef({
    owner, repo, ref:'tags/' + PREVIOUS_TAG,
  })).data, PREVIOUS_SHA);

  // READ ONLY: inspect existing state; never create, move or attach anything.
  const tag = await optional(() => api.git.getRef({
    owner, repo, ref:'tags/' + TAG,
  }));
  if (tag) directTag(tag, candidateSha);
  const release = await optional(() => api.repos.getReleaseByTag({
    owner, repo, tag:TAG,
  }));
  if (release) {
    requireState(tag !== null,
      'A published v0.5.100 release without immutable source tag is invalid');
    verifiedRelease(release, TAG, candidateSha, NAME, notes);
  }
  // Reject drift after asynchronous reads; inspection itself has no writes.
  const finalMain = (await api.repos.getBranch({
    owner, repo, branch:'main',
  })).data;
  requireState(finalMain.protected === true &&
    finalMain.commit?.sha === candidateSha,
    'Protected main changed during read-only preflight');
  return Object.freeze({
    candidateSha, predecessorSha:PREVIOUS_SHA,
    existingTag:tag !== null, existingRelease:release !== null,
    sourceChecks:REQUIRED_CHECKS.length,
    nativeUnrealQualified:false, playableWindowsQualified:false,
    writesPerformed:0,
  });
}
module.exports = {
  TAG, PREVIOUS_TAG, PREVIOUS_SHA, NAME, NOTES_PATH, WARNINGS,
  REQUIRED_MERGED_PR, REQUIRED_CHECKS, requireContext, validateNotes,
  requirePassingChecks, inspectReadiness,
};
