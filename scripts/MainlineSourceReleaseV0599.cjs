'use strict';

// One-version, owner-authorized SOURCE-ONLY mainline publisher.
// It does NOT alter or relax the guarded, manual v0.5.98 draft publisher.
// Checked out and executed ONLY from reviewed protected main.
const TAG = 'v0.5.99';
const PREVIOUS_TAG = 'v0.5.98';
const PREVIOUS_SHA = 'bc0d5606d4c415f7603cc97f5ba6c90f59fa9c37';
const APPROVED_SUBJECT =
  'release(v0.5.99): approved source-only mainline snapshot';
const NOTES_PATH = 'docs/releases/v0.5.99.md';
const NAME = 'LateBrake v0.5.99 — Mainline Drafting and Safety (SOURCE-ONLY Pre-release)';
const SHA40 = /^[0-9a-f]{40}$/;
const WARNINGS = Object.freeze([
  'SOURCE-ONLY PRE-RELEASE', 'NO PLAYABLE WINDOWS EXECUTABLE',
  'NOT a playable game build', 'No compiled Win64 EXE',
  'Android APK/AAB', 'not installers', 'zero executable binaries',
  'zero attached game assets', 'v0.5.98',
  'unmerged development', 'road-tangent',
]);

function ensure(value, message) {
  if (!value) throw new Error(message);
}
function approvedContext(context) {
  const subject = context.payload?.head_commit?.message?.split('\n')[0];
  const approvedPush =
    context.eventName === 'push' && subject === APPROVED_SUBJECT;
  const approvedManual = context.eventName === 'workflow_dispatch';
  ensure(context.repo?.owner === 'sanskarIN' &&
    context.repo?.repo === 'latebrake' &&
    context.actor === 'sanskarIN' &&
    context.ref === 'refs/heads/main' &&
    SHA40.test(context.sha) &&
    (approvedPush || approvedManual),
    'Only a protected-main owner dispatch or exact one-shot approved push may publish');
}
function validateNotes(notes) {
  ensure(typeof notes === 'string' &&
    Buffer.byteLength(notes, 'utf8') > 1100 &&
    notes.includes(TAG) && notes.includes('PR #370'),
    'Release note missing its intended mainline source identity');
  for (const warning of WARNINGS) {
    ensure(notes.includes(warning), 'Missing release warning: ' + warning);
  }
}
async function optional(action) {
  try { return (await action()).data; }
  catch (error) {
    if (error.status === 404) return null;
    throw error;
  }
}
function directTag(ref, sha) {
  ensure(ref?.object?.type === 'commit' && ref.object.sha === sha,
    'Immutable source tag mismatch (annotated tags not accepted)');
}
function publishedRelease(release, tag, sha, name, notes) {
  ensure(release?.draft === false && release.prerelease === true &&
    release.tag_name === tag && release.target_commitish === sha &&
    Array.isArray(release.assets) && release.assets.length === 0,
    'Published identity, prerelease or zero-assets verification failed');
  if (notes !== undefined) {
    ensure(release.name === name && release.body === notes,
      'Published name or exact source-note bytes mismatch');
  }
}
function decodeFile(blob, label) {
  ensure(blob?.type === 'file' && blob.encoding === 'base64' &&
    typeof blob.content === 'string',
    'Missing or non-regular source file: ' + label);
  return Buffer.from(blob.content.replace(/\s/g, ''), 'base64').toString('utf8');
}
async function inspectState({github, context, notes}) {
  approvedContext(context);
  validateNotes(notes);
  const {owner, repo} = context.repo;
  const api = github.rest;
  const main = (await api.repos.getBranch({owner, repo, branch:'main'})).data;
  ensure(main.protected === true && main.commit?.sha === context.sha,
    'Protected main moved after verified checkout');
  const noteBlob = (await api.repos.getContent({
    owner, repo, path:NOTES_PATH, ref:context.sha
  })).data;
  ensure(decodeFile(noteBlob, NOTES_PATH) === notes,
    'Publisher notes differ from the exact mainline source');
  // Require the actual merged PR #370 gameplay path. This refuses to
  // advertise barrier-aware drafting if it was not merged into the tag.
  const policy = decodeFile((await api.repos.getContent({
    owner, repo, path:'Source/Latebrake/Public/Racing/SlipstreamDraftBoost.h',
    ref:context.sha
  })).data, 'SlipstreamDraftBoost.h');
  const mode = decodeFile((await api.repos.getContent({
    owner, repo, path:'Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp',
    ref:context.sha
  })).data, 'LatebrakeRaceGameMode.cpp');
  ensure(policy.includes('ClearPhysicalDraftPath') &&
    policy.includes('MaxRelativeMotionResidualCm') &&
    mode.includes('LatebrakeRivalDraftSightline') &&
    mode.includes('LineTraceSingleByChannel'),
    'Reviewed PR #370 barrier/motion safety code absent from source tag');

  const predecessor = (await api.repos.getReleaseByTag({
    owner, repo, tag:PREVIOUS_TAG
  })).data;
  publishedRelease(predecessor, PREVIOUS_TAG, PREVIOUS_SHA);
  directTag((await api.git.getRef({
    owner, repo, ref:'tags/' + PREVIOUS_TAG
  })).data, PREVIOUS_SHA);

  const tag = await optional(() => api.git.getRef({
    owner, repo, ref:'tags/' + TAG
  }));
  if (tag) directTag(tag, context.sha);
  const release = await optional(() => api.repos.getReleaseByTag({
    owner, repo, tag:TAG
  }));
  if (release) {
    ensure(tag !== null, 'Published release lacks its immutable source tag');
    publishedRelease(release, TAG, context.sha, NAME, notes);
  }
  return {tag, release};
}
async function publish(args) {
  const {github, context, notes} = args;
  let state = await inspectState(args);
  if (state.release) return state.release;
  const {owner, repo} = context.repo;
  if (!state.tag) {
    const newTag = (await github.rest.git.createRef({
      owner, repo, ref:'refs/tags/' + TAG, sha:context.sha
    })).data;
    directTag(newTag, context.sha);
  }
  state = await inspectState(args);
  ensure(state.tag !== null, 'Source tag missing after exact pin creation');
  if (!state.release) {
    await github.rest.repos.createRelease({
      owner, repo, tag_name:TAG, target_commitish:context.sha,
      name:NAME, body:notes,
      draft:false, prerelease:true, generate_release_notes:false,
      make_latest:'false',
    });
  }
  state = await inspectState(args);
  ensure(state.release !== null, 'Release absent after publication readback');
  return state.release;
}
module.exports = {
  TAG, PREVIOUS_TAG, PREVIOUS_SHA, APPROVED_SUBJECT, NOTES_PATH,
  NAME, WARNINGS, approvedContext, validateNotes, inspectState, publish,
};
