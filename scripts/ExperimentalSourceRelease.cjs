'use strict';

// Run ONLY from reviewed protected-main code. Candidate PR content is read
// through the API as data; never require/eval candidate code with write scope.
// The version policy is limited to 0.5.x experimental source snapshots.
const REQUIRED_WARNINGS = Object.freeze([
  'SOURCE-ONLY PRE-RELEASE', 'NO PLAYABLE WINDOWS EXECUTABLE',
  'NOT a playable game build', 'No compiled Win64 EXE', 'Android APK/AAB',
  'not installers', 'zero executable binaries', 'zero attached game assets',
  'unmerged development', 'road-tangent',
]);

function requireState(condition, reason) {
  if (!condition) throw new Error(reason);
}

function validatePlan(context, plan, notes) {
  requireState(context.repo?.owner === 'sanskarIN' && context.repo?.repo === 'latebrake' &&
    context.actor === 'sanskarIN' && context.eventName === 'workflow_dispatch' &&
    context.ref === 'refs/heads/main', 'Only the owner manual protected-main workflow may publish');
  const exactSha = /^[0-9a-f]{40}$/;
  requireState(exactSha.test(context.sha) && exactSha.test(plan.sourceSha) &&
    exactSha.test(plan.previousSha), 'Source/main/predecessor require exact lowercase commit SHAs');
  const next = /^v0\.5\.([0-9]+)$/.exec(plan.tag);
  const previous = /^v0\.5\.([0-9]+)$/.exec(plan.previousTag);
  requireState(next && previous && Number.isSafeInteger(Number(next[1])) &&
    Number.isSafeInteger(Number(previous[1])) && next[1] === String(Number(next[1])) &&
    previous[1] === String(Number(previous[1])) && Number(next[1]) === Number(previous[1]) + 1,
    'Only sequential 0.5.x source snapshots are supported; v0.6.0 has separate gates');
  requireState(plan.prNumber === 295 && plan.branch === 'feat/chaos-four-corner-suspension',
    'Only the reviewed same-repository physics draft is eligible');
  requireState(plan.notesPath === `docs/releases/${plan.tag}.md` &&
    typeof plan.name === 'string' && plan.name.includes(plan.tag) &&
    plan.name.includes('SOURCE-ONLY'), 'Unexpected notes path or source-only release title');
  requireState(typeof notes === 'string' && Buffer.byteLength(notes, 'utf8') > 500,
    'Release notes must be complete UTF-8 source-only notes');
  for (const warning of REQUIRED_WARNINGS) {
    requireState(notes.includes(warning), `Missing release warning: ${warning}`);
  }
}

async function optional(get) {
  try { return (await get()).data; }
  catch (error) {
    if (error.status === 404) return null;
    throw error;
  }
}

function validateTag(ref, sha) {
  requireState(ref?.object?.type === 'commit' && ref.object.sha === sha,
    'Never rewrite a tag: immutable source commit mismatch');
}

function validateRelease(release, plan, notes) {
  requireState(release?.draft === false && release.prerelease === true &&
    release.tag_name === plan.tag && release.target_commitish === plan.sourceSha &&
    release.name === plan.name && release.body === notes &&
    Array.isArray(release.assets) && release.assets.length === 0,
    'Existing/published source release identity, notes or zero-assets check failed');
}

async function inspectState({github, context, plan, notes}) {
  validatePlan(context, plan, notes);
  const {owner, repo} = context.repo;
  const api = github.rest;
  const main = (await api.repos.getBranch({owner, repo, branch: 'main'})).data;
  requireState(main.protected === true && main.commit?.sha === context.sha,
    'Protected main moved or is not protected');
  const pr = (await api.pulls.get({owner, repo, pull_number: plan.prNumber})).data;
  requireState(pr.state === 'open' && pr.draft === true && pr.base?.ref === 'main' &&
    pr.head?.ref === plan.branch && pr.head?.repo?.full_name === 'sanskarIN/latebrake' &&
    pr.head?.user?.login === 'sanskarIN' && pr.head?.sha === plan.sourceSha,
    'Untrusted, closed, ready-for-merge or moved draft PR head');
  const sourceNotes = (await api.repos.getContent({
    owner, repo, path: plan.notesPath, ref: plan.sourceSha,
  })).data;
  requireState(sourceNotes?.type === 'file' && sourceNotes.encoding === 'base64' &&
    typeof sourceNotes.content === 'string' &&
    Buffer.from(sourceNotes.content, 'base64').equals(Buffer.from(notes, 'utf8')),
    'Publisher notes differ from the exact source commit');
  const predecessor = (await api.repos.getReleaseByTag({owner, repo, tag: plan.previousTag})).data;
  requireState(predecessor.draft === false && predecessor.prerelease === true &&
    predecessor.tag_name === plan.previousTag && predecessor.target_commitish === plan.previousSha &&
    Array.isArray(predecessor.assets) && predecessor.assets.length === 0,
    'Previous source-only release identity/zero-assets check failed');
  validateTag((await api.git.getRef({owner, repo, ref: `tags/${plan.previousTag}`})).data,
    plan.previousSha);
  const tag = await optional(() => api.git.getRef({owner, repo, ref: `tags/${plan.tag}`}));
  if (tag) validateTag(tag, plan.sourceSha);
  const release = await optional(() => api.repos.getReleaseByTag({owner, repo, tag: plan.tag}));
  if (release) {
    validateRelease(release, plan, notes);
    requireState(tag !== null, 'Published release has no immutable source tag');
  }
  return {tag, release};
}

async function publish(args) {
  const {github, context, plan, notes} = args;
  let state = await inspectState(args); // reject bad existing releases BEFORE any write
  if (state.release) return state.release; // exact, read-back-verified idempotent retry
  const {owner, repo} = context.repo;
  if (!state.tag) {
    const result = await github.rest.git.createRef({
      owner, repo, ref: `refs/tags/${plan.tag}`, sha: plan.sourceSha,
    });
    validateTag(result.data, plan.sourceSha);
  }
  // Authenticate again after creating the tag, before publishing anything.
  // Drift may leave a source tag, but must not create a misleading release.
  state = await inspectState(args);
  requireState(state.tag !== null, 'Source tag disappeared before publication');
  if (!state.release) {
    await github.rest.repos.createRelease({
      owner, repo, tag_name: plan.tag, target_commitish: plan.sourceSha,
      name: plan.name, body: notes, draft: false, prerelease: true,
      generate_release_notes: false, make_latest: 'false',
    });
  }
  state = await inspectState(args); // fresh tag, source, notes and asset readback
  requireState(state.release !== null, 'Release missing after publication');
  return state.release;
}

module.exports = {REQUIRED_WARNINGS, validatePlan, inspectState, publish};
