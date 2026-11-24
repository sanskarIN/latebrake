'use strict';

// READ-ONLY source-preview readiness gate. No createRef/createRelease, writes,
// Unreal build claims or token use. Runs only with owner-manual main dispatch.
// This is intentionally independent of the official v0.5.100 publisher.
const releaseAudit = require('./AuditPublishedSourceRelease.cjs');
const sourceGate = require('./SourceReleaseV05100Readiness.cjs');

const TAG = releaseAudit.REVIEWED_TEST_TAG;
const NAME = releaseAudit.REVIEWED_TEST_NAME;
const NOTES_PATH = releaseAudit.REVIEWED_TEST_NOTES_PATH;
const PREVIOUS_TAG = releaseAudit.LEGACY_TEST_TAG;
const PREVIOUS_SHA = releaseAudit.LEGACY_TEST_SHA;
const REQUIRED_SOURCE_PR = 395;
const UNMERGED_GAMEPLAY_PRS = Object.freeze([377, 391, 394]);
const SHA40 = /^[0-9a-f]{40}$/;
const REQUIRED_WARNINGS = Object.freeze([
  'SOURCE-ONLY',
  'NOT a playable game build',
  'No compiled Win64 EXE',
  'Android APK/AAB',
  'not installers',
  'zero executable binaries',
  'zero attached game assets',
  'not integrated into real Unreal gameplay',
]);

function requireState(condition, message) {
  if (!condition) throw new Error(message);
}

function checkContext(context, candidateSha) {
  requireState(
    context?.repo?.owner === 'sanskarIN' &&
    context.repo.repo === 'latebrake' &&
    context.actor === 'sanskarIN' &&
    context.eventName === 'workflow_dispatch' &&
    context.ref === 'refs/heads/main' &&
    typeof candidateSha === 'string' && SHA40.test(candidateSha) &&
    candidateSha === context.sha &&
    context.payload?.inputs?.candidate_sha === candidateSha,
    'Owner manual, protected-main exact-SHA review required'
  );
}

function checkNotes(notes) {
  requireState(typeof notes === 'string' &&
    Buffer.byteLength(notes, 'utf8') > 2200 &&
    notes.includes(TAG) &&
    notes.includes(NAME) &&
    notes.includes('PR #395') &&
    notes.includes('PR #377') &&
    notes.includes('PR #391') &&
    notes.includes('PR #394') &&
    notes.includes(PREVIOUS_TAG) &&
    notes.includes(PREVIOUS_SHA) &&
    notes.includes('87,792') &&
    notes.includes('+16,225'),
    'Source preview notes omit the exact old/new development boundary');
  for (const warning of REQUIRED_WARNINGS) {
    requireState(notes.includes(warning),
      'Source-only release warning missing: ' + warning);
  }
  requireState(!notes.includes('DRAFT NOTES PLACEHOLDER') &&
    !notes.includes('REPLACE BEFORE PUBLICATION'),
    'Unreviewed release-note placeholder');
}

function readUtf8File(blob) {
  requireState(blob?.type === 'file' &&
    blob.encoding === 'base64' &&
    typeof blob.content === 'string',
    'Tagged release notes must be a real, base64-backed source file');
  return Buffer.from(blob.content.replace(/\s/g, ''), 'base64').toString('utf8');
}

async function optional(get) {
  try { return (await get()).data; }
  catch (e) { if (e.status === 404) return null; throw e; }
}

async function inspectReadiness({github, context, candidateSha, notes}) {
  checkContext(context, candidateSha);
  checkNotes(notes);
  requireState(candidateSha !== PREVIOUS_SHA,
    'Preview must not silently retag the already-published test.1 commit');

  const api = github.rest;
  const {owner, repo} = context.repo;
  const branch = (await api.repos.getBranch({
    owner, repo, branch: 'main',
  })).data;
  requireState(branch.protected === true &&
    branch.commit?.sha === candidateSha,
    'Candidate is not the current protected-main exact SHA');

  const note = (await api.repos.getContent({
    owner, repo, path: NOTES_PATH, ref: candidateSha,
  })).data;
  requireState(readUtf8File(note) === notes,
    'Local notes differ from the exact remote protected-main source bytes');

  // This source-only preview includes a strictly merged portable PR and
  // MUST NOT imply runtime-affecting features from outstanding draft PRs.
  const merged = (await api.pulls.get({
    owner, repo, pull_number: REQUIRED_SOURCE_PR,
  })).data;
  requireState(merged.state === 'closed' && merged.merged === true &&
    merged.base?.ref === 'main' &&
    merged.head?.repo?.full_name === 'sanskarIN/latebrake' &&
    SHA40.test(merged.merge_commit_sha),
    'Original portable C++ PR #395 is not confirmed merged into main');
  const ancestry = (await api.repos.compareCommits({
    owner, repo, base: merged.merge_commit_sha, head: candidateSha,
  })).data;
  requireState((ancestry.status === 'identical' ||
    ancestry.status === 'ahead') && ancestry.behind_by === 0,
    'Candidate does not include the actual merged PR #395 source');

  for (const number of UNMERGED_GAMEPLAY_PRS) {
    const pr = (await api.pulls.get({
      owner, repo, pull_number: number,
    })).data;
    requireState(pr.state === 'open' && pr.merged !== true &&
      pr.base?.ref === 'main',
      'Source-only test preview scope changed; review gameplay PR #' + number);
  }

  const oldTag = (await api.git.getRef({
    owner, repo, ref: 'tags/' + PREVIOUS_TAG,
  })).data;
  const oldRelease = (await api.repos.getReleaseByTag({
    owner, repo, tag: PREVIOUS_TAG,
  })).data;
  releaseAudit.verifyPublishedSourceRelease({
    tag: PREVIOUS_TAG, reference: oldTag, release: oldRelease,
    sourceNotes: null,
  });
  requireState(oldTag.object.sha === PREVIOUS_SHA,
    'Previously published source test.1 tag has drifted');
  const oldAncestry = (await api.repos.compareCommits({
    owner, repo, base: PREVIOUS_SHA, head: candidateSha,
  })).data;
  requireState(oldAncestry.status === 'ahead' &&
    oldAncestry.behind_by === 0,
    'Candidate is not a new descendant of immutable v0.5.100-test.1');

  // Check latest exact-head trusted source jobs, not stale PR jobs.
  const checkRuns = (await api.checks.listForRef({
    owner, repo, ref: candidateSha, per_page: 100,
  })).data;
  sourceGate.requirePassingChecks(checkRuns, candidateSha);

  // Existing official v0.5.100 means a testing pre-release is stale.
  const officialTag = await optional(() => api.git.getRef({
    owner, repo, ref: 'tags/v0.5.100',
  }));
  requireState(officialTag === null,
    'Official v0.5.100 tag exists: do not publish another test.2 snapshot');

  const nextTag = await optional(() => api.git.getRef({
    owner, repo, ref: 'tags/' + TAG,
  }));
  const nextRelease = await optional(() => api.repos.getReleaseByTag({
    owner, repo, tag: TAG,
  }));
  requireState((nextTag === null) === (nextRelease === null),
    'Partial test.2 tag/release already exists; inspect manually before any action');
  if (nextRelease) {
    // A mutually consistent historical tag and release cannot qualify a
    // different current main commit, even if the note bytes happen to match.
    requireState(nextTag.object?.type === 'commit' &&
      nextTag.object.sha === candidateSha,
      'Existing test.2 is not pinned to the candidate exact immutable source tag SHA');
    releaseAudit.verifyPublishedSourceRelease({
      tag: TAG, reference: nextTag,
      release: nextRelease, sourceNotes: notes,
    });
  }
  const latest = (await api.repos.getBranch({
    owner, repo, branch: 'main',
  })).data;
  requireState(latest.protected === true &&
    latest.commit?.sha === candidateSha,
    'Protected main moved while source-readiness evidence was collected');

  return Object.freeze({
    tag: TAG, candidateSha, previousSha: PREVIOUS_SHA,
    sourceChecks: sourceGate.REQUIRED_CHECKS.length,
    readyForOwnerManualPublication: nextTag === null,
    alreadyPublished: nextRelease !== null,
    nativeUnrealQualified: false,
    playableWin64Qualified: false,
    writesPerformed: 0,
  });
}

module.exports = {
  TAG, NAME, NOTES_PATH, PREVIOUS_TAG, PREVIOUS_SHA,
  REQUIRED_SOURCE_PR, UNMERGED_GAMEPLAY_PRS, REQUIRED_WARNINGS,
  checkContext, checkNotes, inspectReadiness,
};
